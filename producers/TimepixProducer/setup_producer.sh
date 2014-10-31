#source ~/root/bin/thisroot.sh 
source /afs/cern.ch/sw/lcg/app/releases/ROOT/5.34.02_python2.7/x86_64-slc5-gcc43-opt/root/bin/thisroot.sh 
 export TP_CONFIG=/home/lcd/CLIC_Testbeam_August2013/TimepixAssemblies_Data
 export EUDAQ=/afs/cern.ch/work/m/mbenoit/private/Testbeam_Software/eudaq
 export TPPROD=$EUDAQ/producers/TimepixProducer
 export LD_LIBRARY_PATH=$EUDAQ/bin:$LD_LIBRARY_PATH 
 export LD_LIBRARY_PATH=$EUDAQ/TimepixProducer/Pixelman_2013_09_25_x64:$LD_LIBRARY_PATH 
 export KEITHLEYLIB=/home/lcd/CLIC_Testbeam_August2013/gpib_reader
