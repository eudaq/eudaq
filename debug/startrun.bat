echo off
taskkill /f /im TestRunControl.exe.exe
taskkill /f /im TestLogCollector.exe.exe 
taskkill /f /im TestDataCollector.exe.exe 
taskkill /f /im ExampleProducer.exe.exe 

start TestRunControl.exe.exe -a -c nim,ni_autotrig -d 100 -r 2
Timeout /t 1 
start TestLogCollector.exe.exe 
Timeout /t 1 
start TestDataCollector.exe.exe 
Timeout /t 5 
start ExampleProducer.exe.exe -r tcp://192.168.56.1:44000
