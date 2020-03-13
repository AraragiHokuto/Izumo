cmake_minimum_required(VERSION 3.10)
project(Izumo VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  )

file(GLOB srcs
  "${PROJECT_SOURCE_DIR}/include/*/*.h"
  "${PROJECT_SOURCE_DIR}/src/*/*.cc"
  )

add_executable(izumo ${srcs})

find_package(fmt)
target_link_libraries(izumo fmt::fmt)
