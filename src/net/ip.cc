#include <core/exception.hh>
#include <net/ip.hh>

#include <cstring>
#include <memory>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace izumo::net {
    struct address_info_exception: std::runtime_error {
	address_info_exception(int error):
	    std::runtime_error(gai_strerror(error))
	{}
    };

    addrinfo*
    izm_getaddrinfo(const char* node, const char* service, const addrinfo* hints = nullptr)
    {
	addrinfo* ret = nullptr;
	auto result = getaddrinfo(node, service, hints, &ret);
	if (result != 0) throw address_info_exception(result);
	return ret;
    }

    void
    ip_sockaddr::get_address(const char* host, const char* port, bool passive,
			     int family, int type)
    {
	addrinfo hints = {
	    .ai_flags = AI_NUMERICSERV,
	    .ai_family = family,
	    .ai_socktype = type
	};

	bool passive_localhost = false;
	if (passive && std::strcmp(host, "localhost") == 0) {
	    host = nullptr;
	    passive_localhost = true;
	}

	if (passive) hints.ai_flags |= AI_PASSIVE;
	if (passive && host) hints.ai_flags |= AI_NUMERICSERV;
	
	auto addr = izm_getaddrinfo(host, port, &hints);
	// ---NOEXCEPT BEGIN---
	std::memcpy(m_vp(), addr->ai_addr, addr->ai_addrlen);
	freeaddrinfo(addr);
	// ---NOEXCEPT END---
    }
    
    void*
    ip_sockaddr::m_vp()
    {
	return static_cast<void*>(&m_storage);
    }

    const void*
    ip_sockaddr::m_vp() const
    {
	return static_cast<const void*>(&m_storage);
    }
    
    sockaddr*
    ip_sockaddr::addr_ptr() noexcept
    {
	return static_cast<sockaddr*>(m_vp());
    }

    sockaddr_in*
    ip_sockaddr::v4_ptr() noexcept
    {
	return static_cast<sockaddr_in*>(m_vp());
    }

    sockaddr_in6*
    ip_sockaddr::v6_ptr() noexcept
    {
	return static_cast<sockaddr_in6*>(m_vp());
    }

    const sockaddr*
    ip_sockaddr::addr_ptr() const noexcept
    {
	return static_cast<const sockaddr*>(m_vp());
    }

    const sockaddr_in*
    ip_sockaddr::v4_ptr() const noexcept
    {
	return static_cast<const sockaddr_in*>(m_vp());
    }

    const sockaddr_in6*
    ip_sockaddr::v6_ptr() const noexcept
    {
	return static_cast<const sockaddr_in6*>(m_vp());
    }

    const char*
    ip_sockaddr::ntoa() const noexcept
    {
	static thread_local char buf[INET6_ADDRSTRLEN];
	
	inet_ntop(family(), m_vp(), buf, size());
	return buf;
    }

    std::uint16_t
    ip_sockaddr::port() const noexcept
    {
	auto ret = family() == AF_INET6 ? v6_ptr()->sin6_port : v4_ptr()->sin_port;
	return ntohs(ret);
    }

    socklen_t
    ip_sockaddr::size() const noexcept
    {
	return sizeof(m_storage);
    }

    sa_family_t
    ip_sockaddr::family() const noexcept
    {
	return m_storage.ss_family;
    }

    ip_sockaddr
    getsockname(int socket)
    {
	ip_sockaddr ret;
	socklen_t len = ret.size();
	auto result = ::getsockname(socket, ret.addr_ptr(), &len);
	if (result != 0) throw core::osexception();
	return ret;
    }
}
