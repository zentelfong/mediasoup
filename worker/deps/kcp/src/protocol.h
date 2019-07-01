#pragma once
#include "kendian.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#else
#include <arpa/inet.h>  // htonl(), htons(), ntohl(), ntohs()
#include <netinet/in.h> // sockaddr_in, sockaddr_in6
#endif

#define KDEFAULT_MTU 1400
#define KMAX_UDP_PACKET 1600

namespace kcp {
//心跳包等类型
enum KCONTROL_TYPE {
	KCT_NONE,
	KCT_CONNECT_REQ,//连接请求包
	KCT_CONNECT_RSP,//连接回应包
	KCT_CLOSE,//关闭连接类型
    KCT_CLOSED,//连接超时，如果同步服务器和客户端的超时状态
	KCT_KEEP_ALIVE,//保持连接及心跳
	KCT_CHECK_MTU,//检测mtu大小
	KCT_PUSH,//推送数据
};

struct KControlPacketHead
{
	static int MinSize() {
		return 12;
	}
	int Read(const char* buf)
	{
		const char* ptr = buf;
		buf += kReadScalar(buf, &unuse);
		buf += kReadScalar(buf, &controlType);
		buf += kReadScalar(buf, &kcpid);
		return (buf - ptr);
	}

	int Write(char* buf)
	{
		const char* ptr = buf;
		buf += kWriteScalar(buf, unuse);
		buf += kWriteScalar(buf, controlType);
		buf += kWriteScalar(buf, kcpid);
		return (buf - ptr);
	}

	uint32_t unuse;//为0
	uint32_t controlType;//控制类型
	uint32_t kcpid;//连接的id
};

//mtu大小检测
struct KMtuPacketHead :public KControlPacketHead
{
	static int MinSize() {
		return 16;
	}
	int Read(const char* buf)
	{
		const char* ptr = buf;
		buf += KControlPacketHead::Read(buf);
		buf += kReadScalar(buf, &mtu);
		return (buf - ptr);
	}

	int Write(char* buf)
	{
		const char* ptr = buf;
		buf += KControlPacketHead::Write(buf);
		buf += kWriteScalar(buf, mtu);
		return (buf - ptr);
	}
	uint32_t mtu;//握手时客户端生成的传给服务器的
};


//连接请求包
struct KConnectReqPacket :public KControlPacketHead
{
	static size_t MinSize() {
		return 33;
	}
	int Read(const char* buf)
	{
		const char* ptr = buf;
		buf += KControlPacketHead::Read(buf);
		buf += kReadScalar(buf, &cookie);

		buf += kReadScalar(buf, &sndwnd);
		buf += kReadScalar(buf, &rcvwnd);
		buf += kReadScalar(buf, &interval);
		buf += kReadScalar(buf, &nodelay);
		buf += kReadScalar(buf, &fastResend);
		buf += kReadScalar(buf, &nocwnd);
		buf += kReadScalar(buf, &mtu);
		return (buf - ptr);
	}

	int Write(char* buf)
	{
		const char* ptr = buf;
		buf += KControlPacketHead::Write(buf);
		buf += kWriteScalar(buf, cookie);

		buf += kWriteScalar(buf, sndwnd);
		buf += kWriteScalar(buf, rcvwnd);
		buf += kWriteScalar(buf, interval);
		buf += kWriteScalar(buf, nodelay);
		buf += kWriteScalar(buf, fastResend);
		buf += kWriteScalar(buf, nocwnd);
		buf += kWriteScalar(buf, mtu);
		return (buf - ptr);
	}

	uint32_t cookie;//握手时客户端生成的传给服务器的
	int32_t sndwnd;//发送滑动窗口大小
	int32_t rcvwnd;//接收滑动窗口大小
	int32_t interval;//更新时间间隔
	uint8_t nodelay;//是否无延迟发包
	uint8_t fastResend;//是否开启快速重传
	uint8_t nocwnd;//是否关闭流量控制
	uint16_t mtu;//mtu
};

//连接回应包
struct KConnectRspPacket :public KControlPacketHead
{
	enum {
		CODE_OK,
		CODE_REDIRECT
	};

	static int MinSize() {
		return 20;
	}

	int Read(const char* buf)
	{
		const char* ptr = buf;
		buf += KControlPacketHead::Read(buf);
		buf += kReadScalar(buf, &cookie);
		buf += kReadScalar(buf, &code);
		if (code == CODE_REDIRECT)
		{
			buf += kReadScalar<uint16_t>(buf, &redirect.sin_family);
			buf += kReadScalar<uint16_t>(buf, &redirect.sin_port);
			buf += kReadScalar<uint32_t>(buf, (uint32_t*)&redirect.sin_addr.s_addr);
		}
		return (buf-ptr);
	}
	uint32_t cookie;//握手时客户端生成的传给服务器的
	uint32_t code;
    sockaddr_in redirect;
};



struct KOptions 
{
	KOptions()
	{
		timeOutInterval = 3 * 60 * 1000;//默认3分钟
		keepAliveInterval = 30 * 1000;//默认30s发送一次心跳
		mtu = KDEFAULT_MTU;
		sndwnd = 128;
		rcvwnd = 128;
		interval = 20;

		//默认udp缓存大小为128k时速度最快
		udpsendbuf = 128 * 1024;
		udprecvbuf = 128 * 1024;

		nodelay = false;
		fastResend = 2;
		nocwnd = false;
		minrto = 100;
	}
	int32_t	timeOutInterval;//超时时间间隔
	int32_t keepAliveInterval;//心跳时间间隔
	int mtu;//MTU
	int sndwnd;//发送滑动窗口大小
	int rcvwnd;//接收滑动窗口大小
	int interval;//更新时间间隔

	//udp缓存大小
	int udpsendbuf;
	int udprecvbuf;

	bool nodelay;//是否无延迟发包
	uint8_t fastResend;//是否开启快速重传
	bool nocwnd;//是否关闭流量控制
	int  minrto;
};

}//namespace
