#ifndef EUDAQ_INCLUDED_Documentation
#define EUDAQ_INCLUDED_Documentation

/** \file Documentation.hh
 * Extra documentation that doesn't fit in any other file.
 * There is no need to included this file anywhere, since all
 * it contains is comments for doxygen.
 */

/** \mainpage EUDAQ Documentation
 *
 * \section intro_sec Introduction
 * EUDAQ is a simple and easy to use data acquisition framework, written in C++.
 * It was designed to be used for the EUDET JRA1 beam telescope (see http://www.eudet.org).
 *
 * The overall structure has some resemblance to the XDAQ framework (hence the name EUDAQ)
 * but it is greatly simplified, and relies on as few external libraries as possible.
 *
 * \subsection port_sec Portability
 * EUDAQ is designed to be portable, working out of the box Linux, Mac OSX, and
 * Windows.
 * \subsubsection cpp_sec C++
 * It is written mainly in Standard C++, and can be compiled using gcc, Clang, or MSVC.
 * \subsubsection posix_sec Posix
 * Some posix extensions are used where necessary for threading (pthreads) and sockets
 * (Berkely Sockets API).
 * \subsubsection gui_sec GUI
 * Qt is used as portable graphics library for the GUI. All the applications also have a text-only interface
 * so they may be run without the GUI if necessary.
 *
 * \section install_sec Installation
 * \subsection step1 Step 1: Get the code
 * The latest version of the EUDAQ code is available as git repository hosted by GitHub
 * at https://github.com/eudaq/eudaq.
 *
 * @code
 *   git clone https://github.com/eudaq/eudaq.git eudaq
 * @endcode
 *
 * \subsection step2 Step 2: Build it
 * Go to the directory containing eudaq, enter the build directory and type 'cmake ..' and 'make install'. This will build the libeudaq.so library and place it in the ./lib folder. For more detailed instructions see the README.md inside this repository or the manual provided through LaTeX sources.
 *
 */

/** \namespace eudaq
 * The main EUDAQ namespace.
 * All of the EUDAQ components should be inside this namespace.
 */

#endif
