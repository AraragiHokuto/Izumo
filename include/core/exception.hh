#ifndef IZUMO_CORE_EXCEPTION_HH_
#define IZUMO_CORE_EXCEPTION_HH_

#include <stdexcept>

namespace izumo::core {
    class osexception: public std::runtime_error {
    public:	
	osexception();		// construct osexception with current errno
	osexception(int errsv);	// construct osexception with a saved errno
    };
}

#endif	// IZUMO_CORE_EXCEPTION_HH_
