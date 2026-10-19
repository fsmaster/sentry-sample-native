cmake_minimum_required(VERSION 3.18)
project(sentry_sample C CXX)

# sentry-native is pulled in as a subdirectory, built with the crashpad backend
set(SENTRY_BACKEND crashpad CACHE STRING "" FORCE)
set(SENTRY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SENTRY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/sentry-native)

add_executable(sample main.c)
target_link_libraries(sample PRIVATE sentry::sentry)
