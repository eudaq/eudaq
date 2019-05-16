#!/bin/bash
FILE=$1

echo '\nInit'
fgrep 'ini->Get(' $FILE | sed 's/.*ini->Get*("//' | sed 's/).*//' | sed 's/", / = /' | sed 's/",/ = /'

echo '\nConf'
#fgrep 'conf->Get(' $FILE | sed 's/.*conf->Get*("//' | sed 's/).*//' | sed 's/", / = /' | sed 's/",/ = /'
fgrep 'conf->Get(' $FILE | sed 's/.*conf->Get*("/ - `/' | sed 's/).*/`/' | sed 's/", /`: ; default `/' | sed 's/",/`: ; default `/'
