#include <core/ev_watcher.hh>
#include <core/ev_loop.hh>
#include <core/byte_buffer.hh>
#include <core/clock.hh>
#include <core/mem.hh>
#include <core/exception.hh>
#include <core/log.hh>
#include <net/ip.hh>
#include <net/tcp.hh>

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

// tmp workaround

#include "../worker/acceptor.cc.in"

int
main(int argc, char *argv[])
{
    core::logger::get().set_level(core::log_level::debug);
    
    parse_opts(argc, argv);

    izumo::net::ip_sockaddr addr;
    addr.get_address(cmdargs.host, cmdargs.port, true, AF_UNSPEC, SOCK_STREAM);

    auto listen_fd = izumo::net::bind_tcp(addr, 128);
    
    acceptor ac(listen_fd);

    auto& loop = izumo::core::ev_loop::instance();
    ac.add_to_loop(loop);
    
    loop.run_forever();
}
