#include "taki/TakiProducerFAKEDATA.h"
#include "eudaq/Producer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <fstream>
#include <cctype>


#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

using std::cout;

TakiProducerFAKEDATA::TakiProducerFAKEDATA(const std::string & name, const std::string & runcontrol)
          : eudaq::Producer(name, runcontrol),
            m_b_oldRunJustEnded(false),
            m_b_done(false),
            m_b_runInProgress(false),
            m_u_run(0),
            m_u_ev(0),
            m_u_eventsize(100)
{

}

void TakiProducerFAKEDATA::send(const char* const p_c_buffer_TCP, const int& r_i_numBytes, const unsigned long long& r_ull_triggerID)
{
    cout << "[TakiProducerFAKEDATA::send()] !! Sending " << r_i_numBytes << " bytes !!\n";
    cout << "[TakiProducerFAKEDATA::send()] ==== DATA BEGIN ==== \n";
    for (int a = 0; a < r_i_numBytes; ++a)
    {
        printf("%c, \n",p_c_buffer_TCP[a]);
    }
    cout << "[TakiProducerFAKEDATA::send()] ====  DATA END  ==== \n";

    eudaq::RawDataEvent ev("Taki", m_u_run, ++m_u_ev);
    ev.AddBlock(0, p_c_buffer_TCP, (size_t) r_i_numBytes); //size_t AddBlock(unsigned id, const T * data, size_t bytes)
    ev.SetTag("triggerID",eudaq::to_hex(r_ull_triggerID));
    ev.SetTag("state","OK");
    SendEvent(ev);

    cout << "[TakiProducerFAKEDATA::send()] !! DONE Sending !! m_u_ev=" << m_u_ev <<".\n\n";
}

void TakiProducerFAKEDATA::sendCorrupt(const char* const p_c_buffer_TCP, const int& r_i_numBytes, const unsigned long long& r_ull_triggerID)
{
    cout << "[TakiProducerFAKEDATA::sendCorrupt()] !! Sending " << r_i_numBytes << " bytes !!\n";
    cout << "[TakiProducerFAKEDATA::sendCorrupt()] ==== DATA BEGIN ==== \n";
    for (int a = 0; a < r_i_numBytes; ++a)
    {
        printf("%c, ",p_c_buffer_TCP[a]);
    }
    cout << "[TakiProducerFAKEDATA::sendCorrupt()] ====  DATA END  ==== \n";

    eudaq::RawDataEvent ev("Taki", m_u_run, ++m_u_ev);
    ev.AddBlock(0, p_c_buffer_TCP, (size_t) r_i_numBytes); //size_t AddBlock(unsigned id, const T * data, size_t bytes)
    ev.SetTag("triggerID",eudaq::to_hex(r_ull_triggerID));
    ev.SetTag("state","CORRUPT");
    SendEvent(ev);

    cout << "[TakiProducerFAKEDATA::sendCorrupt()] !! DONE Sending !! m_u_ev=" << m_u_ev <<".\n\n";
}

void TakiProducerFAKEDATA::OnConfigure(const eudaq::Configuration & param) {
    m_u_eventsize = param.Get("EventSize", 1);
    EUDAQ_INFO("Configured (" + param.Name() + ")");
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    std::cout << "[TakiProducerFAKEDATA::OnConfigure()] 'Configured'" << std::endl;
    m_b_done = false; //same as in constructor init
}

void TakiProducerFAKEDATA::OnStartRun(unsigned param) {

    printf("[OnStartRun()] START!");
    m_u_run = param;
    m_u_ev = 0;

    printf("[OnStartRun()] SENDING BORE!");
    //SendEvent(RawDataEvent::BORE("DEPFET", m_run)); //DEPFET EXAMPLE
    SendEvent(eudaq::RawDataEvent::BORE("Taki", m_u_run));
    printf("[OnStartRun()] BORE SENT!");

    std::ostringstream os;
    os << "run #" << m_u_run << " started";
    std::string statusMsg = os.str();

    SetStatus( eudaq::Status::LVL_OK, statusMsg.c_str() );
    std::cout << "[TakiProducerFAKEDATA::OnStartRun()] " << statusMsg << std::endl;
    m_b_runInProgress = true;
    printf("[OnStartRun()] END!");
}

void TakiProducerFAKEDATA::OnStopRun() 
{
    //SendEvent(RawDataEvent::EORE("DEPFET", m_run, ++m_evt)); //DEPFET EXAMPLE
    SendEvent(eudaq::RawDataEvent::EORE("Taki", m_u_run, ++m_u_ev));

    SetStatus(eudaq::Status::LVL_OK, "run stopped");
    std::cout << "[TakiProducerFAKEDATA::OnStopRun()] Run Stopped" << std::endl;
    m_b_runInProgress = false;
    m_b_oldRunJustEnded = true;
}

void TakiProducerFAKEDATA::OnTerminate() 
{
    SetStatus(eudaq::Status::LVL_OK, "terminated");
    std::cout << "[TakiProducerFAKEDATA::OnTerminate()] Producer now done" << std::endl;
    //std::cout << "[TakiProducerFAKEDATA::OnTerminate()] (press enter) (??)" << std::endl;
    m_b_done = true;
}

void TakiProducerFAKEDATA::OnReset() 
{
    std::cout << "[TakiProducerFAKEDATA::OnReset()] Resetting Producer (ready to start again)" << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "reset");
    m_b_done = false;
    m_b_runInProgress = false;
}

void TakiProducerFAKEDATA::OnStatus() 
{

    std::cout << "[TakiProducerFAKEDATA::OnStatus()] NOT PROPERLY IMPLEMENTED YET!" << std::endl;

//     std::cout << "[TakiProducerFAKEDATA::OnStatus()] Status reply:" << std::endl;

//     if (m_b_done)
//     {
//         std::cout << "[TakiProducerFAKEDATA::OnStatus()] Terminated / Need Configuration." << std::endl;
//     }
//     else
//     {
//         std::cout << "[TakiProducerFAKEDATA::OnStatus()] Configured / Not done (not terminated)" << std::endl;
//     }

//     if (m_b_runInProgress)
//     {
//         std::cout << "[TakiProducerFAKEDATA::OnStatus()] Run in progress." << std::endl;
//     }
//     else
//     {
//         std::cout << "[TakiProducerFAKEDATA::OnStatus()] No run in progress." << std::endl;
//     }

}

void TakiProducerFAKEDATA::OnUnrecognised(const std::string & cmd, const std::string & param) 
{
    std::cout << "[TakiProducerFAKEDATA::OnUnrecognised()] Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Just testing!");
}

bool TakiProducerFAKEDATA::runInProgress() 
{
    return m_b_runInProgress;
}

bool TakiProducerFAKEDATA::isDone() 
{
    return m_b_done;
}

int safeReceiveTCP(int socket, unsigned char* p_uc_buffer_TCP, int bytesToReceive)
{
		static int i_received_bytes_tcp = 0;
			
		static int i_bytesRemainingToReceive = bytesToReceive; 
		i_bytesRemainingToReceive = bytesToReceive;
		
		
        while (i_bytesRemainingToReceive>0)
        {
			i_received_bytes_tcp = recv(socket,p_uc_buffer_TCP+(bytesToReceive-i_bytesRemainingToReceive),i_bytesRemainingToReceive,0);
			//i_received_bytes_tcp = read(socket,p_uc_buffer_TCP+(bytesToReceive-i_bytesRemainingToReceive),i_bytesRemainingToReceive);
			if (i_received_bytes_tcp < 0)
            {
                   		printf("[TakiProducerFAKEDATA::safeReceiveTCP()] ERROR reading from socket\n");
                   		return -1;
            }
            if (i_received_bytes_tcp == 0)
            {
                   	 	printf("[TakiProducerFAKEDATA::safeReceiveTCP()] Read zero bytes from socket!\n"); 
                   	 	return 0;
            } 
			printf("[TakiProducerFAKEDATA::safeReceiveTCP()] Received %d bytes from TCP this time (requested %d Bytes).\n", i_received_bytes_tcp, i_bytesRemainingToReceive);
			i_bytesRemainingToReceive -= i_received_bytes_tcp;
		}
		return 1;
}  

int main(int argc, char* argv[]) 
{

    int length = atoi(argv[1]);
    if (length > 1000)
    {
        printf("Too long! Max = 1000\n");
        return -1;
    }


    //./TakiProducer.exe [-r tcp://runcontrolhost:port]
    eudaq::OptionParser optionParser("EUDAQ Producer", "1.0", "A command-line version of a dummy Producer, modified by CT to do some test");
    //eudaq::Option<std::string> rctrl(optionParser, "r", "runcontrol", "tcp://zenpixell3.desy.de:44000", "address", "The address of the RunControl application");
    eudaq::Option<std::string> rctrl(optionParser, "r", "runcontrol", "tcp://128.141.61.32:44000", "address", "The address of the RunControl application");
    eudaq::Option<std::string> level(optionParser, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
    eudaq::Option<std::string> name (optionParser, "n", "name", "SUS_HD_Producer", "string", "The name of this Producer");
    EUDAQ_LOG_LEVEL(level.Value());
    //optionParser.Parse(argv);
    TakiProducerFAKEDATA producer(name.Value(), rctrl.Value());
    printf("[TakiProducer] EUDAQ-Producer connected to runcontrol!\n");


    while (producer.runInProgress()==false)
    {
        printf("[TakiProducer] Waiting for setup!\n");
        sleep(1);
    } //wait for producer to be set up!


    // == wait for termination ==
    if (producer.isDone() == false)
    {
        printf("SENDING!! length = %d\n", length);
        char buff[1000];
        for (int i = 0; i < length; ++i)
        {
            buff[i] = (char) i;
        }
        producer.send(buff, length, 0xFFFFEEEEDDDDCCCCULL);
    }

    while (producer.isDone() == false)
    {
        printf("[TakiProducer] Waiting for termination!\n");
        sleep(1);
    }

    printf("[TakiProducer] DONE!\n");
    return 0;
}
