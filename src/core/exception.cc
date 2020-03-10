#include <core/exception.hh>

#include <cstring>
#include <errno.h>

/* unfortunately XSI-compliant and GNU-specific
 * strerror_r have different sematics
 * so we need to wrap it to ensure consistent behaviour */
static const char* izm_strerror(int errsv)
{
    static thread_local char buf[512];
    std::size_t buflen = sizeof(buf) / sizeof(char);
    
#if (_POSIX_C_SOURCE >= 200112L) && ! _GNU_SOURCE
    // XSI-compliant strerror_r
    strerror_r(errsv, buf, buflen);
    return buf;
#else
    // GNU-specific strerror_r
    return strerror_r(errsv, buf, buflen);
#endif
}

namespace izumo::core {
    osexception::osexception(int errsv):
	std::runtime_error(izm_strerror(errsv))
    {}

    osexception::osexception():
	osexception(errno)
    {}
}
