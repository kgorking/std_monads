﻿# Monads for STL containers
# 
#
cmake_minimum_required (VERSION 3.30)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project ("std_monads" LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD 1)

add_executable (monad)
target_compile_features(monad PRIVATE cxx_std_23)
target_sources(monad PUBLIC "main.cpp" )
target_sources(monad PUBLIC FILE_SET CXX_MODULES FILES "monad.cppm")
