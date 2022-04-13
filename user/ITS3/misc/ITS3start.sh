#!/bin/bash

### VARIABLES#####################################
NPLANES=7             # Number of telescope planes
USE_POWERPRODUCER=YES # YES or NO
USE_PTHPRODUCER=YES   # YES or NO
##################################################

### Do not touch the variables below unles you are developing EUDAQ ITS3
EUDAQ=$(dirname "$0")/../../../
PYTHONPATH=${EUDAQ}/lib
RUNCONTROL="PYTHONPATH=${PYTHONPATH} ${EUDAQ}/user/ITS3/python/ITS3RunControl.py"
ALPIDEPRODUCER="PYTHONPATH=${PYTHONPATH} ${EUDAQ}/user/ITS3/python/ALPIDEProducer.py"
POWERPRODUCER="PYTHONPATH=${PYTHONPATH} ${EUDAQ}/user/ITS3/python/PowerProducer.py"
PTHPRODUCER="PYTHONPATH=${PYTHONPATH} ${EUDAQ}/user/ITS3/python/PTHProducer.py"
DATACOLLECTOR="PYTHONPATH=${PYTHONPATH} ${EUDAQ}/user/ITS3/python/ITS3DataCollector.py"
SESSION="ITS3"
RC_LOG="rc.log"

NPROD=$NPLANES
if [ $USE_POWERPRODUCER = YES ]; then
    PROD_POWER=$NPROD
    let NPROD=$NPROD+1
fi

if [ $USE_PTHPRODUCER = YES ]; then
    PROD_PTH=$NPROD
    let NPROD=$NPROD+1
fi


tmux new-session -d -s $SESSION -n rc
if [[ $? -ne 0 ]];then exit $?;fi
tmux set-option -t =$SESSION -g pane-border-status top
tmux set-option -t =$SESSION -g pane-border-format "#P: #{pane_title} (#{pane_pid})"

tmux select-pane -t =$SESSION:=rc -T "Run Control"

tmux new-window  -t =$SESSION -n ps
for ((prod=1;prod<$NPROD;prod++)); do
    tmux split-window  -t =$SESSION:=ps
    tmux select-layout -t =$SESSION:=ps tiled
done
for ((plane=0;plane<$NPLANES;plane++)); do
    tmux select-pane -t =$SESSION:=ps.$plane -T "Producer $plane"
done
[[ $USE_POWERPRODUCER = YES ]] && tmux select-pane -t =$SESSION:=ps.$PROD_POWER -T "Producer Power"
[[ $USE_PTHPRODUCER = YES ]] && tmux select-pane -t =$SESSION:=ps.$PROD_PTH -T "Producer PTH"


tmux new-window  -t =$SESSION -n dc
tmux select-pane -t =$SESSION:=dc -T "Data Collector"

tmux new-window  -t =$SESSION -n rclog
tmux select-pane -t =$SESSION:=rclog -T "Run Control Log"
tmux send-keys -t =$SESSION:=rclog "touch $RC_LOG" ENTER
tmux send-keys -t =$SESSION:=rclog "tail -f $RC_LOG" ENTER


tmux new-window  -t =$SESSION -n perf
tmux select-pane -t =$SESSION:=perf -T "Performance"
tmux send-keys -t =$SESSION:=perf "htop" ENTER

tmux send-keys -t =$SESSION:=rc "$RUNCONTROL 2> $RC_LOG" ENTER

sleep 2 # TODO: sleep less

tmux send-keys -t =$SESSION:=dc "$DATACOLLECTOR" ENTER

for ((plane=0;plane<$NPLANES;plane++)); do
   tmux send-keys -t =$SESSION:=ps.$plane "$ALPIDEPRODUCER --name ALPIDE_plane_$plane" ENTER
done
[[ $USE_POWERPRODUCER = YES ]] && tmux send-keys -t =$SESSION:=ps.$PROD_POWER "$POWERPRODUCER" ENTER
[[ $USE_PTHPRODUCER = YES ]] && tmux send-keys -t =$SESSION:=ps.$PROD_PTH "$PTHPRODUCER" ENTER

tmux select-window -t =$SESSION:=rc

