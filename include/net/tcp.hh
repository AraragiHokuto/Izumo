#ifndef IZUMO_NET_TCP_HH_
#define IZUMO_NET_TCP_HH_

#include <core/byte_buffer.hh>
#include <net/ip.hh>

#include <cstdint>
#include <utility>

namespace izumo::net {
    int bind_tcp(ip_sockaddr& addr, int backlog);

    std::pair<int, ip_sockaddr> accept(int listen_fd);

    std::size_t recv(int fd, core::byte_buffer_writer& writer);
    std::size_t send(int fd, core::byte_buffer_view& view);
}

#endif	// IZUMO_NET_TCP_HH_
