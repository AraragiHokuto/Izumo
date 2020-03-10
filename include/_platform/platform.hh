// platform.hh -- platform detection and macro defs
#ifndef IZUMO_PLATFORM_PLATFORM_HH_
#define IZUMO_PLATFORM_PLATFORM_HH_

#if defined(__GNUC__)
#define IZUMO_PLATFORM_COMPILER_GCC_COMPATIBLE 1
#define IZUMO_PLATFORM_COMPILER_GCC 1
#endif	// __GNUC__

#if defined (__clang__)
#define IZUMO_PLATFORM_COMPILER_GCC_COMPATIBLE 1
#define IZUMO_PLATFORM_COMPILER_CLANG 1
#undef IZUMO_PLATFORM_COMPILER_GCC
#endif	// __clang__

#if defined(__linux__)
#define IZUMO_PLATFORM_OS_POSIX 1
#define IZUMO_PLATFORM_OS_LINUX 1
#endif	// __linux__

namespace izumo::_platform {
    namespace os {
	#if IZUMO_PLATFORM_OS_POSIX
	constexpr static bool posix = true;
	#else
	constexpr static bool posix = false;
	#endif	// IZUMO_PLATFORM_OS_POSIX

	#if IZUMO_PLATFORM_OS_LINUX
	constexpr static bool linux = true;
	#else
	constexpr static bool linux = false;
	#endif	// IZUMO_PLATFORM_OS_LINUX;
    }
    namespace compiler {
	#if IZUMO_PLATFORM_COMPILER_GCC_COMPATIBLE
	constexpr static bool gcc_compatible = true;
	#else
	constexpr static bool gcc_compatible = false;
	#endif	// IZUMO_PLATFORM_COMPILER_GCC_COMPATIBLE

	#if IZUMO_PLATFORM_COMPILER_GCC
	constexpr static bool gcc = true;
	#else
	constexpr static bool gcc = false;
	#endif	// IZUMO_PLATFORM_COMPILER_GCC

	#if IZUMO_PLATFORM_COMPILER_CLANG
	constexpr static bool clang = true;
	#else
	constexpr static bool clang = false;
	#endif	// IZUMO_PLATFORM_COMPILER_CLANG
    }
}

#endif	// IZUMO_PLATFORM_PLATFORM_HH_
