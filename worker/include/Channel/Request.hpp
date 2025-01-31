#ifndef MS_CHANNEL_REQUEST_HPP
#define MS_CHANNEL_REQUEST_HPP

#include "common.hpp"
#include "json.hpp"
#include <string>
#include <unordered_map>

using json = nlohmann::json;

namespace Channel
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class ChannelBase;

	class Request
	{
	public:
		enum class MethodId
		{
			WORKER_DUMP = 1,
			WORKER_UPDATE_SETTINGS,
			WORKER_CREATE_ROUTER,
			ROUTER_CLOSE,
			ROUTER_DUMP,
			ROUTER_CREATE_WEBRTC_TRANSPORT,
			ROUTER_CREATE_PLAIN_RTP_TRANSPORT,
			ROUTER_CREATE_PIPE_TRANSPORT,
			ROUTER_CREATE_AUDIO_LEVEL_OBSERVER,
			TRANSPORT_CLOSE,
			TRANSPORT_DUMP,
			TRANSPORT_GET_STATS,
			TRANSPORT_CONNECT,
			TRANSPORT_SET_MAX_INCOMING_BITRATE,
			TRANSPORT_RESTART_ICE,
			TRANSPORT_PRODUCE,
			TRANSPORT_CONSUME,
			PRODUCER_CLOSE,
			PRODUCER_DUMP,
			PRODUCER_GET_STATS,
			PRODUCER_PAUSE,
			PRODUCER_RESUME,
			CONSUMER_CLOSE,
			CONSUMER_DUMP,
			CONSUMER_GET_STATS,
			CONSUMER_PAUSE,
			CONSUMER_RESUME,
			CONSUMER_SET_PREFERRED_LAYERS,
			CONSUMER_REQUEST_KEY_FRAME,
			RTP_OBSERVER_CLOSE,
			RTP_OBSERVER_PAUSE,
			RTP_OBSERVER_RESUME,
			RTP_OBSERVER_ADD_PRODUCER,
			RTP_OBSERVER_REMOVE_PRODUCER
		};

	private:
		static std::unordered_map<std::string, MethodId> string2MethodId;

	public:
		Request(Channel::ChannelBase* channel, json& jsonRequest);
		virtual ~Request();

		void Accept();
		void Accept(json& data);
		void Error(const char* reason = nullptr);
		void TypeError(const char* reason = nullptr);

	public:
		// Passed by argument.
		Channel::ChannelBase* channel{ nullptr };
		uint32_t id{ 0u };
		std::string method;
		MethodId methodId;
		json internal;
		json data;
		// Others.
		bool replied{ false };
	};
} // namespace Channel

#endif
