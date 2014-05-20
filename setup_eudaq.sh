#. /afs/cern.ch/sw/lcg/external/gcc/4.8.1/x86_64-slc5-gcc48-opt/setup.sh
#. /afs/cern.ch/sw/lcg/app/releases/ROOT/5.33.02/x86_64-slc5-gcc43-opt/root/bin/thisroot.sh
#export QTDIR=/usr/lib/qt4

. /afs/cern.ch/sw/lcg/external/gcc/4.8.1/x86_64-slc6-gcc48-opt/setup.sh
. /afs/cern.ch/sw/lcg/app/releases/ROOT/5.34.18/x86_64-slc6-gcc48-opt/root/bin/thisroot.sh

export LCGEXTERNAL=/afs/cern.ch/sw/lcg/external
export CMAKE_HOME=$LCGEXTERNAL/CMake/2.8.8/x86_64-slc6-gcc46-opt
export PATH=${CMAKE_HOME}/bin:$PATH

export QTDIR=/afs/cern.ch/sw/lcg/external/qt/5.0.0/x86_64-slc6-gcc47-opt
export QTINC=$QTDIR/include
export QTLIB=$QTDIR/lib
export PATH=$QTDIR/bin:$PATH
