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
//������������
enum KCONTROL_TYPE {
	KCT_NONE,
	KCT_CONNECT_REQ,//���������
	KCT_CONNECT_RSP,//���ӻ�Ӧ��
	KCT_CLOSE,//�ر���������
    KCT_CLOSED,//���ӳ�ʱ�����ͬ���������Ϳͻ��˵ĳ�ʱ״̬
	KCT_KEEP_ALIVE,//�������Ӽ�����
	KCT_CHECK_MTU,//���mtu��С
	KCT_PUSH,//��������
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

	uint32_t unuse;//Ϊ0
	uint32_t controlType;//��������
	uint32_t kcpid;//���ӵ�id
};

//mtu��С���
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
	uint32_t mtu;//����ʱ�ͻ������ɵĴ�����������
};


//���������
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

	uint32_t cookie;//����ʱ�ͻ������ɵĴ�����������
	int32_t sndwnd;//���ͻ������ڴ�С
	int32_t rcvwnd;//���ջ������ڴ�С
	int32_t interval;//����ʱ����
	uint8_t nodelay;//�Ƿ����ӳٷ���
	uint8_t fastResend;//�Ƿ��������ش�
	uint8_t nocwnd;//�Ƿ�ر���������
	uint16_t mtu;//mtu
};

//���ӻ�Ӧ��
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
	uint32_t cookie;//����ʱ�ͻ������ɵĴ�����������
	uint32_t code;
    sockaddr_in redirect;
};



struct KOptions 
{
	KOptions()
	{
		timeOutInterval = 3 * 60 * 1000;//Ĭ��3����
		keepAliveInterval = 30 * 1000;//Ĭ��30s����һ������
		mtu = KDEFAULT_MTU;
		sndwnd = 128;
		rcvwnd = 128;
		interval = 20;

		//Ĭ��udp�����СΪ128kʱ�ٶ����
		udpsendbuf = 128 * 1024;
		udprecvbuf = 128 * 1024;

		nodelay = false;
		fastResend = 2;
		nocwnd = false;
		minrto = 100;
	}
	int32_t	timeOutInterval;//��ʱʱ����
	int32_t keepAliveInterval;//����ʱ����
	int mtu;//MTU
	int sndwnd;//���ͻ������ڴ�С
	int rcvwnd;//���ջ������ڴ�С
	int interval;//����ʱ����

	//udp�����С
	int udpsendbuf;
	int udprecvbuf;

	bool nodelay;//�Ƿ����ӳٷ���
	uint8_t fastResend;//�Ƿ��������ش�
	bool nocwnd;//�Ƿ�ر���������
	int  minrto;
};

}//namespace
