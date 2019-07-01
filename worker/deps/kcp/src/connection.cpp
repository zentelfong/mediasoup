#include "connection.h"

namespace kcp {

#define DEFAULT_UPDATE_DELAY 30

enum {
    MSG_RECV_PACKET,
    MSG_UPDATE,
    MSG_DESTROY_CONN,
};

inline int64_t kTimeMillis() {
    uv_timeval64_t tv;
    uv_gettimeofday(&tv);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void onClose(uv_handle_t* handle)
{
    free(handle);
}


///////////////////////////////////////////////////////////////////////////////////
//connection

static void OutLog(const char *log, struct IKCPCB *kcp, void *user)
{
    
}

inline static void onTimer(uv_timer_t* handle)
{
    static_cast<Connection*>(handle->data)->OnTimer();
}


Connection::Connection(Server* server, uint32_t id)
    :server_(server), connect_key_(0),
    last_recv_time_(0),connection_time_(0),timeout_(60000)
{
    memset(&addr_, 0, sizeof(addr_));
    ikcp_ctor(&kcp_, id, this);
    ikcp_setmtu(&kcp_, KDEFAULT_MTU);
    ikcp_wndsize(&kcp_, 64, 64);
    ikcp_nodelay(&kcp_, false, DEFAULT_UPDATE_DELAY, 2, 0);
    kcp_.output = SendPacket;
    kcp_.writelog = OutLog;
    last_recv_time_ = kTimeMillis();
    connection_time_ = last_recv_time_;

    timer_ = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(server_->loop(), timer_);

    uv_timer_start(timer_, onTimer, 10, 1);
}


Connection::~Connection() {
    ikcp_dtor(&kcp_);
    uv_close((uv_handle_t*)timer_, onClose);
}

int Connection::SendPacket(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    Connection* pThis = (Connection*)user;
    pThis->SendPacket(buf, len);
    return 0;
}

void Connection::SendPacket(const char* data, size_t len) {
    server_->SendPacket(data, len, addr_);
}

void Connection::OnTimer() {
    int64_t time = kTimeMillis();
    IUINT32 cur = (IUINT32)(time - connection_time_);
    ikcp_update(&kcp_, cur);

    int readable = 0, writeable = 0;

    if (ikcp_check_read_write(&kcp_, &readable, &writeable))
    {
        if (readable)
            OnReadEvent();

        if (writeable)
            OnWriteEvent();
    }

    //接收超时
    if (time > last_recv_time_ + timeout_)
    {
        Destroy();
    }
    else
    {
        uv_timer_start(timer_, onTimer, ikcp_check(&kcp_, cur) - cur, 1);
    }
}

int Connection::Send(const char* data, size_t len) {
    return ikcp_send(&kcp_, data, len);
}

int Connection::Recv(char* data, size_t len) {
    return ikcp_recv(&kcp_, data, len);
}

void Connection::OnRecvUdpPacket(const char* data, int len,
    const sockaddr& addr) {
    last_recv_time_ = kTimeMillis();
    addr_ = addr;

    if (len> IUSER_DATA_HEAD_LEN && data[4] == IUSER_DATA_CMD)
    {
        OnRecvUserPacket(data+ IUSER_DATA_HEAD_LEN, len- IUSER_DATA_HEAD_LEN);
    }
    else
        ikcp_input(&kcp_, data, len);
}

void Connection::Destroy() {
    OnClose();
    server_->RemoveConnection(this);
    delete this;
}

void Connection::SendUserPacket(const char* data, size_t len){
    uint8_t buffer[KMAX_UDP_PACKET];

    kWriteScalar<uint32_t>(buffer, kcp_.conv);
    kWriteScalar<uint8_t>(buffer+4, IUSER_DATA_CMD);

    memcpy(buffer + 5, data, len);
    server_->SendPacket((char*)buffer, len + 5, addr_);
}


////////////////////////////////////////////////////////////////////////////////
//dispatcher

Server::Server(uv_loop_t* loop)
    :loop_(loop), socket_(nullptr)
{
    socket_ = (uv_udp_t*)malloc(sizeof(uv_udp_t));
    uv_udp_init(loop_, socket_);
    socket_->data = this;

    crypt_ = new RandomPacketCrypto();
}


Server::~Server() {
    for (auto it : connections_) {
        delete it.second;
    }

    uv_udp_recv_stop(socket_);
    uv_close(reinterpret_cast<uv_handle_t*>(socket_), static_cast<uv_close_cb>(onClose));

    delete crypt_;
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    buf->base =(char*) malloc(suggested_size);
    buf->len = suggested_size;
}

void Server::on_read(uv_udp_t *req, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
    Server* server = (Server*)req->data;
    char data[KMAX_UDP_PACKET];
    int len = server->crypt_->Decrypt((const uint8_t*)buf->base, buf->len, (uint8_t*)data);
    if(len>0)
        server->OnRecvPacket(data, len, *addr);
    free(buf->base);
}

bool Server::Bind(const sockaddr* addr) {
    if (!uv_udp_bind(socket_, addr, UV_UDP_REUSEADDR))
        return false;
    return uv_udp_recv_start(socket_, alloc_buffer, on_read) ==0;
}

Connection* Server::FindConnection(uint32_t id) {
    auto find = connections_.find(id);
    if (find != connections_.end()) {
        return find->second;
    }
    return nullptr;
}

void Server::RemoveConnection(Connection* conn) {
    auto find = connections_.find(conn->id());
    if (find != connections_.end()) {
        connections_.erase(find);
    }
}

void Server::SendResponse(const sockaddr& addr, uint32_t kcpid, KCONTROL_TYPE type)
{
    char buf[24] = { 0 };
    KControlPacketHead keepAlivePacket;
    keepAlivePacket.unuse = 0;
    keepAlivePacket.kcpid = kcpid;
    keepAlivePacket.controlType = type;
    keepAlivePacket.Write(buf);


    SendPacket(buf, sizeof(buf), addr);
}

void Server::SendUserPacket(uint32_t connid, const char* data, int len) {
    Connection* con = FindConnection(connid);
    if (con)
    {
        con->SendUserPacket(data, len);
    }
}

void Server::OnRecvPacket(const char* data, int len,
    const sockaddr& addr) {
    //处理收到的数据包
    if (len<0) {
        uint32_t connid = kReadScalar<uint32_t>(data);
        if (connid>0)//如果收到connid为0表示客户端的keepalive
        {
            Connection * conn = FindConnection(connid);
            if (conn)
            {
                conn->OnRecvUdpPacket(data, len, addr);
            }
            else
            {
                //通知客户端连接已超时
                SendResponse(addr, connid, KCT_CLOSED);
            }
        }
        else
        {
            //KServer的控制数据包,keepAlive，connect等
            KControlPacketHead packetHead;
            packetHead.Read(data);
            connid = packetHead.kcpid;
            switch (packetHead.controlType)
            {
            case KCT_KEEP_ALIVE:
            {
                Connection * conn = FindConnection(connid);
                if (conn)
                {
                    conn->SetLastRecvTime(kTimeMillis());
                    SendResponse(addr, connid, KCT_KEEP_ALIVE);
                }
                else
                {
                    //通知客户端连接已超时
                    SendResponse(addr, connid, KCT_CLOSED);
                }
                break;
            }
            case KCT_CONNECT_REQ:
                if (len >= KConnectReqPacket::MinSize())
                {
                    //客户端连接请求
                    KConnectReqPacket reqPacket;
                    reqPacket.Read(data);

                    uint64_t hashVal;
                    uint32_t key = 0;
                    if (addr.sa_family == AF_INET)
                    {
                        sockaddr_in * addr_in = (sockaddr_in*)&addr;
                        key = addr_in->sin_addr.s_addr ^ addr_in->sin_port;
                    }
                    else {
                        sockaddr_in6 * addr_in = (sockaddr_in6*)&addr;
                        const uint32_t* v6_as_ints =
                            reinterpret_cast<const uint32_t*>(&addr_in->sin6_addr.s6_addr);
                        key = v6_as_ints[0] ^ v6_as_ints[1] ^ v6_as_ints[2] ^ v6_as_ints[3] ^ addr_in->sin6_port;
                    }

                    bool newConn = false;
                    hashVal = key;
                    key ^= reqPacket.cookie;
                    hashVal |= ((uint64_t)key) << 32;

                    for (uint32_t i = key; i != key - 1; ++i)
                    {
                        Connection* conn = FindConnection(i);
                        if (!conn)
                        {
                            newConn = true;
                            connid = i;
                            break;
                        }
                        else if (conn->connect_key() == hashVal)
                        {
                            connid = i;
                            break;
                        }
                    }

                    if (newConn)
                    {
                        Connection * conn = OnConnection(this,connid);//新连接到来
                        if (reqPacket.interval < 10)
                            reqPacket.interval = 10;

                        conn->SetLastRecvTime(kTimeMillis());
                        conn->SetConnectKey(hashVal);
                        conn->SetWndSize(reqPacket.sndwnd, reqPacket.rcvwnd);
                        conn->SetNodelay(reqPacket.nodelay, reqPacket.interval, 
                            reqPacket.fastResend, reqPacket.nocwnd);
                        conn->SetAddress(addr);
                        conn->SetMtu(reqPacket.mtu);

                        connections_.insert(std::pair<uint32_t,Connection*>(connid, conn));
                    }

                    //发送连接通知
                    {
                        char temp[24] = { 0 };
                        KConnectRspPacket rspPacket;
                        rspPacket.unuse = 0;
                        rspPacket.cookie = reqPacket.cookie;
                        rspPacket.kcpid = connid;
                        rspPacket.controlType = KCT_CONNECT_RSP;
                        rspPacket.code = KConnectRspPacket::CODE_OK;

                        rspPacket.Write(temp);
                        SendPacket(temp, sizeof(temp), addr);
                    }
                }
                break;
            case KCT_CLOSE:
            {
                    auto find = connections_.find(connid);
                    if (find != connections_.end()) {
                        Connection* connection = find->second;
                        connections_.erase(find);
                        connection->OnClose();
                        delete connection;
                    }
                    //回应一个关闭，让客户端处理
                    SendResponse(addr, connid, KCT_CLOSED);
            }
            break;
            }
        }
    }


}

struct UvSendData
{
    uv_udp_send_t req;
    uint8_t store[1];
};

static void onSend(uv_udp_send_t* req, int status)
{
    auto* sendData = static_cast<UvSendData*>(req->data);
    // Delete the UvSendData struct (which includes the uv_req_t and the store char[]).
    free(sendData);
}


int Server::SendPacket(const char* data, int len,
    const sockaddr& addr) 
{
    if (len == 0)
        return -1;

    char endata[KMAX_UDP_PACKET];
    len = crypt_->Encrypt((const uint8_t*)data, len, (uint8_t*)endata);


    // First try uv_udp_try_send(). In case it can not directly send the datagram
    // then build a uv_req_t and use uv_udp_send().
    uv_buf_t buffer = uv_buf_init(endata, len);
    int sent = uv_udp_try_send(socket_, &buffer, 1, &addr);

    // Entire datagram was sent. Done.
    if (sent == static_cast<int>(len))
    {
        return sent;
    }

    if (sent >= 0)
    {
        return sent;
    }

    // Error,
    if (sent != UV_EAGAIN)
    {
        return sent;
    }

    // Otherwise UV_EAGAIN was returned so cannot send data at first time. Use uv_udp_send().

    // Allocate a special UvSendData struct pointer.
    auto* sendData = static_cast<UvSendData*>(malloc(sizeof(UvSendData) + len));

    std::memcpy(sendData->store, endata, len);
    sendData->req.data = (void*)sendData;

    buffer = uv_buf_init(reinterpret_cast<char*>(sendData->store), len);

    int err = uv_udp_send(
        &sendData->req, socket_, &buffer, 1, &addr, static_cast<uv_udp_send_cb>(onSend));

    if (err != 0)
    {
        // NOTE: uv_udp_send() returns error if a wrong INET family is given
        // (IPv6 destination on a IPv4 binded socket), so be ready.
        // Delete the UvSendData struct (which includes the uv_req_t and the store char[]).
        free(sendData);
    }
    return err;
}


}//namespace

