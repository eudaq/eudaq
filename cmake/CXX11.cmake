# Copyright (c) 2013 Nathan Osman

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Determines whether or not the compiler supports C++11
# some information on the C++11 compatability of the various compilers:
# Clang: http://clang.llvm.org/cxx_status.html
# GCC: http://gcc.gnu.org/projects/cxx0x.html
# MSVC: http://msdn.microsoft.com/en-us/en-us/library/hh567368.aspx
macro(check_for_cxx11_compiler _VAR)
    message(STATUS "Checking for C++11 compliance of compiler")
    set(${_VAR})
    if(
       (MSVC AND MSVC11) OR
       (MSVC AND MSVC12) OR
       (CMAKE_COMPILER_IS_GNUCXX AND NOT ${COMPILER_VERSION} VERSION_LESS 4.6) OR
       (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT ${COMPILER_VERSION} VERSION_LESS 3.1))
        set(${_VAR} 1)
	message(STATUS "Checking for C++11 compliance of compiler - available")
    else()
	message(STATUS "Checking for C++11 compliance of compiler - unavailable")
    endif()
endmacro()


# Sets the appropriate flag to enable C++11 support
macro(enable_cxx11)
  if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # do some more elaborate compiler argument checks
    # as suggested in http://stackoverflow.com/questions/20166663/compiler-flags-for-c11

    # test for C++11 flags
    include(TestCXXAcceptsFlag)

    # try to use compiler flag -std=c++11
    check_cxx_accepts_flag("-std=c++11" CXX_FLAG_CXX11)

    if(CXX_FLAG_CXX11)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else(CXX_FLAG_CXX11)
      # try to use compiler flag -std=c++0x for older compilers
      check_cxx_accepts_flag("-std=c++0x" CXX_FLAG_CXX0X)
      if(CXX_FLAG_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
      else(CXX_FLAG_CXX0X)
	message( FATAL_ERROR "Compiler does not support either '-std=c++0x' or '-std=c++11' flags." )
      endif(CXX_FLAG_CXX0X)
    endif(CXX_FLAG_CXX11)
    
    # and clang needs additional flags
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    endif()

  ELSEIF(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  /D \"CPP11\"")
  endif()


endmacro()

