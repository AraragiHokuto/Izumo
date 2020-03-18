#include <core/log.hh>
#include <core/ev_watcher.hh>
#include <core/mem.hh>

#include <net/tcp.hh>

using namespace izumo::core;
using namespace izumo::net;

class http_static_handler {
private:
    byte_buffer m_buffer;
    byte_buffer_writer m_writer;
    byte_buffer_view m_view;
    
    mem_pool m_pool;
    mp_unique_ptr<ev_watcher> watcher;

    int local_fd;

public:
    
};
