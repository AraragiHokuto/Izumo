#include <core/ev_loop.hh>
#include <core/exception.hh>

#include <queue>

#include <unistd.h>
#include <sys/epoll.h>

#include "ev_loop.in.cc"

namespace izumo::core {
    struct timeout_queue_entry {
	uint64_t deadline;
	ev_watcher* watcher;
    };
    
    constexpr static auto timeout_queue_cmp = [](const timeout_queue_entry& a, const timeout_queue_entry& b)
    {
	return a.deadline > b.deadline;
    };

    using timeout_queue_t = std::priority_queue<timeout_queue_entry,
						std::vector<timeout_queue_entry>,
						decltype(timeout_queue_cmp)>;
    
    class ev_loop_epoll : public ev_loop {
	int m_epfd;
	timeout_queue_t m_timeout_queue = timeout_queue_t(timeout_queue_cmp);
    
	uint64_t m_next_timer_id = 0;
  
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
    }

    void ev_loop_epoll::add_timer(ev_watcher &watcher, timedelta_ms_t timeout) {
	timeout_queue_entry entry;
	entry.deadline = clock::now() + timeout;
	entry.watcher = &watcher;

	this->m_timeout_queue.push(entry);
    }

    void ev_loop_epoll::run_once() {
	epoll_event evs[128];

	int timeout = -1;
	if (m_timeout_queue.size() != 0) {
	    auto now = clock::now();
	    timeout = std::max(static_cast<int>(m_timeout_queue.top().deadline) -
			       static_cast<int>(now),
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
	while (m_timeout_queue.size() && m_timeout_queue.top().deadline <= now) {
	    auto &top = m_timeout_queue.top();
	    top.watcher->on_timeout();
	    m_timeout_queue.pop();
	}

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
