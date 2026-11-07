
cmake_minimum_required(VERSION 3.21)

project(PrecisionNpcAimFix
    VERSION 0.0.1
    LANGUAGES C CXX
)

# Target Skyrim AE, including 1.7.104.
set(ENABLE_SKYRIM_SE OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Build CommonLibSSE-NG from our pinned source.
add_subdirectory(
    "${CMAKE_CURRENT_LIST_DIR}/_deps/CommonLibSSE-NG"
    "${CMAKE_BINARY_DIR}/commonlib"
)
include("${CMAKE_CURRENT_LIST_DIR}/_deps/CommonLibSSE-NG/cmake/CommonLibSSE.cmake")

file(GLOB_RECURSE PLUGIN_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/*.h"
)

add_commonlibsse_plugin(${PROJECT_NAME}
    SOURCES ${PLUGIN_SOURCES}
)

target_compile_features(${PROJECT_NAME}
    PRIVATE cxx_std_23
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE "${CMAKE_CURRENT_LIST_DIR}/PCH.h"
)

find_package(minhook CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE minhook::minhook
)

find_package(Boost REQUIRED COMPONENTS algorithm)
target_link_libraries(${PROJECT_NAME}
    PRIVATE Boost::algorithm
)

find_package(xbyak CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE xbyak::xbyak
)
