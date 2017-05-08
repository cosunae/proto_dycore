cmake_policy(SET CMP0048 NEW)
project(proto_dycore VERSION "0.1")
cmake_minimum_required(VERSION 3.0)
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(GridTools REQUIRED)
file(GLOB_RECURSE headers "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" )

# ===============
# library header files
# ===============
include_directories ( "${GRIDTOOLS_INCLUDE_DIR}" )
include_directories ( "${GRIDTOOLS_INCLUDE_DIR}/../tools/gridtools_storage/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package( Boost 1.58 COMPONENTS timer system chrono REQUIRED )

if(Boost_FOUND)
    include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
    set(exe_LIBS "${Boost_LIBRARIES}" "${exe_LIBS}")
endif()

add_executable(proto_dycore main.cpp repository.cpp field_pool.cpp ${headers})
target_link_libraries(proto_dycore ${exe_LIBS})
