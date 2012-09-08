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
 * EUDAQ is a simple and easy to use data aquisition framework, written in C++.
 * It was designed to be used for the EUDET JRA1 beam telescope (see http://www.eudet.org).
 *
 * The overall structure has some resemblance to the XDAQ framework (hence the name EUDAQ)
 * but it is greatly simplified, and relies on as few external libraries as possible.
 *
 * \subsection port_sec Portability
 * EUDAQ is designed to be reasonably portable, working out of the box Linux, MAC OS X, and
 * Windows with cygwin. It may be ported to Microsoft Visual C++ and/or Borland Developer
 * in future versions.
 * \subsubsection cpp_sec C++
 * It is written mainly in Standard C++, and compiles on gcc in pedantic ansi mode with no
 * warnings.
 * \subsubsection posix_sec Posix
 * Some posix extensions are used where necessary for threading (pthreads) and sockets
 * (Berkely Sockets API). The socket functionality should be relatively easy to port to
 * WinSocks, and there are freely available pthread libraries for Windows so the threading
 * code should not need to be significantly rewritten.
 * \subsubsection gui_sec GUI
 * A portable graphics library will be used for the GUI, this may be Qt, WxWidgets, or
 * simply Root (yet to be decided). All the applications also have a text-only interface
 * so they may be run without the GUI if necessary.
 *
 * \section install_sec Installation
 * \subsection step1 Step 1: Get the code
 * The latest version of the EUDAQ code is available on eudaq.hepforge.org SubVersion control server
 * at http://eudaq.hepforge.org/svn/.
 *
 * @code
 *   svn co  http://eudaq.hepforge.org/svn/trunk/ eudaq
 * @endcode
 *
 * \subsection step2 Step 2: Build it
 * Go to the directory containing eudaq and type make main. This will be the libeudaq.so library and place it in the ./bin/ folder.
 *
 */

/** \namespace eudaq
 * The main EUDAQ namespace.
 * All of the EUDAQ components should be inside this namespace.
 */

#endif
