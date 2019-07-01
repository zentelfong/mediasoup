#ifndef MS_RTC_CUSTOM_RTP_TRANSPORT_HPP
#define MS_RTC_CUSTOM_RTP_TRANSPORT_HPP

#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"

namespace RTC
{
    class CustomSender
    {
    public:
        virtual int SendRtpPacket(const uint8_t* data, size_t len) =0;
        virtual int SendRtcpPacket(const uint8_t* data, size_t len) = 0;
    };

    //自定义rtp传输
	class CustomTransport : public RTC::Transport
	{
	public:
        CustomTransport(const std::string& id, RTC::Transport::Listener* listener, json& data);
		~CustomTransport() override;

        void SetSender(CustomSender* sender);

		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) override;

	private:
		bool IsConnected() const override;
		void SendRtpPacket(
		  RTC::RtpPacket* packet,
		  RTC::Consumer* consumer,
		  bool retransmitted = false,
		  bool probation     = false) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;
	private:
		void UserOnNewProducer(RTC::Producer* producer) override;
		void UserOnNewConsumer(RTC::Consumer* consumer) override;
		void UserOnRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb) override;

		/* Pure virtual methods inherited from RTC::Consumer::Listener. */
	public:
		void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) override;

	public:
        //收到包调用
        void OnPacketRecv(const uint8_t* data, size_t len);
        void OnRtpDataRecv(const uint8_t* data, size_t len);
        void OnRtcpDataRecv(const uint8_t* data, size_t len);
	private:
		// Others.
		bool rtcpMux{ true };
		bool comedia{ false };
		bool multiSource{ false };
        CustomSender* sender{ nullptr };
	};
} // namespace RTC

#endif
