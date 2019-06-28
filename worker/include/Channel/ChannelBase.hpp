#ifndef MS_CHANNEL_BASE_HPP
#define MS_CHANNEL_BASE_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"

namespace Channel
{
    //进程间通讯
    class ChannelBase
    {
    public:
        virtual ~ChannelBase() {}
        class Listener
        {
        public:
            virtual void OnChannelRequest(Channel::ChannelBase* channel, Channel::Request* request) = 0;
            virtual void OnChannelRemotelyClosed(Channel::ChannelBase* channel) = 0;
        };

    public:
        virtual void SetListener(Listener* listener) { this->listener = listener; }
        virtual void Send(json& jsonMessage) = 0;
        virtual void SendLog(char* nsPayload, size_t nsPayloadLen) =0;
        virtual void SendBinary(const uint8_t* nsPayload, size_t nsPayloadLen) =0;

    private:
        // Passed by argument.
        Listener* listener{ nullptr };
    };
} // namespace Channel

#endif
