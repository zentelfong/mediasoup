#define MS_CLASS "RTC::CustomTransport"
// #define MS_LOG_DEV

#include "RTC/CustomTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    CustomTransport::CustomTransport(
	  const std::string& id, RTC::Transport::Listener* listener, json& data)
	  : RTC::Transport::Transport(id, listener)
	{
		MS_TRACE();

		auto jsonRtcpMuxIt = data.find("rtcpMux");

		if (jsonRtcpMuxIt != data.end())
		{
			if (!jsonRtcpMuxIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong rtcpMux (not a boolean)");

			this->rtcpMux = jsonRtcpMuxIt->get<bool>();
		}

		auto jsonComediaIt = data.find("comedia");

		if (jsonComediaIt != data.end())
		{
			if (!jsonComediaIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong comedia (not a boolean)");

			this->comedia = jsonComediaIt->get<bool>();
		}

		auto jsonMultiSourceIt = data.find("multiSource");

		if (jsonMultiSourceIt != data.end())
		{
			if (!jsonMultiSourceIt->is_boolean())
				MS_THROW_TYPE_ERROR("wrong multiSource (not a boolean)");

			this->multiSource = jsonMultiSourceIt->get<bool>();

			// If multiSource is set disable RTCP-mux and comedia mode.
			if (this->multiSource)
			{
				this->rtcpMux = false;
				this->comedia = false;
			}
		}
	}

    CustomTransport::~CustomTransport()
	{
		MS_TRACE();
	}

    void CustomTransport::SetSender(CustomSender* sender)
    {
        this->sender = sender;
        if (!IsConnected())
            RTC::Transport::Connected();
    }

	void CustomTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);

		// Add rtcpMux.
		jsonObject["rtcpMux"] = this->rtcpMux;

		// Add comedia.
		jsonObject["comedia"] = this->comedia;

		// Add multiSource.
		jsonObject["multiSource"] = this->multiSource;

		// Add headerExtensionIds.
		jsonObject["rtpHeaderExtensions"] = json::object();
		auto jsonRtpHeaderExtensionsIt    = jsonObject.find("rtpHeaderExtensions");

		if (this->rtpHeaderExtensionIds.mid != 0u)
			(*jsonRtpHeaderExtensionsIt)["mid"] = this->rtpHeaderExtensionIds.mid;

		if (this->rtpHeaderExtensionIds.rid != 0u)
			(*jsonRtpHeaderExtensionsIt)["rid"] = this->rtpHeaderExtensionIds.rid;

		if (this->rtpHeaderExtensionIds.rrid != 0u)
			(*jsonRtpHeaderExtensionsIt)["rrid"] = this->rtpHeaderExtensionIds.rrid;

		if (this->rtpHeaderExtensionIds.absSendTime != 0u)
			(*jsonRtpHeaderExtensionsIt)["absSendTime"] = this->rtpHeaderExtensionIds.absSendTime;

		// Add rtpListener.
		this->rtpListener.FillJson(jsonObject["rtpListener"]);
	}

	void CustomTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		jsonArray.emplace_back(json::value_t::object);
		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "plain-rtp-transport";

		// Add transportId.
		jsonObject["transportId"] = this->id;

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTime();

		// Add rtcpMux.
		jsonObject["rtcpMux"] = this->rtcpMux;

		// Add comedia.
		jsonObject["comedia"] = this->comedia;

		// Add multiSource.
		jsonObject["multiSource"] = this->multiSource;

		// Add bytesReceived.
		jsonObject["bytesReceived"] = RTC::Transport::GetReceivedBytes();

		// Add bytesSent.
		jsonObject["bytesSent"] = RTC::Transport::GetSentBytes();

		// Add recvBitrate.
		jsonObject["recvBitrate"] = RTC::Transport::GetRecvBitrate();

		// Add sendBitrate.
		jsonObject["sendBitrate"] = RTC::Transport::GetSendBitrate();
	}

	inline bool CustomTransport::IsConnected() const
	{
		return this->sender != nullptr;
	}

	void CustomTransport::SendRtpPacket(
	  RTC::RtpPacket* packet, RTC::Consumer* /*consumer*/, bool /*retransmitted*/, bool /*probation*/)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		this->sender->SendRtpPacket(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void CustomTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

        this->sender->SendRtcpPacket(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void CustomTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

        this->sender->SendRtcpPacket(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void CustomTransport::OnPacketRecv(const uint8_t* data, size_t len)
	{
		MS_TRACE();
        if (!this->rtcpMux)
            return;

		// Increase receive transmission.
		RTC::Transport::DataReceived(len);

		// Check if it's RTCP.
		if (RTC::RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataRecv(data, len);
		}
		// Check if it's RTP.
		else if (RTC::RtpPacket::IsRtp(data, len))
		{
			OnRtpDataRecv(data, len);
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

    //收到rtp包，交由producer处理
	inline void CustomTransport::OnRtpDataRecv(const uint8_t* data, size_t len)
	{
		MS_TRACE();
		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Apply the Transport RTP header extension ids so the RTP listener can use them.
		packet->SetMidExtensionId(this->rtpHeaderExtensionIds.mid);
		packet->SetRidExtensionId(this->rtpHeaderExtensionIds.rid);
		packet->SetRepairedRidExtensionId(this->rtpHeaderExtensionIds.rrid);
		packet->SetAbsSendTimeExtensionId(this->rtpHeaderExtensionIds.absSendTime);

		// Get the associated Producer.
		RTC::Producer* producer = this->rtpListener.GetProducer(packet);

		if (producer == nullptr)
		{
			MS_WARN_TAG(
			  rtp,
			  "no suitable Producer for received RTP packet [ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]",
			  packet->GetSsrc(),
			  packet->GetPayloadType());

			delete packet;

			return;
		}

		// MS_DEBUG_DEV(
		//   "RTP packet received [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", producerId:%s]",
		//   packet->GetSsrc(),
		//   packet->GetPayloadType(),
		//   producer->id.c_str());

		// Pass the RTP packet to the corresponding Producer.
		producer->ReceiveRtpPacket(packet);

		delete packet;
	}

	inline void CustomTransport::OnRtcpDataRecv(const uint8_t* data, size_t len)
	{
		MS_TRACE();
		RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

			return;
		}

		// Handle each RTCP packet.
		while (packet != nullptr)
		{
			ReceiveRtcpPacket(packet);

			RTC::RTCP::Packet* previousPacket = packet;

			packet = packet->GetNext();
			delete previousPacket;
		}
	}

	void CustomTransport::UserOnNewProducer(RTC::Producer* /*producer*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void CustomTransport::UserOnNewConsumer(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();
	}

	void CustomTransport::UserOnRembFeedback(RTC::RTCP::FeedbackPsRembPacket* /*remb*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	inline void CustomTransport::OnConsumerNeedBitrateChange(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();

		// Do nothing.
	}


} // namespace RTC
