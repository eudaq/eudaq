export LD_LIBRARY_PATH=/opt/ipbus-software/uhal/uhal/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/ipbus-software/uhal/log/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/ipbus-software/uhal/grammars/lib:$LD_LIBRARY_PATH
echo $LD__LIBRARY_PATH

export PYTHONPATH=/opt/ipbus-software/uhal/pycohal/pkg:$PYTHONPATH
echo $PYTHONPATH

/opt/ipbus-software/controlhub/scripts/controlhub_start
/opt/ipbus-software/controlhub/scripts/controlhub_info
