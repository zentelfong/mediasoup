#define MS_CLASS "Logger"
// #define MS_LOG_DEV

#include "Logger.hpp"
#ifdef _WIN32
#include <Windows.h>
#define getpid GetCurrentProcessId
#else
#include <unistd.h> // getpid()
#endif


/* Class variables. */

const int64_t Logger::pid{
    static_cast<int64_t>(getpid()) 
};
Channel::ChannelBase* Logger::channel{ nullptr };
char Logger::buffer[Logger::bufferSize];

/* Class methods. */

void Logger::ClassInit(Channel::ChannelBase* channel)
{
	Logger::channel = channel;

	MS_TRACE();
}
