#include "eudaq/FileReader.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/OptionParser.hh"
#include <fstream>
#include <iostream>
#include <iomanip>

static const std::string EVENT_TYPE = "Example";

std::string makestring(long int);
std::string makeLongString(long int);
std::string getOutputFilename(long int);
std::string getHeader(std::string,int,long int);
std::string getTimeString(long int);
long int getTime(long int);

int main(int /*argc*/, const char ** argv) 
{
    // You can use the OptionParser to get command-line arguments
    // then they will automatically be described in the help (-h) option
    eudaq::OptionParser op("EUDAQ Example File Reader", "1.0", "Print out the test data in approprriate format", 1);
    eudaq::Option<int> eventsToProcess(op, "e", "eventstoprocess", -1, "value",  "Number of events to process");
    eudaq::Option<int> eventsToGroup(op, "g", "eventstogroup", 100, "value",  "Number of events to group into one output file");
    eudaq::Option<int> eventsToSkip(op, "s", "eventstoskip", 0, "value",  "Number of events to skip");
    eudaq::Option<std::string> outputPath(op, "p", "outputpath", "Output path for results files", "string", "Output filename");

    try 
    {
        // This will look through the command-line arguments and set the options
        op.Parse(argv);

        // Loop over all filenames
        for (size_t i = 0; i < op.NumArgs(); ++i) 
        {
            // Create a reader for this file
            eudaq::FileReader reader(op.GetArg(i));

            // Display the actual filename (argument could have been a run number)
            std::cout << "Opened file: " << reader.Filename() << std::endl;

//            std::cout << "Opened file: " << reader.Filename() << std::endl; 
//            std::cout << "BORE:\n" << reader.GetDetectorEvent() << std::endl; 
//            while (reader.NextEvent()) 
//            {
//                std::cout << reader.GetDetectorEvent() << std::endl;
//            }

            // The PluginManager should be initialized with the BORE
            eudaq::PluginManager::Initialize(reader.GetDetectorEvent());

            long int numberOfEvents(0);
            std::ofstream outFile;
            // Now loop over all events in the file
            while (reader.NextEvent() && (numberOfEvents < eventsToProcess.Value())) 
            {
                numberOfEvents++;
                
                if (numberOfEvents < eventsToSkip.Value())
                {
                    continue;
                }

                if (reader.GetDetectorEvent().IsEORE()) 
                {
                    std::cout << "End of run detected" << std::endl;
                    // Don't try to process if it is an EORE
                    break;
                }

                if (reader.GetDetectorEvent().GetEventNumber() % eventsToGroup.Value() == 0 || numberOfEvents == eventsToSkip.Value())
                {
                    // Output file information
                    if(outFile.is_open())
                    {
                        outFile.close();
                    }
                    std::string outFileName = outputPath.Value() + "/" + getOutputFilename(reader.GetDetectorEvent().GetEventNumber());
                    outFile.open(outFileName.c_str());
                }

                // Event processing
                const eudaq::Event & event  = reader.GetDetectorEvent();
                const eudaq::StandardEvent standardEvent = eudaq::PluginManager::ConvertToStandard(reader.GetDetectorEvent());
                unsigned int numberOfPlanes = (unsigned int) standardEvent.NumPlanes();

//                std::cout << "Number of planes in event is : " << numberOfPlanes << std::endl;

                int mimosaCounter = 0;

                for (unsigned int i = 0; i < numberOfPlanes; i++)
                {
                    const eudaq::StandardPlane & standardPlane = standardEvent.GetPlane(i);
//                    outFile << getHeader(standardPlane.Type(), mimosaCounter, standardPlane.TLUEvent()) << "\n";
                    outFile << getHeader(standardPlane.Type(), mimosaCounter, numberOfEvents) << "\n";
//                    std::cout << "Plane Type : " << standardPlane.Type() << std::endl;

                    if ("CCPX" == standardPlane.Type()) // CLICPIX
                    {
                        for(unsigned int j = 0; j < standardPlane.HitPixels(); j++)
                        {
                            outFile << standardPlane.GetX(j) << "\t" << standardPlane.GetY(j) << "\t" << standardPlane.GetPixel(j) << std::endl;
                        }                 
                    }
                    else if ("NI" == standardPlane.Type()) // MIMOSA
                    {
                        for(unsigned int j = 0; j < standardPlane.HitPixels(); j++)
                        {
                            outFile << standardPlane.GetX(j) << "\t" << standardPlane.GetY(j) << "\t" << 1 << std::endl;
                        }
                        mimosaCounter++;
                    }
                    else if ("Timepix3Raw" == standardPlane.Type()) // TimePix 
                    {
                        for(unsigned int j = 0; j < standardPlane.HitPixels(); j++)
                        {
                            outFile << standardPlane.GetX(j) << "\t" << standardPlane.GetY(j) << "\t" << standardPlane.GetPixel(j) << std::endl;
                        }
                        mimosaCounter++;
                    }
                    else
                    {
                        std::cout << "=====Unknown Standard Plane Type=====" << std::endl;
                    }
                }
            }
        }
    } 
    
    catch (...) 
    {
        // This does some basic error handling of common exceptions
        return op.HandleMainException();
    }
    return 0;
}

//============================================

std::string getOutputFilename(long int eventNum)
{
    std::string fileName = "mpx-141004-000600-" + makeLongString(eventNum) + ".txt";
    return fileName;
}

//============================================

std::string getHeader(std::string standardPlanType, int mimosaCounter, long int eventNum)
{
    std::string deviceID;
    if(strcmp(standardPlanType.c_str(),"NI")==0) deviceID = "Mim-osa0" + makestring(mimosaCounter);
    if(strcmp(standardPlanType.c_str(),"CCPX")==0) deviceID = "CLi-CPix";
    if(strcmp(standardPlanType.c_str(),"Timepix3Raw")==0) deviceID = "TimePix3";

    std::string time = getTimeString(eventNum);
    long int timestamp = getTime(eventNum);
    std::string header = "# Start time (string) : Oct 04 " + time + " # Start time : " + makestring(timestamp) + "0 # Acq time : 0.015000 # ChipboardID : " + deviceID + " # DACs : 5 100 255 127 127 0 385 7 130 130 80 56 128 128 # Mpx type : 3 # Timepix clock : 40 # Eventnr 497 # RelaxD 3 devs 4 TPX DAQ = 0x110402 = START_HW STOP_HW MPX_PAR_READ COMPRESS=1";
    return header;
}

//============================================

std::string getTimeString(long int eventNum)
{
    std::string time = "Oct 04 00:06:0." + makeLongString(eventNum);
    return time;
}

//============================================
  
long int getTime(long int eventNum)
{
    long int time = 360e9 + eventNum;
    return time;
}

//============================================

std::string makeLongString(long int integer)
{
    std::ostringstream strs;
    strs << std::setprecision(9) << std::fixed << std::showpoint << (integer/1.e8); // 10 ns time gap between events
    std::string legs = strs.str();
    return legs;
}

//============================================

std::string makestring(long int integer)
{
    std::ostringstream strs;
    strs << integer;
    std::string legs = strs.str();
    return legs;
}

//============================================
