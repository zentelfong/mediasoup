#ifndef MS_CHANNEL_UNIX_STREAM_SOCKET_HPP
#define MS_CHANNEL_UNIX_STREAM_SOCKET_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "Channel/ChannelBase.hpp"
#include "handles/PipeStreamSocket.hpp"

namespace Channel
{
    //进程间通讯
	class PipeChannel : public PipeStreamSocket,public ChannelBase
	{
	public:
		class Listener
		{
		public:
			virtual void OnChannelRequest(Channel::PipeChannel* channel, Channel::Request* request) = 0;
			virtual void OnChannelRemotelyClosed(Channel::PipeChannel* channel) = 0;
		};

	public:
		explicit PipeChannel(int fd);

	public:
		void SetListener(Listener* listener);
		void Send(json& jsonMessage);
		void SendLog(char* nsPayload, size_t nsPayloadLen);
		void SendBinary(const uint8_t* nsPayload, size_t nsPayloadLen);

	public:
		void UserOnStreamRead() override;
		void UserOnStreamSocketClosed(bool isClosedByPeer) override;

	private:
        enum {
            NsMessageMaxLen = 65543,
            NsPayloadMaxLen = 65536
        };

		// Passed by argument.
		Listener* listener{ nullptr };
		// Others.
		size_t msgStart{ 0 }; // Where the latest message starts.
        uint8_t writeBuffer[NsMessageMaxLen];
	};
} // namespace Channel

#endif
