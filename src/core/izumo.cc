#include <core/ev_watcher.hh>
#include <core/ev_loop.hh>
#include <core/byte_buffer.hh>
#include <core/clock.hh>
#include <core/mem.hh>
#include <core/exception.hh>
#include <core/log.hh>

#include <net/ip.hh>

#include <http/parser.hh>

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
    const char* host = "127.0.0.1";
    const char* port = "12345";
} cmdargs;

static void
usage(const char* cmdname = "izumo")
{
    fmt::print("Usage: {} [-p, --port port]\n", cmdname);
    fmt::print("\t-p, --port port: port number for demo server to listen to\n");
    fmt::print("\t-h, --host host: host address for demo server to listen to\n");
}

static void
parse_opts(int argc, char *argv[])
{
    const char* opts = "h:p:";

    option longopts[] = {
	{ .name = "port", .has_arg = true, .flag = nullptr, .val = 'p' },
	{ .name = "host", .has_arg = true, .flag = nullptr, .val = 'h' }
    };

    auto running = true;
    while (running) {
	switch (getopt_long(argc, argv, opts, longopts, nullptr))
	{
	case 'p':
	    cmdargs.port = optarg;
	    break;
	case 'h':
	    cmdargs.host = optarg;
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

const char* RESPONSE_HEADER =
    "HTTP/1.1 200 OK\r\n"
    "Server: Izumo\r\n"
    "Content-Type: text/plain\r\n\r\n";
const char* RESPONSE_400 =
    "HTTP/1.1 400 Bad Request\r\n"
    "Server: Izumo\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length 15\r\n\r\n"
    "400 Bad Request";

int
bind_listen_sock(izumo::net::ip_sockaddr addr)
{
    int sock = socket(addr.family(), SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock < 0) throw izumo::core::osexception();

    int val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    if (bind(sock, addr.addr_ptr(), addr.size()) != 0) {
	throw izumo::core::osexception();
    }

    listen(sock, 128);
    return sock;
}

class client: public izumo::core::ev_watcher {
private:
    constexpr static std::size_t BUFSIZE = 4096;
    bool m_writing = false;
    izumo::core::byte_buffer m_buffer;
    izumo::core::byte_buffer_writer m_buffer_writer;
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
	m_buffer_writer(m_buffer),
	m_pool(std::move(p))
    {
	izumo::core::log::info("New client: {}:{}",
			       inet_ntoa(m_addr->ipv4.sin_addr),
			       ntohs(m_addr->ipv4.sin_port));
    }

    bool
    on_event(bool r, bool w) override
    {
	if (!m_writing && !r) return false;
	if (m_writing && !w) return false;

	izumo::core::byte_buffer_view read_view;
	while (true) {
	    auto ret = recv(m_fd, m_buffer_writer.current(), m_buffer_writer.space(), MSG_NOSIGNAL);
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
	    
	    m_buffer_writer.append(ret);
	    read_view = m_buffer_writer.to_view();
	    
	    if (izumo::http::header_completed(read_view)) {
		break;
	    }

	    if (!m_buffer_writer.space()) {
		break;
	    }
	}

	m_writing = true;
	try {
	    izumo::http::request req(m_pool);
	    izumo::http::parse_request(req, read_view);
	    
	    auto response = fmt::format("{}{}: {}", RESPONSE_HEADER, req.method, req.target);
	    assert(response.size() < m_buffer.size());
	    m_write_view = izumo::core::byte_buffer_view(m_buffer, response.size());
	    std::memcpy(m_write_view.data(), response.data(), response.size());
	} catch(const izumo::http::bad_request&) {
	    m_write_view = izumo::core::byte_buffer_view(m_buffer, std::strlen(RESPONSE_400));
	    std::memcpy(m_write_view.data(), RESPONSE_400, std::strlen(RESPONSE_400));
	}

	
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
	    
	    m_write_view = m_write_view.slice(ret, m_write_view.size());
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
    acceptor(izumo::net::ip_sockaddr& addr):
	ev_watcher(bind_listen_sock(addr))
    {
	izumo::core::log::info("Listening on {}:{}", cmdargs.host, cmdargs.port);
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
    izumo::net::ip_sockaddr addr;
    addr.get_address(cmdargs.host, cmdargs.port, true, AF_UNSPEC, SOCK_STREAM);
    acceptor ac(addr);

    auto& loop = izumo::core::ev_loop::instance();
    loop.add_watcher(ac);
    loop.run_forever();
}
