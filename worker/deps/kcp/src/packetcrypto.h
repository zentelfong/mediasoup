#pragma once
#include <stdint.h>

namespace kcp {

#define MAX_PACKET_SIZE 1600

class PacketCryptoBase
{
public:
	virtual ~PacketCryptoBase(){}
	virtual int Encrypt(const uint8_t* data, int dataLen, unsigned char* dataOut) = 0;
	virtual int Decrypt(const uint8_t* data, int dataLen, unsigned char* dataOut) = 0;
};

class RandomPacketCrypto:public PacketCryptoBase
{
public:
	RandomPacketCrypto();
	~RandomPacketCrypto();

	int Encrypt(const uint8_t* data, int dataLen, unsigned char* dataOut) override;
	int Decrypt(const uint8_t* data, int dataLen, unsigned char* dataOut) override;
};



}//namespace
