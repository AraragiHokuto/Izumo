#include <core/exception.hh>
#include <net/tcp.hh>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <buildconfig.h>

namespace izumo::net {
    int
    bind_tcp(ip_sockaddr& addr, int backlog = 0)
    {
	int sock = socket(addr.family(), SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sock < 0) throw core::osexception();

	int val = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
	    close(sock);
	    throw core::osexception();
	}

	if (bind(sock, addr.addr_ptr(), addr.size()) < 0) {
	    close(sock);
	    throw core::osexception();
	}

	if (listen(sock, backlog) < 0) {
	    close(sock);
	    throw core::osexception();
	}

	return sock;
    }

    std::pair<int, ip_sockaddr>
    accept(int listen_fd)
    {
	ip_sockaddr ret_addr;
	socklen_t len;
	int ret_sock;
#if defined(IZM_HAVE_ACCEPT4)
	ret_sock = accept4(listen_fd, ret_addr.addr_ptr(), &len, SOCK_NONBLOCK);
	if (ret_sock < 0) {
	    if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return { -1, {} };
	    }
	    throw core::osexception();
	}
#else
	ret_sock = accept(listen_fd, ret_addr.addr_ptr(), &len);
	if (ret_sock < 0) {
	    if (errno == EGAGIN || errno == EWOULDBLOCK) {
		return { -1, {} };
	    }
	    throw core::osexception();
	}

	auto flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
	    close(ret_sock);
	    throw core::osexception();
	}

	flags |= O_NONBLOCK;
	if (fcntl(ret_sock, F_SETFL, flags) < 0) {
	    close(ret_sock);
	    throw core::osexception();
	}
#endif
	return {ret_sock, ret_addr};
    }

    std::size_t
    recv(int fd, core::byte_buffer_writer& writer)
    {
	std::size_t ret = 0;
	while (writer.space()) {
	    auto recvret = ::recv(fd, writer.current(), writer.space(), MSG_NOSIGNAL);
	    if (recvret < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
		    break;
		}
		throw core::osexception();
	    }

	    if (recvret == 0) {
		// XXX: return value on closed connection?
		break;
	    }

	    ret += recvret;
	    writer.move_current(recvret);
	}

	return ret;
    }

    std::size_t
    send(int fd, core::byte_buffer_view& view)
    {
	std::size_t ret = 0;
	while (view.size()) {
	    auto sendret = ::send(fd, view.data(), view.size(), MSG_NOSIGNAL);
	    if (sendret < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
		    break;
		}
		throw core::osexception();
	    }

	    if (sendret == 0) {
		break;
	    }

	    ret += sendret;
	    view = view.slice(sendret, view.size());
	} 

	return ret;
    }
}
