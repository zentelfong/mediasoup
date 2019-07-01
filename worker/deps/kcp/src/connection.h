#pragma once
#include "ikcp.h"
#include "protocol.h"
#include "packetcrypto.h"
#include <unordered_map>
#include <uv.h>

namespace kcp
{
class Server;

//kcp的连接
class Connection
{
public:
    Connection(Server* server, uint32_t id);
    ~Connection();

    void SetConnectKey(uint64_t key) {
        connect_key_ = key;
    }
    uint64_t connect_key() { return connect_key_; }

    int Send(const char* data, size_t len);
    int Recv(char* data, size_t len);

    //通知事件
    virtual void OnReadEvent() = 0;
    virtual void OnWriteEvent() = 0;

    //收发自定义udp包
    void SendUserPacket(const char* data, size_t len);
    virtual void OnRecvUserPacket(const char* data, size_t len) {}

    //连接关闭
    virtual void OnClose() = 0;

    //参数设置
    inline void SetLastRecvTime(int64_t time) { last_recv_time_ = time; }
    inline void SetAddress(const sockaddr addr) { addr_ = addr; }
    inline void SetMtu(uint16_t mtu) { kcp_.mtu = mtu; }
    inline int SetWndSize(int sndwnd, int rcvwnd){ 
        return ikcp_wndsize(&kcp_, sndwnd, rcvwnd);
    }

    inline int SetNodelay(int nodelay, int interval, int resend, int nc)
    {
        return ikcp_nodelay(&kcp_, nodelay, interval, resend, nc);
    }

    inline void SetTimeout(int timeout) {
        timeout_ = timeout;
    }

    uint32_t id() { return kcp_.conv; }
    //销毁当前连接，会delete this
    void Destroy();

    void OnTimer();

protected:
    friend class Server;

    void OnRecvUdpPacket(const char* data, int len,
        const sockaddr& addr);

    static int SendPacket(const char *buf, int len, struct IKCPCB *kcp, void *user);
    void SendPacket(const char* data, size_t len);
    Server* server_;
    uint64_t connect_key_;
    ikcpcb  kcp_;
    sockaddr addr_;//目标地址
    int64_t last_recv_time_;//超时检测使用
    int64_t connection_time_;
    int     timeout_;
    uv_timer_t *timer_;
};

class Server
{
public:
    Server(uv_loop_t* loop);
    ~Server();

    bool Bind(const sockaddr* addr);
    Connection* FindConnection(uint32_t id);
    uv_loop_t* loop() { return loop_; }

    //发送自定义包
    void SendUserPacket(uint32_t connid, const char* data, int len);

protected:
    virtual Connection* OnConnection(Server* server, uint32_t id) = 0;

private:
    friend class Connection;

    static void on_read(uv_udp_t *req, ssize_t nread, 
        const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags);

    //收到udp包
    void OnRecvPacket(const char* data, int len,
        const sockaddr& addr);

    int SendPacket(const char* data, int len,
        const sockaddr& addr);

    void SendResponse(const sockaddr& addr,
        uint32_t kcpid, KCONTROL_TYPE type);

    void RemoveConnection(Connection* conn);

private:
    std::unordered_map<uint32_t, Connection*> connections_;
    uv_loop_t* loop_;
    uv_udp_t* socket_;
    PacketCryptoBase* crypt_;
};

}//namespace

