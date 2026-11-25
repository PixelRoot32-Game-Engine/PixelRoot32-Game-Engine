cmake_minimum_required(VERSION 3.20)

# Out-of-tree consumer of the installed package. It must not see this
# repository: no add_subdirectory, no relative includes, no source paths. Its
# only input is the prefix PixelRoot32 was installed into, which is what makes
# it the acceptance test for `install()` + `find_package()` instead of a
# restatement of the build.
#
#   cmake -S tools/cmake_consumer -B build/consumer \
#         -DCMAKE_PREFIX_PATH=<install prefix>
#   cmake --build build/consumer
#   ./build/consumer/pr32_cmake_consumer

project(pr32_cmake_consumer LANGUAGES CXX)

find_package(PixelRoot32 1.16 CONFIG REQUIRED)

add_executable(pr32_cmake_consumer src/main.cpp)
target_link_libraries(pr32_cmake_consumer PRIVATE PixelRoot32::Engine)

# Printed by main.cpp: proves find_package matched a version file, not just a
# config file.
target_compile_definitions(pr32_cmake_consumer PRIVATE
    PR32_PACKAGE_VERSION="${PixelRoot32_VERSION}")
set_target_properties(pr32_cmake_consumer PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

# The engine's flags are part of its exported interface: a consumer compiles
# the same preprocessor branches the library was built with, without repeating
# any -D of its own. src/main.cpp asserts this for PLATFORM_NATIVE.
