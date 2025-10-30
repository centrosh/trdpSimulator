cmake_minimum_required(VERSION 3.16)
project(trdpSimulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build unit tests" ON)

add_library(trdp_simulator
    src/communication/Wrapper.cpp
    src/simulation/Engine.cpp
)

target_include_directories(trdp_simulator
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(trdp_simulator PUBLIC cxx_std_20)

add_executable(trdp_sim_cli src/main.cpp)
target_link_libraries(trdp_sim_cli PRIVATE trdp_simulator)

if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()
