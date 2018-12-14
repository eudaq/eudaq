#!/bin/sh

while true; do
    read -p "Do you want to delete the folder-contents of build, lib, bin and extern? " yn
    case $yn in
        [Yy]* ) rm -rf build/* && rm -rf extern/* && rm -rf bin/* && rm -rf lib/*; break;;
        [Nn]* ) return;;
        * ) echo "Please answer yes or no.";;
    esac
done

cd build
