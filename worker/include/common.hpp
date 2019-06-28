#ifndef MS_COMMON_HPP
#define MS_COMMON_HPP

#include <algorithm>    // std::transform(), std::find(), std::min(), std::max()
#include <cinttypes>    // PRIu64, etc
#include <cstddef>      // size_t
#include <cstdint>      // uint8_t, etc
#include <memory>       // std::addressof()

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>
#include <ws2ipdef.h>
#include <intrin.h>

inline unsigned int __builtin_popcount(unsigned int n) {
    return __popcnt(n);
}

inline unsigned int __builtin_popcount(unsigned short n) {
    return __popcnt16(n);
}

#else
#include <arpa/inet.h>  // htonl(), htons(), ntohl(), ntohs()
#include <netinet/in.h> // sockaddr_in, sockaddr_in6
#include <sys/socket.h> // struct sockaddr, struct sockaddr_storage, AF_INET, AF_INET6
#endif

#endif
