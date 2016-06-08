CACTUS_RPM_REPO=http://cactus.web.cern.ch/cactus/release/2.3/slc6_x86_64/base/RPMS/

# use gzip for slc5 and xz for slc6
#UNCOMPRESS_COMMAND="gunzip -c -"
UNCOMPRESS_COMMAND="xzcat -"

wget $CACTUS_RPM_REPO
for i in `cat index.html | grep cactuscore-extern | grep rpm | cut -f 6 -d '"'`
do
wget --continue $CACTUS_RPM_REPO/$i
rpm2cpio $i | $UNCOMPRESS_COMMAND | cpio -ivmd
done
for i in `cat index.html | grep cactuscore-uhal | grep rpm | cut -f 6 -d '"'`
do
wget --continue $CACTUS_RPM_REPO/$i
rpm2cpio $i | $UNCOMPRESS_COMMAND | cpio -ivmd
done

rm index.html
