#ifndef IZUMO_NET_IP_HH_
#define IZUMO_NET_IP_HH_

#include <cassert>
#include <cstdint>
#include <string_view>

#include <netinet/ip.h>

namespace izumo::net {
    class ip_sockaddr {
    private:
	sockaddr_storage m_storage;

	void* m_vp();
	const void* m_vp() const;

    public:
	ip_sockaddr() = default;
	ip_sockaddr(const in6_addr& addr, std::uint16_t port);
	ip_sockaddr(std::uint32_t addr, std::uint16_t port);

	sockaddr* addr_ptr() noexcept;
	sockaddr_in* v4_ptr() noexcept;
	sockaddr_in6* v6_ptr() noexcept;
	const sockaddr* addr_ptr() const noexcept;
	const sockaddr_in* v4_ptr() const noexcept;
	const sockaddr_in6* v6_ptr() const noexcept;

	void set(const in6_addr& addr);
	void set(std::uint32_t addr);
	void set(std::uint16_t port);

	void get_address(const char* host, const char* port, bool passive,
			 int family = AF_UNSPEC, int type = 0);

	const char* ntoa() const noexcept;
	std::uint16_t port() const noexcept;

	socklen_t size() const noexcept;
	sa_family_t family() const noexcept;
    };

    ip_sockaddr getsockname(int socket);
}

#endif	// IZUMO_NET_IP_HH_
