#include "packetcrypto.h"
#include <stdlib.h>
#include <string.h>

//little endian
#define le32toh(v) (v)
#define htole32(v) (v)

namespace kcp {

RandomPacketCrypto::RandomPacketCrypto() {

}
RandomPacketCrypto::~RandomPacketCrypto() {

}

#define CRYPT_ROUND 16
#define PACKET_OVERHEAD 4
#define ENCRYPT_KEY {0x60763BC5,0xE25847C3,0x94322080,0xA49920AC}



/* take 64 bits of data in v[0] and v[1] and 128 bits of key[0] - key[3] */
void encryptBlock(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]) {
	unsigned int i;
	uint32_t v0 = le32toh(v[0]);
	uint32_t v1 = le32toh(v[1]);
	uint32_t sum = 0, delta = 0x9E3779B9;
	for (i = 0; i < num_rounds; i++) {
		v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
	}
	v[0] = htole32(v0);
	v[1] = htole32(v1);
}

void decryptBlock(unsigned int num_rounds, uint32_t v[2], uint32_t const key[4]) {
	unsigned int i;
	uint32_t v0 = le32toh(v[0]);
	uint32_t v1 = le32toh(v[1]);
	uint32_t delta = 0x9E3779B9, sum = delta*num_rounds;
	for (i = 0; i < num_rounds; i++) {
		v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
		sum -= delta;
		v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
	}
	v[0] = htole32(v0);
	v[1] = htole32(v1);
}


//加密data必须为8的倍数
void encryptData(uint32_t *data, int len, uint32_t key[4])
{
	uint32_t* dataEnd = data + len;
	while (data < dataEnd)
	{
		uint32_t val = *data;
		encryptBlock(CRYPT_ROUND, data, key);
		for (int i = 0; i<4; ++i)
		{
			key[i] ^= val;//将第一次的内容和key进行异或，生成第二次加密秘钥
		}
		data += 2;
	}
}

//解密data必须为8的倍数
void decryptData(uint32_t *data, int len, uint32_t key[4])
{
	uint32_t* dataEnd = data + len;
	while (data < dataEnd)
	{
		decryptBlock(CRYPT_ROUND, (uint32_t*)data, key);
		uint32_t val = *data;
		for (int i = 0; i<4; ++i)
		{
			key[i] ^= val;//还原出第一次内容后和key进行异或，生成第二次解密秘钥
		}
		data += 2;
	}
}


//4个字节的长度
struct DataHeader {
	uint8_t  seed[2];	//随机数
	uint8_t  bcc;		//校验数据
	uint8_t  padding;	//表示补充数据的长度
};

static uint32_t bcc(const uint8_t* data, int dataLen)
{
	uint32_t val = 0x4f9d0db8;
	int i;
	for (i = 0; i<dataLen; i += 4)
	{
		val ^= *(uint32_t*)(data + i);
	}

	if (i != dataLen)
	{
		uint32_t temp = 0;
		for (i = i - 4; i<dataLen; ++i)
		{
			temp = temp << 8 | data[i];
		}
		val ^= temp;
	}
	return val;
}

inline int getUpValue8(int len)
{
	if ((len & 0x7) == 0)
		return len;
	else
		return (len & 0xffffff8) + 8;
}


//包加密算法
int PacketEncrypt(const uint8_t* data, int dataLen, unsigned char* dataOut, uint32_t const key[4])
{
	DataHeader* header = (DataHeader*)dataOut;
	header->seed[0] = (uint8_t)(rand() & 0xff);
	header->seed[1] = (uint8_t)(rand() & 0xff);

	header->bcc = (uint8_t)bcc(data, dataLen);

	int padding = getUpValue8(dataLen) - dataLen;
	if (dataLen + padding <16)
	{
		padding = 16 - dataLen;
	}

	header->padding = padding;

	//根据seed计算新密钥
	uint32_t newKey[4];
	for (int i = 0; i<4; ++i)
	{
		newKey[i] = key[i] ^ header->seed[0] ^ (header->seed[1] << 8);
	}

	//拷贝数据
	memcpy((dataOut + PACKET_OVERHEAD), data, dataLen);

	//填充随机数据
	for (int i = 0; i<header->padding; ++i)
	{
		dataOut[PACKET_OVERHEAD + dataLen + i] = (uint8_t)rand();
	}
	encryptData((uint32_t*)(dataOut + PACKET_OVERHEAD), (dataLen + padding)/4, newKey);
	return dataLen + PACKET_OVERHEAD + padding;
}


int PacketDecrypt(const uint8_t* data, int dataLen, unsigned char* dataOut, uint32_t const key[4])
{
	if (dataLen < (int)PACKET_OVERHEAD)
		return -1;

	if (((dataLen - PACKET_OVERHEAD) & 0x7) != 0)
		return -1;

	DataHeader* header = (DataHeader*)data;
	uint32_t newKey[4];
	for (int i = 0; i<4; ++i)
	{
		newKey[i] = key[i] ^ header->seed[0] ^ (header->seed[1] << 8);
	}

	//对data进行解密
	memcpy(dataOut, data + PACKET_OVERHEAD, dataLen - PACKET_OVERHEAD);
	decryptData((uint32_t*)dataOut, (dataLen - PACKET_OVERHEAD)/4, newKey);

	int outLen = dataLen - PACKET_OVERHEAD - header->padding;
	if (outLen>0 && header->bcc == (uint8_t)bcc(dataOut, outLen))
	{
		return outLen;
	}
	else
	{
		return -1;
	}
}




int RandomPacketCrypto::Encrypt(const uint8_t* data, int dataLen, unsigned char* dataOut) {
	uint32_t key[] = ENCRYPT_KEY;
	return PacketEncrypt(data, dataLen, dataOut,key);
}
int RandomPacketCrypto::Decrypt(const uint8_t* data, int dataLen, unsigned char* dataOut) {
	uint32_t key[] = ENCRYPT_KEY;
	return PacketDecrypt(data, dataLen, dataOut,key);
}


}//namespace

