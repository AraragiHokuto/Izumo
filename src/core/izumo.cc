#include <iostream>
#include <cstring>

#include <core/ev_watcher.hh>
#include <core/ev_loop.hh>
#include <core/clock.hh>
#include <core/exception.hh>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include <array>

int
bind_listen_sock(uint16_t port)
{
    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock < 0) throw izumo::core::osexception();

    int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
	throw izumo::core::osexception();
    }

    listen(sock, 128);
    return sock;
}

class client: public izumo::core::ev_watcher {
private:
    constexpr static std::size_t BUFSIZE = 4096;
    std::array<char, BUFSIZE> m_buffer;
    std::size_t m_rp = 0, m_wp = 0;
    
public:
    client(int fd): ev_watcher(fd) {}

    bool
    on_event(bool r, bool w) override
    {
	int ret;
	while (true) {
	    ret = recv(m_fd, m_buffer.data() + m_rp, BUFSIZE - m_rp, MSG_NOSIGNAL);
	    if (ret < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
		    throw izumo::core::osexception();
		}
		break;
	    }

	    if (ret == 0) {
		shutdown(m_fd, SHUT_RDWR);
		close(m_fd);
		delete this;
		return false;
	    }

	    m_rp += ret;
	    if (m_rp == BUFSIZE)
		m_rp = 0;
	}

	while (true) {
	    if (m_wp == m_rp) break; // nothing to write

	    auto end = m_rp < m_wp ? BUFSIZE : m_rp;

	    ret = send(m_fd, m_buffer.data() + m_wp, end - m_wp, MSG_NOSIGNAL);
	    if (ret < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
		    throw izumo::core::osexception();
		}
		break;
	    }

	    if (ret == 0) {
		shutdown(m_fd, SHUT_RDWR);
		close(m_fd);
		delete this;
		return false;
	    }

	    m_wp += ret;
	    if (m_wp == BUFSIZE)
		m_wp = 0;
	}

	return false;
    }
};

class acceptor: public izumo::core::ev_watcher {
private:
    std::array<int, 128> m_queue;
    std::size_t m_qp = 0;
    
public:
    acceptor(uint16_t port): ev_watcher(bind_listen_sock(port)) {}
    
    bool
    on_event(bool r, bool) override
    {
	if (!r) return false;

	bool do_defer = false;

	while (true) {
	    int ret = accept4(m_fd, nullptr, 0, SOCK_NONBLOCK);

	    if (ret < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
		    throw izumo::core::osexception();
		}
		break;
	    }

	    do_defer = true;
	    m_queue[m_qp++] = ret;
	    if (m_qp == 128) break;
	}

	return do_defer;
    }

    void
    on_deferred() override {
	for (std::size_t i = 0; i < m_qp; ++i) {
	    auto c = new client(m_queue[i]);
	    izumo::core::ev_loop::instance().add_watcher(*c);
	}

	m_qp = 0;
    }
};

int
main()
{
    acceptor ac(13445);

    auto& loop = izumo::core::ev_loop::instance();
    loop.add_watcher(ac);
    loop.run_forever();
}
