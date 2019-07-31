#euRun &
#sleep 8

euLog &
sleep 2

euCliCollector -n TriggerIDSyncDataCollector -t one_dc &
sleep 2

euCliProducer -n AidaTluProducer -t aida_tlu &
sleep 2

euCliProducer -n NiProducer -t ni_mimosa &
sleep 2

euCliProducer -n PIStageProducer -t Stage
