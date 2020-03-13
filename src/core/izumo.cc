#include <core/ev_watcher.hh>
#include <core/ev_loop.hh>
#include <core/byte_buffer.hh>
#include <core/clock.hh>
#include <core/mem.hh>
#include <core/exception.hh>
#include <core/log.hh>

#include <array>
#include <iostream>
#include <cstring>

#include <fmt/printf.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <getopt.h>

// this file needs to be refactored one day

static struct {
    std::uint16_t port_h = 12345; // port number in host byte order
} cmdargs;

static void
usage(const char* cmdname = "izumo")
{
    fmt::print("Usage: {} [-p, --port port]\n", cmdname);
    fmt::print("\t-p, --port port: port number for demo server to listen to\n");
}

static void
parse_opts(int argc, char *argv[])
{
    const char* opts = "p:";

    option longopts[] = {
	{ .name = "port", .has_arg = true, .flag = nullptr, .val = 'p' }
    };

    auto running = true;
    while (running) {
	switch (getopt_long(argc, argv, opts, longopts, nullptr))
	{
	case 'p':
	    cmdargs.port_h = std::stoul(optarg);
	    break;
	case -1:
	    running = false;
	    break;
	default:
	    usage();
	    std::exit(-1);
	}
    }
}

struct izm_sockaddr {
    union {
	sockaddr untyped;
	sockaddr_in ipv4;
    };
    socklen_t len;
};

const char* RESPONSE = "HTTP/1.1 200 OK\r\nServer: Izumo\r\nContent-Type: text/plain\r\nContent-Length: 10\r\n\r\nIzumo DEMO";

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
    std::size_t m_bytes_read = 0;
    bool m_writing = false;

    izumo::core::byte_buffer m_buffer;
    izumo::core::byte_buffer_view m_write_view;
    izumo::core::mem_pool m_pool;
    izumo::core::mp_unique_ptr<izm_sockaddr> m_addr;

    void
    stop()
    {
	izumo::core::ev_loop::instance().remove_watcher(*this);
	shutdown(m_fd, SHUT_RDWR);
	close(m_fd);
	delete this;
    }
    
public:
    client(int fd, izumo::core::mp_unique_ptr<izm_sockaddr> addr,
	   izumo::core::mem_pool p):
	ev_watcher(fd), m_addr(std::move(addr)),
	m_buffer(BUFSIZE),
	m_pool(std::move(p))
    {
	izumo::core::log::info("New client: {}:{}",
			       inet_ntoa(m_addr->ipv4.sin_addr),
			       ntohs(m_addr->ipv4.sin_port));
    }

    bool
    have_eoh(const izumo::core::byte_buffer_view& view)
    {
	if (view.size() < 4) return false;
	
	for (int i = 0; i < view.size() - 3; ++i) {
	    if (view.slice(i, 4) == std::string_view("\r\n\r\n")) return true;
	}
	return false;
    }

    bool
    on_event(bool r, bool w) override
    {
	if (!m_writing && !r) return false;
	if (m_writing && !w) return false;

	while (true) {
	    auto ret = recv(m_fd, m_buffer.ptr() + m_bytes_read, m_buffer.size() - m_bytes_read, MSG_NOSIGNAL);
	    if (ret < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
		    throw izumo::core::osexception();
		}
		return false;
	    }
	    
	    if (ret == 0) {
		stop();
		return false;
	    }
	    
	    m_bytes_read += ret;
	    if (have_eoh(izumo::core::byte_buffer_view(m_buffer, m_bytes_read))) {
		break;
	    }

	    if (m_bytes_read == m_buffer.size()) {
		break;
	    }
	}

	m_writing = true;
	m_write_view = izumo::core::byte_buffer_view(m_buffer, std::strlen(RESPONSE));
	std::strcpy(reinterpret_cast<char*>(m_write_view.data()), RESPONSE);
	
	while (true) {
	    auto ret = send(m_fd, m_write_view.data(), m_write_view.size(), MSG_NOSIGNAL);
	    if (ret < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
		    throw izumo::core::osexception();
		}
		return false;
	    }

	    if (ret == 0) {
		stop();
		return false;
	    }

	    if (ret == m_write_view.size()) {
		stop();
		return false;
	    }
	    
	    m_write_view = m_write_view.slice(ret, m_write_view.size() - ret);
	}
    }
};

class acceptor: public izumo::core::ev_watcher {
private:
    struct queue_entry {
	int fd;
	izm_sockaddr addr;
    };
    std::array<queue_entry, 128> m_queue;
    std::size_t m_qp = 0;
    
public:
    acceptor(uint16_t port): ev_watcher(bind_listen_sock(port)) {
	izumo::core::log::info("Listening on {}", port);
    }
    
    bool
    on_event(bool r, bool) override
    {
	if (!r) return false;

	bool do_defer = false;

	while (true) {
	    auto& qe = m_queue[m_qp];
	    auto ret = accept4(m_fd, &qe.addr.untyped, &qe.addr.len, SOCK_NONBLOCK);
	    
	    if (ret < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
		    throw izumo::core::osexception();
		}
		break;
	    }
	    qe.fd = ret;

	    do_defer = true;
	    ++m_qp;
	    if (m_qp == 128) break;
	}

	return do_defer;
    }

    void
    on_deferred() override {
	for (std::size_t i = 0; i < m_qp; ++i) {
	    izumo::core::mem_pool p;
	    auto addr = p.make_unique<izm_sockaddr>();
	    *addr = m_queue[i].addr;
	    auto c = new client(m_queue[i].fd, std::move(addr), std::move(p));
	    izumo::core::ev_loop::instance().add_watcher(*c);
	}

	m_qp = 0;
    }
};

int
main(int argc, char *argv[])
{
    parse_opts(argc, argv);
    acceptor ac(cmdargs.port_h);

    auto& loop = izumo::core::ev_loop::instance();
    loop.add_watcher(ac);
    loop.run_forever();
}
