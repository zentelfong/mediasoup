#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "Channel/ChannelBase.hpp"
#include "RTC/Router.hpp"
#include "handles/SignalsHandler.hpp"
#include <string>
#include <unordered_map>

using json = nlohmann::json;

class Worker : public Channel::ChannelBase::Listener, public SignalsHandler::Listener
{
public:
	explicit Worker(Channel::ChannelBase* channel);
	~Worker();

    //Ìí¼Óapi²Ù×÷
    RTC::Router* CreateRouter(const std::string& routerId);
    bool CloseRouter(const std::string& routerId);
    RTC::Router* FindRouter(const std::string& routerId);

private:
	void Close();
	void FillJson(json& jsonObject) const;
	void SetNewRouterIdFromRequest(Channel::Request* request, std::string& routerId) const;
	RTC::Router* GetRouterFromRequest(Channel::Request* request) const;

public:
	void OnChannelRequest(Channel::ChannelBase* channel, Channel::Request* request) override;
	void OnChannelRemotelyClosed(Channel::ChannelBase* channel) override;

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

private:
	// Passed by argument.
	Channel::ChannelBase* channel{ nullptr };
	// Allocated by this.
	SignalsHandler* signalsHandler{ nullptr };
	// Others.
	bool closed{ false };
	std::unordered_map<std::string, RTC::Router*> mapRouters;
};

#endif
