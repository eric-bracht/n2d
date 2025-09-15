cmake_minimum_required(VERSION 3.21)
project(n2d LANGUAGES CXX)

option(N2D_USE_AMGCL "Enable amgcl" OFF)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(n2d
  src/io/inspect.cpp
)

target_include_directories(n2d PUBLIC include)

find_package(Eigen3 CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(OpenImageIO CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
if(N2D_USE_AMGCL)
  find_package(amgcl CONFIG REQUIRED)
endif()

target_link_libraries(n2d
  PUBLIC
    Eigen3::Eigen
    TBB::tbb
    OpenImageIO::OpenImageIO
    assimp::assimp
    CLI11::CLI11
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    tl::expected
)

if(N2D_USE_AMGCL)
  target_link_libraries(n2d PUBLIC amgcl::amgcl)
endif()

if(MSVC)
  target_compile_options(n2d PRIVATE $<$<CONFIG:Debug,Release>:/W4 /WX>)
else()
  target_compile_options(n2d PRIVATE $<$<CONFIG:Debug,Release>:-Wall -Wextra -Wpedantic -Werror>)
endif()

add_executable(n2d_cli apps/n2d_cli.cpp)
target_link_libraries(n2d_cli PRIVATE n2d)

enable_testing()
add_subdirectory(tests)
