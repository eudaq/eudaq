#! /bin/bash

if [ "$#" -ne 9 ]
then
    echo "not enough arguments!"
    exit 1
fi
n_file=$1
vbb=$(printf "%0.2f" $2)
vrst=$(printf "%0.2f" $3)
vcasn=$(printf "%0.2f" $4)
vcasp=$(printf "%0.2f" $5)
ithr=$(printf "%0.2f" $6)
vlight=$(printf "%0.2f" $7)
acq_time=$(printf "%0.2f" $8)
trg_dly=$(printf "%0.2f" ${9})

# wide number string for the config file name
n_file_str=$(printf "%06d" $n_file)

# assemble file names
config_file='palpideteam_'${n_file_str}'_VBB'${vbb}'_VRST'${vrst}'_VCASN'${vcasn}'_VCASP'${vcasp}'_ITHR'${ithr}'_VLIGHT'${vlight}'_TACQ'$acq_time'_TTRG'${trg_dly}'.conf'

# EUDAQ config file
cp palpideteam_template.conf ${config_file}
sed -i -e 's/VbbTmp/'${vbb}'/g' ${config_file}
sed -i -e 's/VrstTmp/'${vrst}'/g' ${config_file}
sed -i -e 's/VcasnTmp/'${vcasn}'/g' ${config_file}
sed -i -e 's/VcaspTmp/'${vcasp}'/g' ${config_file}
sed -i -e 's/IthrTmp/'${ithr}'/g' ${config_file}
sed -i -e 's/VlightTmp/'${vlight}'/g' ${config_file}
sed -i -e 's/AcqTimeTmp/'${acq_time}'/g' ${config_file}
sed -i -e 's/TrigDelayTmp/'${trg_dly}'/g' ${config_file}
mv ${config_file} conf/

# remove sed tmp files (on MAC only?)
rm -f ${config_file}-e
