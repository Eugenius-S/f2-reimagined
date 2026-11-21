cmake_minimum_required(VERSION 3.18)
project(scene_build_tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(scene_build_tests
    ../../src/scene/scene_build.cc
    scene_build_test.cc
)
add_test(NAME scene_build_tests COMMAND scene_build_tests)
