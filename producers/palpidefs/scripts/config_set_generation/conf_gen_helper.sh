#! /bin/bash

if [ "$#" -ne 9 ]
then
    echo "not enough arguments!"
    exit 1
fi
n_file=$1
vbb=$2
vcasn=$3
ithr=$4
vaux=$5
vrst=$6
idb=$7
dut_pos=$8
scs=$9

# generate folder structure
mkdir -p conf/palpidefs/conf

# hex strings for the XML file
vcasn_hex=$(printf "0x%X\n" ${vcasn})
ithr_hex=$(printf "0x%X\n" ${ithr})
vaux_hex=$(printf "0x%X\n" ${vaux})
vrst_hex=$(printf "0x%X\n" ${vrst})
idb_hex=$(printf "0x%X\n" ${idb})

# wide number string for the config file name
n_file_str=$(printf "%06d" $n_file)

# assemble file names
config_file='palpidefs_'${n_file_str}'_VCASN'${vcasn}'_ITHR'${ithr}'_VBB'${vbb}'_DUTpos'$dut_pos'_VAUX'${vaux}'_VRST'${vrst}'_IDB'${idb}'.conf'
threshold_file='threshold_VCASN'${vcasn}'_ITHR'${ithr}'_VAUX'${vaux}'_VRST'${vrst}'_IDB'${idb}'.xml'

# EUDAQ config file
cp palpidefs_template.conf ${config_file}
sed -i -e 's/thresholdTmp/'${threshold_file}'/g' ${config_file}
sed -i -e 's/VBBtmp/'${vbb}'/g' ${config_file}
sed -i -e 's/DUTposTmp/'$dut_pos'/g' ${config_file}
sed -i -e 's/SCStmp/'$scs'/g' ${config_file}
mv ${config_file} conf/

# XML config file
cp ./threshold_template.xml ${threshold_file}
sed -i -e 's/vcasnTmp/'${vcasn_hex}'/g' ${threshold_file}
sed -i -e 's/ithrTmp/'${ithr_hex}'/g' ${threshold_file}
sed -i -e 's/vauxTmp/'${vaux_hex}'/g' ${threshold_file}
sed -i -e 's/vrstTmp/'${vrst_hex}'/g' ${threshold_file}
sed -i -e 's/idbTmp/'${idb_hex}'/g' ${threshold_file}
mv ${threshold_file} conf/palpidefs/conf

# remove sed tmp files (on MAC only?)
rm -f ${config_file}-e
rm -f ${threshold_file}-e
