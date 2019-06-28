#ifndef MS_UNIX_STREAM_SOCKET_HPP
#define MS_UNIX_STREAM_SOCKET_HPP

#include "common.hpp"
#include <uv.h>
#include <string>

class PipeStreamSocket
{
public:
	/* Struct for the data field of uv_req_t when writing data. */
	struct UvWriteData
	{
		uv_write_t req;
		uint8_t store[1];
	};

public:
    PipeStreamSocket(int fd, size_t bufferSize);
    PipeStreamSocket& operator=(const PipeStreamSocket&) = delete;
    PipeStreamSocket(const PipeStreamSocket&)            = delete;
	virtual ~PipeStreamSocket();

public:
	void Close();
	bool IsClosed() const;
	void Write(const uint8_t* data, size_t len);
	void Write(const std::string& data);

	/* Callbacks fired by UV events. */
public:
	void OnUvReadAlloc(size_t suggestedSize, uv_buf_t* buf);
	void OnUvRead(ssize_t nread, const uv_buf_t* buf);
	void OnUvWriteError(int error);

	/* Pure virtual methods that must be implemented by the subclass. */
protected:
	virtual void UserOnStreamRead() = 0;
	virtual void UserOnStreamSocketClosed(bool isClosedByPeer) = 0;

private:
	// Allocated by this.
	uv_pipe_t* uvHandle{ nullptr };
	// Others.
	bool closed{ false };
	bool isClosedByPeer{ false };
	bool hasError{ false };

protected:
	// Passed by argument.
	size_t bufferSize{ 0 };
	// Allocated by this.
	uint8_t* buffer{ nullptr };
	// Others.
	size_t bufferDataLen{ 0 };
};

/* Inline methods. */

inline bool PipeStreamSocket::IsClosed() const
{
	return this->closed;
}

inline void PipeStreamSocket::Write(const std::string& data)
{
	Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
}

#endif
