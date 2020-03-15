#include <core/ev_loop.hh>

#include <cassert>
#include <unordered_map>

using _ev_loop_get_impl_t = izumo::core::ev_loop& (*)();
using _ev_loop_impl_map_t = std::unordered_map<std::string, _ev_loop_get_impl_t>;

#define IMPL_MAP_DEF							\
    std::unordered_map<std::string, _ev_loop_get_impl_t> _ev_loop_impl_map

extern IMPL_MAP_DEF;

#define CREATE_IMPL_GETTER_(_name, _impl)	\
    static izumo::core::ev_loop&		\
    _get_##_name()				\
    {						\
	static thread_local _impl ret;		\
	return ret;				\
    }

#define ADD_IMPL_(_name, _impl)			\
    static struct _add_##_name##_t {		\
	_add_##_name##_t()			\
	{					\
	    _ev_loop_impl_map.emplace(		\
		#_name, _get_##_name		\
		);				\
	};					\
    } _add_##_name;

#define DEFINE_IMPL(_name, _impl)		\
    CREATE_IMPL_GETTER_(_name, _impl);		\
    ADD_IMPL_(_name, _impl)			\

static izumo::core::ev_loop&
_ev_loop_get_impl_instance(const std::string& name)
{
    assert(_ev_loop_impl_map.find(name) != _ev_loop_impl_map.end());
    return _ev_loop_impl_map[name]();
}
