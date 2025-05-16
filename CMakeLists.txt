# Monads for STL containers
# 
#
cmake_minimum_required (VERSION 3.30)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project ("std_monads" LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD 1)

add_executable (std_monads)
target_compile_features(std_monads PRIVATE cxx_std_23)
target_sources(std_monads PUBLIC "main.cpp" )
target_sources(std_monads PUBLIC FILE_SET CXX_MODULES FILES "std_monads.cppm")
