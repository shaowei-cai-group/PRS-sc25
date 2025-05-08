cmake_minimum_required(VERSION 3.15)

project(prs)

# compiler options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -march=native -flto -static")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")

# 设置本地 Eigen 库路径
set(EIGEN3_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/eigen-3.4.0")
message(STATUS "Using local Eigen: ${EIGEN3_INCLUDE_DIR}")

# all source code
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${EIGEN3_INCLUDE_DIR})
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c" "src/*.hpp" "src/*.h")

# pthread
find_package(Threads REQUIRED)

# 设置第三方库路径
# set(KISSAT_SRC_DIR "${CMAKE_SOURCE_DIR}/third_party/kissat-rel-4.0.0")
set(KISSAT_SRC_DIR "${CMAKE_SOURCE_DIR}/third_party/kissat-inc")
set(M4RI_SRC_DIR "${CMAKE_SOURCE_DIR}/third_party/m4ri-20140914")
set(YALSAT_SRC_DIR "${CMAKE_SOURCE_DIR}/third_party/yalsat")

message(STATUS "KISSAT_SRC_DIR: ${KISSAT_SRC_DIR}")
message(STATUS "M4RI_SRC_DIR: ${M4RI_SRC_DIR}")
message(STATUS "YALSAT_SRC_DIR: ${YALSAT_SRC_DIR}")

# 设置预编译库的路径
set(KISSAT_STATIC_LIB "${KISSAT_SRC_DIR}/build/libkissat.a")
set(M4RI_STATIC_LIB "${M4RI_SRC_DIR}/.libs/libm4ri.a")
set(YALSAT_STATIC_LIB "${YALSAT_SRC_DIR}/libyals.a")

# 包含库的头文件
include_directories("${KISSAT_SRC_DIR}/src")
include_directories("${M4RI_SRC_DIR}/install/include")
include_directories("${YALSAT_SRC_DIR}")

# create main executable
add_executable(prs ${SOURCES})

# 链接预编译的静态库
target_link_libraries(prs ${KISSAT_STATIC_LIB} ${M4RI_STATIC_LIB} ${YALSAT_STATIC_LIB} Threads::Threads)
