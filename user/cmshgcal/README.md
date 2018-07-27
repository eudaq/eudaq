# Ipbus and Eudaq : 

To build and compile the code:

* Install ipbus : [link](https://svnweb.cern.ch/trac/cactus/wiki/uhalQuickTutorial#HowtoInstalltheIPbusSuite)

* Import libraries paths if needed:
```
export LD_LIBRARY_PATH=/opt/cactus/lib:$LD_LIBRARY_PATH
export PATH=/opt/cactus/bin:$PATH
```

* Compile. From eudaq root dir:
```
cd build
cmake -DCMAKE_PREFIX_PATH=/afs/cern.ch/sw/lcg/external/qt/5.0.0/x86_64-slc6-gcc47-opt -DUSER_CMSHGCAL_BUILD=ON ../
make install -j4
```

* Start ORM data emulation: see [ipbus-test](https://github.com/asteencern/ipbus-test/tree/hgcal-test) repository. This can be launched from any computer with ipbus installed. 
* Run this producer (from root of eudaq):
```
./bin/euRun &
./bin/HGCalProducer
```
  * Load config file from *eudaq/user/cmshgcal/misc/hgcal.conf*, and start the run. It will readout the data and store them in file.


