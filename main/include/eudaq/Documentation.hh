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
 * The latest version of the EUDAQ code will soon be available either via our Subversion server
 * at http://svn.example.com/eudaq/ or you can download a tarball from
 * http://www.example.com/eudaq/download/
 * \subsection step2 Step 2: Unzip it
 * If you downloaded the tarball, unzip it into a directory of your choice.
 * \subsection step3 Step 3: Build it
 * Go to the directory containing eudaq and type make.
 *
 */

/** \namespace eudaq
 * The main EUDAQ namespace.
 * All of the EUDAQ components should be inside this namespace.
 */

#endif
