#include <core/ev_loop.hh>
#include <core/exception.hh>

#include <map>

#include <unistd.h>
#include <sys/epoll.h>

#include "ev_loop.in.cc"

namespace izumo::core {
    using timeout_queue_t = std::multimap<timestamp_ms_t, ev_watcher*>;
    
    class ev_loop_epoll : public ev_loop {
	int m_epfd;
	timeout_queue_t m_timeout_queue = timeout_queue_t();
    
    public:
	ev_loop_epoll();
	~ev_loop_epoll();
  
	void add_watcher(ev_watcher &watcher) override;
	void remove_watcher(ev_watcher &watcher) override;
    
	void add_timer(ev_watcher &watcher, timedelta_ms_t timeout) override;

	void run_once() override;
    };

    ev_loop_epoll::ev_loop_epoll() {
	int epfd = epoll_create1(EPOLL_CLOEXEC);
	if (!epfd) {
	    throw osexception();
	}

	m_epfd = epfd;
    }

    ev_loop_epoll::~ev_loop_epoll() { close(m_epfd); }

    void ev_loop_epoll::add_watcher(ev_watcher &watcher) {
	epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.ptr = &watcher;

	if (epoll_ctl(m_epfd, EPOLL_CTL_ADD, watcher.fd(), &ev) < 0) {
	    throw osexception();
	}
    }

    void ev_loop_epoll::remove_watcher(ev_watcher &watcher) {
	// for portability before 2.6.9
	epoll_event ev;

	// XXX: should ENOENT be ignored here?
	if (epoll_ctl(m_epfd, EPOLL_CTL_DEL, watcher.fd(), &ev) < 0 &&
	    errno != ENOENT) {
	    throw osexception();
	}

	// this will become .erase_if() once we embrace C++20
	// (or implement out own priority queue)
	for (auto& i: m_timeout_queue) {
	    if (i.second == &watcher) {
		i.second = nullptr;
	    }
	}
    }

    void ev_loop_epoll::add_timer(ev_watcher &watcher, timedelta_ms_t timeout) {
	assert(timeout > 0);
	m_timeout_queue.emplace(clock::now() + timeout, &watcher);
    }

    void ev_loop_epoll::run_once() {
	epoll_event evs[128];

	int timeout = -1;
	if (m_timeout_queue.size() != 0) {
	    auto now = clock::now();
	    timeout = std::max(static_cast<int>(m_timeout_queue.begin()->first - now),
			       0);
	}
    
	int ret = epoll_wait(m_epfd, evs, 128, timeout);

	if (ret < 0) {
	    if (errno != EINTR)
		throw osexception();
	    return;
	}

	// timer events
	auto now = clock::now();
	auto i = m_timeout_queue.begin();
	while (i != m_timeout_queue.end() && i->first <= now) {
	    auto w = (i++)->second;
	    if (!w) continue;
	    w->on_timeout();
	}

	m_timeout_queue.erase(m_timeout_queue.begin(), i);

	ev_watcher *defers[128];
	std::size_t defers_count = 0;

	for (int i = 0; i < ret; ++i) {
	    auto &ev = evs[i];
	    auto w = static_cast<ev_watcher *>(ev.data.ptr);
	    auto do_defer = w->on_event(ev.events & EPOLLIN, ev.events & EPOLLOUT);

	    if (do_defer) {
		defers[defers_count++] = w;
	    }
	}

	for (std::size_t i = 0; i < defers_count; ++i) {
	    auto w = defers[i];
	    w->on_deferred();
	}
    }
}

DEFINE_IMPL(epoll, izumo::core::ev_loop_epoll);
