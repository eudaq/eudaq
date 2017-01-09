#!/usr/bin/env sh
CACTUS_RPM_REPO=http://cactus.web.cern.ch/cactus/release/2.4/slc6_x86_64/base/RPMS/
CACTUS_THIS_DIR=`pwd`
CACTUS_EXTRACT_DIR=$CACTUS_THIS_DIR/cactus_v2p4_slc6_x86_64
CACTUS_DOWNLOAD_DIR=$CACTUS_THIS_DIR/cactus_rpm

mkdir $CACTUS_DOWNLOAD_DIR
mkdir $CACTUS_EXTRACT_DIR
wget $CACTUS_RPM_REPO

for i in `cat index.html | grep cactuscore-extern | grep rpm | cut -f 6 -d '"'`
do
    cd $CACTUS_DOWNLOAD_DIR
    wget --continue $CACTUS_RPM_REPO/$i
    cd $CACTUS_EXTRACT_DIR
    rpm2cpio $CACTUS_DOWNLOAD_DIR/$i | cpio --extract --make-directories
    cd $CACTUS_THIS_DIR
done

for i in `cat index.html | grep cactuscore-uhal | grep rpm | cut -f 6 -d '"'`
do
    cd $CACTUS_DOWNLOAD_DIR
    wget --continue $CACTUS_RPM_REPO/$i
    cd $CACTUS_EXTRACT_DIR
    rpm2cpio $CACTUS_DOWNLOAD_DIR/$i | cpio --extract --make-directories
    cd $CACTUS_THIS_DIR
done

for i in `cat index.html | grep cactuscore-controlhub | grep rpm | cut -f 6 -d '"'`
do
    cd $CACTUS_DOWNLOAD_DIR
    wget --continue $CACTUS_RPM_REPO/$i
    cd $CACTUS_EXTRACT_DIR
    rpm2cpio $CACTUS_DOWNLOAD_DIR/$i | cpio --extract --make-directories
    cd $CACTUS_THIS_DIR
done

cd $CACTUS_THIS_DIR
rm index.html
