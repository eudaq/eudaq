all: caliceProducer caliceDataCollector caliceMonitor

CPPFLAGS = -g -Wall -Wno-deprecated -I../main/include -I/home/calice/ilcsoft-eudaq/v01-17-03/lcio/v02-04-02/include -std=c++11 -I$(shell root-config --incdir)
LIBS = -lpthread -lstdc++ -L../lib -lEUDAQ -L/home/calice/ilcsoft-eudaq/v01-17-03/lcio/v02-04-02/lib -llcio

caliceProducer: CaliceReceiver.o SiReader.o ScReader.o
	gcc -g -Wall -o caliceProducer CaliceReceiver.o SiReader.o ScReader.o $(shell root-config --glibs) $(LIBS)

caliceDataCollector: CaliceDataCollector.o CaliceGenericConverterPlugin.o FileWriterLCIOC.o
	gcc -g -Wall -o caliceDataCollector CaliceDataCollector.o CaliceGenericConverterPlugin.o FileWriterLCIOC.o $(shell root-config --glibs) $(LIBS)
#caliceDataCollector: CaliceDataCollector.o CaliceGenericConverterPlugin.o
#	gcc -g -Wall -lpthread -lstdc++ -L../eudaq-1.3.1/lib -lEUDAQ -L ../lcio/v02-04-03/lib -llcio -o caliceDataCollector CaliceDataCollector.o CaliceGenericConverterPlugin.o
#caliceMonitor: moniq/CaliceMonitor.o CaliceGenericConverterPlugin.o
#	gcc -g -Wall $(LIBS) -o caliceMonitor moniq/CaliceMonitor.o CaliceGenericConverterPlugin.o

.cc.o	:
	g++ $(CPPFLAGS) -c $< -o $@

%.o : %.h

clean :
	rm -f *.o *~ \#*  caliceProducer caliceDataCollector caliceMonitor
