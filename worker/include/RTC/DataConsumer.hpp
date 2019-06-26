#ifndef MS_RTC_DATA_CONSUMER_HPP
#define MS_RTC_DATA_CONSUMER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/SctpDictionaries.hpp"
#include <string>

namespace RTC
{
	class DataConsumer
	{
	public:
		class Listener
		{
		public:
			virtual void OnDataConsumerSendSctpData(RTC::DataConsumer* dataConsumer, const uint8_t* data, size_t len) = 0;
			virtual void onDataConsumerDataProducerClosed(RTC::DataConsumer* dataConsumer) = 0;
		};

	public:
		DataConsumer(const std::string& id, RTC::DataConsumer::Listener* listener, json& data);
		virtual ~DataConsumer();

	public:
		void FillJson(json& jsonObject) const;
		void FillJsonStats(json& jsonArray) const;
		void HandleRequest(Channel::Request* request);
		const RTC::SctpStreamParameters& GetSctpStreamParameters() const;
		bool IsActive() const;
		void TransportConnected();
		void TransportDisconnected();
		void DataProducerClosed();

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		RTC::DataConsumer::Listener* listener{ nullptr };
		// Others.
		RTC::SctpStreamParameters sctpStreamParameters;
		bool transportConnected{ false };
		bool dataProducerClosed{ false };
	};

	/* Inline methods. */

	inline const RTC::SctpStreamParameters& DataConsumer::GetSctpStreamParameters() const
	{
		return this->sctpStreamParameters;
	}

	inline bool DataConsumer::IsActive() const
	{
		return (this->transportConnected && !this->dataProducerClosed);
	}
} // namespace RTC

#endif
