#!/bin/bash

echo "WARN: Please make sure you are now inside the anaconda environment that you wanted to update with eudaq2 dependencies! (y/n)"
read choice

if [ $choice == 'y' ];then
    echo "Update conda env..."
    conda env update --file environment.yml
else
    echo "Exit without update conda env."
fi


