cmake_minimum_required(VERSION 3.10)
project(Izumo VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
  )

include (CheckSymbolExists)
include (CheckFunctionExists)

# feature testse
check_symbol_exists(epoll_ctl "sys/epoll.h" IZM_HAVE_EPOLL)
check_function_exists(accept4 IZM_HAVE_ACCEPT4)

list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
if (IZM_HAVE_EPOLL) 
  set(IZM_EVLOOP_DEFAULT_IMPL "epoll")
  else()
  set(IZM_EVLOOP_DEFAULT_IMPL "select")
  endif()
  
configure_file(buildconfig.h.in buildconfig.h @ONLY)

file(GLOB srcs
  "${PROJECT_SOURCE_DIR}/include/*/*.h"
  "${PROJECT_SOURCE_DIR}/src/*/*.cc"
  )

add_executable(izumo ${srcs})

find_package(fmt)
target_link_libraries(izumo fmt::fmt)
