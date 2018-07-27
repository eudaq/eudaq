source /afs/desy.de/project/ilcsoft/sw/x86_64_gcc44_sl6/v01-17-08/init_ilcsoft.sh
source /afs/cern.ch/sw/lcg/external/gcc/4.9/x86_64-slc6-gcc49-opt/setup.sh
source /afs/cern.ch/sw/lcg/app/releases/ROOT/6.06.08/x86_64-slc6-gcc49-opt/root/bin/thisroot.sh

export CMAKEDIR=/afs/cern.ch/sw/lcg/contrib/CMake/3.5.2/Linux-x86_64
export QTDIR=/afs/cern.ch/sw/lcg/external/qt/5.0.0/x86_64-slc6-gcc47-opt
export QTINC=$QTDIR/include
export QTLIB=$QTDIR/lib

export LD_LIBRARY_PATH=$QTLIB:$LD_LIBRARY_PATH

export PATH=${CMAKEDIR}/bin:$QTDIR/bin:./bin:$PATH
