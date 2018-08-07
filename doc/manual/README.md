To generate a pdf file from the LaTeX source, run these commands in your CMake build directory, e.g. eudaq/build:
cmake -D EUDAQ_BUILD_MANUAL=ON ..
make pdf
make install

The last step will copy the pdf from eudaq/build/doc/manual to eudaq/doc/manual
