cmake_minimum_required (VERSION 3.31)

# https://github.com/Kitware/CMake/blob/v3.31.6/Help/dev/experimental.rst
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project ("monads" LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD 1)

#add_compile_options(/Qpar /Qpar-report:1 /Qvec-report:1)


add_executable (monad)
target_compile_features(monad PRIVATE cxx_std_23) 
target_sources(monad PUBLIC "main.cpp")
target_sources(monad PUBLIC FILE_SET CXX_MODULES FILES "monad.cppm")

add_executable (unittest)
target_compile_features(unittest PRIVATE cxx_std_23)
target_sources(unittest PUBLIC "unittest.cpp")
target_sources(unittest PUBLIC FILE_SET CXX_MODULES FILES "monad.cppm")

add_executable (benchmark)
target_compile_features(benchmark PRIVATE cxx_std_23)
target_sources(benchmark PUBLIC "benchmark.cpp" "nanobench.cpp")
target_sources(benchmark PUBLIC FILE_SET CXX_MODULES FILES "monad.cppm")
