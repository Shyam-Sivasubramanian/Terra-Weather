# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-src"
  "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-build"
  "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-subbuild/glad-populate-prefix"
  "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-subbuild/glad-populate-prefix/tmp"
  "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp"
  "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-subbuild/glad-populate-prefix/src"
  "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/mnt/c/Users/shyam/Documents/code/ProceduralWorld/ProceduralWorld/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
