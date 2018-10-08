#source /afs/cern.ch/sw/lcg/external/gcc/4.9/x86_64-slc6-gcc49-opt/setup.sh
source /cvmfs/cms.cern.ch/cmsset_default.sh
cd /cvmfs/cms.cern.ch/slc6_amd64_gcc493/cms/cmssw/CMSSW_8_0_1/src/
eval `scramv1 runtime -sh`
cd -

export CMAKEDIR=/afs/cern.ch/sw/lcg/contrib/CMake/3.5.2/Linux-x86_64
export QTDIR=/afs/cern.ch/sw/lcg/external/qt/5.0.0/x86_64-slc6-gcc47-opt
export QTINC=$QTDIR/include
export QTLIB=$QTDIR/lib

export LD_LIBRARY_PATH=$QTLIB:$LD_LIBRARY_PATH

export PATH=${CMAKEDIR}/bin:$QTDIR/bin:./bin:$PATH
