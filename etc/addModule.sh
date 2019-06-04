#!/bin/bash

echo -e "\nPreparing code sceletons for a new module. Note that the script has to executed from the main folder of the repository for now:\n"

# Ask for module name:
read -p "Name of the module? " MODNAME

# Ask for module type:
echo -e "Type of the module?\n"
type=0
select yn in "Producer" "Monitor" "Converter" "All" "ConverterAndMonitor" ; do
    case $yn in
        Producer ) type=1; break;;
        Monitor ) type=2; break;;
        Converter ) type=3; break;;
	All ) type=4; break;;
	ConverterAndMonitor ) type=5; break;;
    esac
done

echo "Creating directory and files..."
echo $type

MODDIR="user/$MODNAME"
echo $MODDIR
echo $MODNAME
# Create directory
mkdir "$MODDIR/"
mkdir  "$MODDIR/module"
mkdir  "$MODDIR/module/src"

# Copy over CMake file and sources from Dummy:
sed -e "s/DUMMY/$MODNAME/g"  "user/Dummy/CMakeLists.txt" > "user/$MODNAME/CMakeLists.txt" \
    -e "s/Dummy/$MODNAME/g"  "user/$MODNAME/CMakeLists.txt" > "user/$MODNAME/CMakeLists.txt"

sed -e "s/DUMMY/$MODNAME/g"  "user/Dummy/module/CMakeLists.txt" >"user/$MODNAME/module/CMakeLists.txt"

# Copy over source code skeleton:
if [ $type -eq 1 ]  || [ $type -eq 4 ] 
then  
 if [ ! -f "user/$MODNAME/module/src/${MODNAME}Producer.cc" ]
 then
	sed -e "s/Dummy/$MODNAME/g" "user/Dummy/module/src/DummyProducer.cc" > "user/$MODNAME/module/src/${MODNAME}Producer.cc"
 else
	echo "Prdoducer already exiting -> will not be recreated"
 fi
fi

if [ $type -eq 3 ]  || [ $type -eq 4 ] || [ $type -eq 5 ]
then  
 if [ ! -f "user/$MODNAME/module/src/${MODNAME}RawEvent2StdEventConverter.cc" ]
 then
sed -e "s/Dummy/$MODNAME/g" "user/Dummy/module/src/DummyRawEvent2StdEventConverter.cc" > "user/$MODNAME/module/src/${MODNAME}RawEvent2StdEventConverter.cc"
 else
	echo "RawEvent2StdEventConverter already exiting -> will not be recreated"
 fi
fi

if [ $type -eq 2 ]  || [ $type -eq 4 ] || [ $type -eq 5 ]
then  
 if [ ! -f "user/$MODNAME/module/src/${MODNAME}Monitor.cc" ]
then  sed -e "s/Dummy/$MODNAME/g" "user/Dummy/module/src/DummyMonitor.cc" > "user/$MODNAME/module/src/${MODNAME}Monitor.cc"
 else
	echo "Monitor already exiting -> will not be recreated"
 fi
fi

# add an cmake entry in the user
if  grep -q  "add_subdirectory($MODNAME)" user/CMakeLists.txt 
 then	echo "already included in user/CMakeLists.txt"
else
echo "add_subdirectory($MODNAME)" >>user/CMakeLists.txt
fi

# Print a summary of the module created:
FINALPATH=`realpath $MODDIR`
echo "Name:   $MODNAME"
echo "Author: $MYNAME ($MYMAIL)"
echo "Path:   $FINALPATH"
echo
echo "Re-run CMake and active in order to build your new module."
