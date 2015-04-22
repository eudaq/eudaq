echo off
taskkill /f /im TestRunControl.exe.exe
taskkill /f /im TestLogCollector.exe.exe 
taskkill /f /im TestDataCollector.exe.exe 
taskkill /f /im ExampleProducer.exe.exe 
Timeout /t 1 
start TestRunControl.exe.exe -s -c test -d 30
Timeout /t 1 
start TestLogCollector.exe.exe 
Timeout /t 1 
start TestDataCollector.exe.exe 
Timeout /t 5 
start ExampleProducer.exe.exe 
start ExampleProducer.exe.exe -n testproducer
