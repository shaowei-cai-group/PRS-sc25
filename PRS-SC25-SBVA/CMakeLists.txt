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

# 查找kissat源代码文件
file(GLOB_RECURSE KISSAT_SRC_FILES "${KISSAT_SRC_DIR}/src/*.c" "${KISSAT_SRC_DIR}/src/*.cpp")
# 查找yalsat源代码文件
file(GLOB YALSAT_SRC_FILES "${YALSAT_SRC_DIR}/yals.c" "${YALSAT_SRC_DIR}/config.c")

# 使用 ExternalProject 来管理第三方库
include(ExternalProject)

# 配置 kissat
ExternalProject_Add(
    kissat
    BUILD_IN_SOURCE 1
    WORKING_DIRECTORY ${KISSAT_SRC_DIR}
    SOURCE_DIR ${KISSAT_SRC_DIR}
    CONFIGURE_COMMAND cd ${KISSAT_SRC_DIR} && ./configure --quiet
    BUILD_COMMAND cd ${KISSAT_SRC_DIR} && make -j
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${KISSAT_STATIC_LIB}
)

# 配置 m4ri
ExternalProject_Add(
    m4ri
    BUILD_IN_SOURCE 1
    WORKING_DIRECTORY ${M4RI_SRC_DIR}
    SOURCE_DIR ${M4RI_SRC_DIR}
    CONFIGURE_COMMAND ./configure --prefix=${M4RI_SRC_DIR}/install
    BUILD_COMMAND make -j
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS ${M4RI_STATIC_LIB}
)

# 配置 yalsat
ExternalProject_Add(
    yalsat
    BUILD_IN_SOURCE 1
    WORKING_DIRECTORY ${YALSAT_SRC_DIR}
    SOURCE_DIR ${YALSAT_SRC_DIR}
    CONFIGURE_COMMAND cd ${YALSAT_SRC_DIR} && ./configure.sh
    BUILD_COMMAND cd ${YALSAT_SRC_DIR} && make libyals.a
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${YALSAT_STATIC_LIB}
)

# 设置 kissat 静态库的输出目录
set(KISSAT_STATIC_LIB "${KISSAT_SRC_DIR}/build/libkissat.a")
# 设置 m4ri 静态库的输出目录
set(M4RI_STATIC_LIB "${M4RI_SRC_DIR}/install/lib/libm4ri.a")
# 设置 yalsat 静态库的输出目录
set(YALSAT_STATIC_LIB "${YALSAT_SRC_DIR}/libyals.a")

# kissat 的重新编译触发器
add_custom_command(
    OUTPUT ${KISSAT_STATIC_LIB}
    COMMAND cd ${KISSAT_SRC_DIR} && ./configure --quiet && make -j
    DEPENDS ${KISSAT_SRC_FILES}
    WORKING_DIRECTORY ${KISSAT_SRC_DIR}
    COMMENT "Building kissat library"
    VERBATIM
)

# m4ri 的重新编译触发器
add_custom_command(
    OUTPUT ${M4RI_STATIC_LIB}
    COMMAND make -C ${M4RI_SRC_DIR} -j
    WORKING_DIRECTORY ${M4RI_SRC_DIR}
    COMMENT "Building m4ri library"
    VERBATIM
)

# yalsat 的重新编译触发器
add_custom_command(
    OUTPUT ${YALSAT_STATIC_LIB}
    COMMAND cd ${YALSAT_SRC_DIR} && ./configure.sh && make libyals.a
    DEPENDS ${YALSAT_SRC_FILES}
    WORKING_DIRECTORY ${YALSAT_SRC_DIR}
    COMMENT "Building yalsat library"
    VERBATIM
)

# 添加静态库目标
add_custom_target(kissat_static
    DEPENDS ${KISSAT_STATIC_LIB}
)

add_custom_target(m4ri_static
    DEPENDS ${M4RI_STATIC_LIB}
)

add_custom_target(yalsat_static
    DEPENDS ${YALSAT_STATIC_LIB}
)

# 添加顺序依赖关系
add_dependencies(m4ri_static kissat_static)
add_dependencies(yalsat_static m4ri_static)

# 包含库的头文件
include_directories("${KISSAT_SRC_DIR}/src")
include_directories("${M4RI_SRC_DIR}/install/include")
include_directories("${YALSAT_SRC_DIR}")

# create main executable
add_executable(prs ${SOURCES})

# 确保prs是最后一个构建的
add_dependencies(prs kissat_static m4ri_static yalsat_static)
target_link_libraries(prs ${KISSAT_STATIC_LIB} ${M4RI_STATIC_LIB} ${YALSAT_STATIC_LIB} Threads::Threads)
