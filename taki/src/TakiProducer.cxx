#include "taki/TakiProducer.h"
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

TakiProducer::TakiProducer(const std::string & name, const std::string & runcontrol)
          : eudaq::Producer(name, runcontrol),
            m_b_oldRunJustEnded(false),
            m_b_done(false),
            m_b_runInProgress(false),
            m_u_run(0),
            m_u_ev(0),
            m_u_eventsize(100)
{

}

void TakiProducer::send(const char* const p_c_buffer_TCP, const int& r_i_numBytes, const unsigned long long& r_ull_triggerID)
{
    cout << "[TakiProducer::send()] !! Sending " << r_i_numBytes << " bytes !!\n";
    cout << "[TakiProducer::send()] ==== DATA BEGIN ==== \n";
    for (int a = 0; a < r_i_numBytes; ++a)
    {
        printf("%c, ",p_c_buffer_TCP[a]);
    }
    cout << "[TakiProducer::send()] ====  DATA END  ==== \n";

    eudaq::RawDataEvent ev("Taki", m_u_run, ++m_u_ev);
    ev.AddBlock(0, p_c_buffer_TCP, (size_t) r_i_numBytes); //size_t AddBlock(unsigned id, const T * data, size_t bytes)
    ev.SetTag("triggerID",eudaq::to_hex(r_ull_triggerID));
    ev.SetTag("state","OK");
    SendEvent(ev);

    cout << "[TakiProducer::send()] !! DONE Sending !! m_u_ev=" << m_u_ev <<".\n\n";
}

void TakiProducer::sendCorrupt(const char* const p_c_buffer_TCP, const int& r_i_numBytes, const unsigned long long& r_ull_triggerID)
{
    cout << "[TakiProducer::sendCorrupt()] !! Sending " << r_i_numBytes << " bytes !!\n";
    cout << "[TakiProducer::sendCorrupt()] ==== DATA BEGIN ==== \n";
    for (int a = 0; a < r_i_numBytes; ++a)
    {
        printf("%c, ",p_c_buffer_TCP[a]);
    }
    cout << "[TakiProducer::sendCorrupt()] ====  DATA END  ==== \n";

    eudaq::RawDataEvent ev("Taki", m_u_run, ++m_u_ev);
    ev.AddBlock(0, p_c_buffer_TCP, (size_t) r_i_numBytes); //size_t AddBlock(unsigned id, const T * data, size_t bytes)
    ev.SetTag("triggerID",eudaq::to_hex(r_ull_triggerID));
    ev.SetTag("state","CORRUPT");
    SendEvent(ev);

    cout << "[TakiProducer::sendCorrupt()] !! DONE Sending !! m_u_ev=" << m_u_ev <<".\n\n";
}

void TakiProducer::OnConfigure(const eudaq::Configuration & param) {
    m_u_eventsize = param.Get("EventSize", 1);
    EUDAQ_INFO("Configured (" + param.Name() + ")");
    SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    std::cout << "[TakiProducer::OnConfigure()] 'Configured'" << std::endl;
    m_b_done = false; //same as in constructor init
}

void TakiProducer::OnStartRun(unsigned param) {
    m_u_run = param;
    m_u_ev = 0;
    //SendEvent(RawDataEvent::BORE("DEPFET", m_run)); //DEPFET EXAMPLE
    SendEvent(eudaq::RawDataEvent::BORE("Taki", m_u_run));

    std::ostringstream os;
    os << "run #" << m_u_run << " started";
    std::string statusMsg = os.str();

    SetStatus( eudaq::Status::LVL_OK, statusMsg.c_str() );
    std::cout << "[TakiProducer::OnStartRun()] " << statusMsg << std::endl;
    m_b_runInProgress = true;
}

void TakiProducer::OnStopRun() 
{
    //SendEvent(RawDataEvent::EORE("DEPFET", m_run, ++m_evt)); //DEPFET EXAMPLE
    SendEvent(eudaq::RawDataEvent::EORE("Taki", m_u_run, ++m_u_ev));

    SetStatus(eudaq::Status::LVL_OK, "run stopped");
    std::cout << "[TakiProducer::OnStopRun()] Run Stopped" << std::endl;
    m_b_runInProgress = false;
    m_b_oldRunJustEnded = true;
}

void TakiProducer::OnTerminate() 
{
    SetStatus(eudaq::Status::LVL_OK, "terminated");
    std::cout << "[TakiProducer::OnTerminate()] Producer now done" << std::endl;
    //std::cout << "[TakiProducer::OnTerminate()] (press enter) (??)" << std::endl;
    m_b_done = true;
}

void TakiProducer::OnReset() 
{
    std::cout << "[TakiProducer::OnReset()] Resetting Producer (ready to start again)" << std::endl;
    SetStatus(eudaq::Status::LVL_OK, "reset");
    m_b_done = false;
    m_b_runInProgress = false;
}

void TakiProducer::OnStatus() 
{

//     std::cout << "[TakiProducer::OnStatus()] NOT PROPERLY IMPLEMENTED YET!" << std::endl;

//     std::cout << "[TakiProducer::OnStatus()] Status reply:" << std::endl;

//     if (m_b_done)
//     {
//         std::cout << "[TakiProducer::OnStatus()] Terminated / Need Configuration." << std::endl;
//     }
//     else
//     {
//         std::cout << "[TakiProducer::OnStatus()] Configured / Not done (not terminated)" << std::endl;
//     }

//     if (m_b_runInProgress)
//     {
//         std::cout << "[TakiProducer::OnStatus()] Run in progress." << std::endl;
//     }
//     else
//     {
//         std::cout << "[TakiProducer::OnStatus()] No run in progress." << std::endl;
//     }

}

void TakiProducer::OnUnrecognised(const std::string & cmd, const std::string & param) 
{
    std::cout << "[TakiProducer::OnUnrecognised()] Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Just testing!");
}

bool TakiProducer::runInProgress() 
{
    return m_b_runInProgress;
}

bool TakiProducer::isDone() 
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
                   		printf("[TakiProducer::safeReceiveTCP()] ERROR reading from socket\n");
                   		return -1;
            }
            if (i_received_bytes_tcp == 0)
            {
                   	 	printf("[TakiProducer::safeReceiveTCP()] Read zero bytes from socket!\n"); 
                   	 	return 0;
            } 
			printf("[TakiProducer::safeReceiveTCP()] Received %d bytes from TCP this time (requested %d Bytes).\n", i_received_bytes_tcp, i_bytesRemainingToReceive);
			i_bytesRemainingToReceive -= i_received_bytes_tcp;
		}
		return 1;
}  

int main(int argc, char* argv[]) 
{
    if (argc>=2) //at least one argument supplied -> send EUDAQ (ZS mode OR FrameMode)
    {
	    
	    // == set correct size for TCP buffer ==
	    int i_bufferlength_TCP;   //= 65536; fixed size
	    if (*argv[1] == 'z') //ZS MODE
	    {
	    	printf("[TakiProducer] Setting TCP buffer to small for ZS data events!\n");
	    	i_bufferlength_TCP = 4*64; //max for now - for ZS DATA (actually a little bit less is needed)
	    }
	    else //FRAME MODE
	    {
	    	printf("[TakiProducer] Setting TCP buffer to big for whole frame data events!\n");
	    	i_bufferlength_TCP  = 3*128*128+4+2; //max for FM DATA
	    }
	
	
        // == setup producer ==
        eudaq::OptionParser optionParser("EUDAQ Producer", "1.0", "A command-line version of a dummy Producer, modified by CT to do some test");
        //eudaq::Option<std::string> rctrl(optionParser, "r", "runcontrol", "tcp://zenpixell3.desy.de:44000", "address", "The address of the RunControl application");
        eudaq::Option<std::string> rctrl(optionParser, "r", "runcontrol", "tcp://localhost:44000", "address", "The address of the RunControl application");
        eudaq::Option<std::string> level(optionParser, "l", "log-level", "NONE", "level", "The minimum level for displaying log messages locally");
        eudaq::Option<std::string> name (optionParser, "n", "name", "SUS_HD_Producer", "string", "The name of this Producer");
        EUDAQ_LOG_LEVEL(level.Value());
	    if (argc >= 3) //read custom runcontroll connection info
	    {
	    	optionParser.Parse(argv+1); //+2 does not work even alothough it makes more sense. because at +1 there should be the other param??
	    }	 	
        TakiProducer producer(name.Value(), rctrl.Value());
	    printf("[TakiProducer] EUDAQ-Producer connected to runconctrol!\n");

	
	    // == init variables ==
        unsigned char* p_uc_buffer_TCP 		     = new unsigned char[i_bufferlength_TCP];
        char*          p_c_buffer_TCP            = (char*) p_uc_buffer_TCP;
        unsigned char  p_uc_fakeEmptyBuffer[7]   = "empty!"; //6 chars + null-termination. going to send the 6 chars only.
        char*          p_c_fakeEmptyBuffer       = (char*) p_uc_fakeEmptyBuffer;
        unsigned int   ui_lengthOfEUDAQData      = 0;

        unsigned long  ul_TID_read = 0UL;
        unsigned long  ul_TID_prev = 0UL-1UL;
        //unsigned long ul_TID_diff = 0UL;

	
	    #define TID_BUFFER_DEPTH 10
	    #define TID_BUFFER_END 9
	    //always set end to depth-1 !!
	    unsigned long p_ul_bufferedTIDs[TID_BUFFER_DEPTH];


        // == SETUP SERVER ==
        printf("[TakiProducer] Setting up TCP server!\n");
        int sockfd, newsockfd, portno, clilen;
        struct sockaddr_in serv_addr, cli_addr;
	    int i_safeReceiveTCP_returnValue = 0;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            printf("[TakiProducer] Error opening socket!\n");
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        //portno = 44005;
        portno = 5901;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("[TakiProducer] ERROR while binding\n");
            return -1;
        }
        std::cout << "[TakiProducer] Socket bound. Starting to listen..." << std::endl;

        // == WAIT FOR CONN ==
        listen(sockfd,5);
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);
        std::cout << "[TakiProducer] Connection found. Trying to accept..." << std::endl;
        if (newsockfd < 0)
        {
            printf("[TakiProducer] Error on accept!\n");
            return -1;
        }
        printf("[TakiProducer] Connection accepted!\n");
        bzero(p_uc_buffer_TCP,i_bufferlength_TCP);

        // == wait for producer to be set up ==
        printf("[TakiProducer] WAITING FOR PRODUCER TO BE SET UP ('config').\n");
        while (producer.runInProgress()==false) ; //wait for producer to be set up!


        // == wait for termination ==
        while (producer.isDone() == false)
        {
            ul_TID_prev = 0UL-1UL; //! if stopRun() and startRun() happen, this line is not(!) bound to be executed in between!

            // == loop: read from chip and send via EUDAQ ==
            while (producer.runInProgress() == true)
            {
                //received_bytes_tcp = read(newsockfd,p_uc_buffer_TCP,i_bufferlength_TCP);
		        //received_bytes_tcp = recv(newsockfd,p_uc_buffer_TCP,i_bufferlength_TCP,0);

                //HACK FOR NOW ***********
                /*
                int received_bytes_tcp = read(newsockfd,p_uc_buffer_TCP,i_bufferlength_TCP);
                if ( received_bytes_tcp == 0)
		        {
			        printf("[TakiProducer()] Read zero bytes from socket!\n");
                    return -1;
                }
		        else if ( received_bytes_tcp < 0)
		        {
			        printf("[TakiProducer()] ERROR reading from socket\n");
			        return -1;
		        } 
                */
                //HACK FOR NOW ***********


                //TODO BUT FIX THIS FOR FRAME READOUT ************
                i_safeReceiveTCP_returnValue = safeReceiveTCP(newsockfd,p_uc_buffer_TCP,i_bufferlength_TCP);
		if ( i_safeReceiveTCP_returnValue == 0)
		        {
                    printf("[TakiProducer()] Read zero bytes from socket (safeReceiveTCP)!\n");
                    return -1;
                }
		        else if ( i_safeReceiveTCP_returnValue < 0)
		        {
                    printf("[TakiProducer()] ERROR reading from socket (safeReceiveTCP)!\n");
			        return -1;
		        }
                //TODO BUT FIX THIS FOR FRAME READOUT ************


		        printf("[TakiProducer] DATA RECEIVED:\n");
		        for (int a=0; a<i_bufferlength_TCP; ++a)
		        {
			        if (a==2) printf("\n");					
      		        if (a==6) printf("\n");										
			        if (a==10+6) a = i_bufferlength_TCP-10;
			        printf("byte# %05d: 0x%02X\n", a, p_uc_buffer_TCP[a]); 
		        }
		        printf("[TakiProducer] DATA RECEIVED END!\n");
		        
							
			   
                ui_lengthOfEUDAQData =    (( ((unsigned int )(p_uc_buffer_TCP[1]))       ) & 0x00FF)
                                        | (( ((unsigned int )(p_uc_buffer_TCP[0])) << 8  ) & 0xFF00); 

                ul_TID_read =   (( ((unsigned long)(p_uc_buffer_TCP[5]))       ) & 0x000000FF)
                              | (( ((unsigned long)(p_uc_buffer_TCP[4])) << 8  ) & 0x0000FF00)
                              | (( ((unsigned long)(p_uc_buffer_TCP[3])) << 16 ) & 0x00FF0000)
                              | (( ((unsigned long)(p_uc_buffer_TCP[2])) << 24 ) & 0xFF000000);  

                //! The readout PC does not send any 0 TIDs or TIDs > 32767 ! (making it easier for us to remove garbled TIDs)
                //! So if a very high TID is received here, we just ignore it, because it must be garbage TCP data resulting from
                //! too much data arring in the TCP buffer. In that case we receive very high numbers sometimes??
		
		        if (ul_TID_read != ul_TID_prev+1UL) //TID seems weird 
		        {
			        printf("[TakiProducer] Received WEIRD  TID: %lu\n",ul_TID_read);
			        p_ul_bufferedTIDs[0] = ul_TID_read;
			        for (int i=1;i<TID_BUFFER_DEPTH;++i)
			        {
                        //HACK FOR NOW ***********
                        /*
                        received_bytes_tcp = read(newsockfd,p_uc_buffer_TCP,i_bufferlength_TCP);
                        if ( received_bytes_tcp == 0)				
		                {
			                printf("[TakiProducer()] Read zero bytes from socket!\n");
                    		return -1;
                    	} 
		                else if ( received_bytes_tcp < 0) 
		                {
			               printf("[TakiProducer()] ERROR reading from socket\n");
			               return -1;
		                }  
                        */
                        //HACK FOR NOW ***********  

                        //TODO BUT FIX THIS FOR FRAME READOUT ************
                        i_safeReceiveTCP_returnValue = safeReceiveTCP(newsockfd,p_uc_buffer_TCP,i_bufferlength_TCP);
				        if ( i_safeReceiveTCP_returnValue == 0)				
				        {
                            printf("[TakiProducer()] Read zero bytes from socket (safeReceiveTCP)!\n");
               				return -1;
               			}
				        else if ( i_safeReceiveTCP_returnValue < 0)
				        {
                            printf("[TakiProducer()] ERROR reading from socket (safeReceiveTCP)!\n");
					        return -1;
				        }
                        //TODO BUT FIX THIS FOR FRAME READOUT ************


				        p_ul_bufferedTIDs[i] =    (( ((unsigned long)(p_uc_buffer_TCP[5]))       ) & 0x000000FF)
                		               		    | (( ((unsigned long)(p_uc_buffer_TCP[4])) << 8  ) & 0x0000FF00)
		                             		    | (( ((unsigned long)(p_uc_buffer_TCP[3])) << 16 ) & 0x00FF0000)
		                             		    | (( ((unsigned long)(p_uc_buffer_TCP[2])) << 24 ) & 0xFF000000);
			        }
			
			        bool bIsAscending = true;
			        for (int i=1;i<TID_BUFFER_DEPTH;++i) //TODO: BUGFIX!! if a 0 comes in between, we think, that a new run has started
			        {	                                 //TODO: TAKE (NEW RUN HAS STARTED MESSAGE)/(ONSTARTRUN)/(ONSTOPRUN) AS A SIGNAL TO START A NEW RUN!! SHOULD BE EASY AND SAFE
				        if (p_ul_bufferedTIDs[i-1] >= p_ul_bufferedTIDs[i])
				        {	
					        bIsAscending = false;
					        break;		
				        }   	
			        }   
			
			        if (bIsAscending==true) //this means: new run has indeed started
			        {
				        if (ul_TID_read < ul_TID_prev) 
				        {
					        printf("[TakiProducer] ...Looks like a new run has started!\n");
					        printf("[TakiProducer] ...Sending empty events: TIDs %d to	%lu!\n",0,p_ul_bufferedTIDs[TID_BUFFER_END]);
					        for (unsigned long i=0UL;i<=p_ul_bufferedTIDs[TID_BUFFER_END];++i)
	                        	{
	                        	    producer.send(p_c_fakeEmptyBuffer, 6, i); 
	                        	}
				        }
				        else //ul_TID_read is bigger than ul_TID_prev by at least 2, so there seems to be one or more TIDs missing
				        {
					        printf("[TakiProducer] ...Looks like there is a TID missing!\n");
					        printf("[TakiProducer] ...Sending empty events: TIDs %lu to %lu!\n",ul_TID_prev+1UL,p_ul_bufferedTIDs[TID_BUFFER_END]);
					        for (unsigned long i=ul_TID_prev+1UL;i<=p_ul_bufferedTIDs[TID_BUFFER_END];++i)
	                        	{
	                        	    producer.send(p_c_fakeEmptyBuffer, 6, i);
	                        	}	
				        }
			        }
			        else //this means: garbled TID read before
			        {
				        printf("[TakiProducer] ...Looks like a garbled/zero/missing TID occured!\n");
				        printf("[TakiProducer] ...Sending empty events: TIDs %lu to %lu!\n",ul_TID_prev+1UL,p_ul_bufferedTIDs[TID_BUFFER_END]);
				        for (unsigned long i=ul_TID_prev+1UL;i<=p_ul_bufferedTIDs[TID_BUFFER_END];++i)
                        	{
                        	    producer.send(p_c_fakeEmptyBuffer, 6, i);
                        	}
			        }
			        ul_TID_prev = p_ul_bufferedTIDs[TID_BUFFER_END];
		        }
		        else //TID seems okay
		        {   
		            //printf("[TakiProducer] Received proper TID: %lu\n",ul_TID_read);
		            //printf("[TakiProducer]  Sending event with length: %u\n",ui_lengthOfEUDAQData);
                    producer.send(p_c_buffer_TCP, ui_lengthOfEUDAQData, ul_TID_read);
               	    ul_TID_prev = ul_TID_read;
	    	    }
            } 
        }
	    delete[] p_uc_buffer_TCP;
       	close(newsockfd);
    	close(sockfd);
    }
    else //no arguments supplied, return because arguments are needed !OR! debug output only!
    {
        printf("[TakiProducer] no arguments supplied! usage: ((( ./TakiProducer.exe [z/f] [-r tcp://runcontrolhost:port] )))\n");
        printf("[TakiProducer] no arguments supplied! debug output only! ... NOT IMPLEMENTEOD PROPERLY TCP RECEIVE HAS GOT TO BE FIXED!\n");
        return -1;

        //const int i_bufferlength_TCP = 65536;
        const int     i_bufferlength_TCP                = 4*64; //max for now
        unsigned char p_uc_buffer_TCP[i_bufferlength_TCP];
        unsigned int  ui_lengthOfEUDAQData              = 0;

        unsigned long ul_TID_read = 0UL;
        unsigned long ul_TID_prev = 0UL-1UL;
        unsigned long ul_TID_diff = 0UL;

        // == SETUP SERVER ==
        printf("[TakiProducer] Setting up TCP server!\n");
        int sockfd, newsockfd, portno, clilen;
        struct sockaddr_in serv_addr, cli_addr;
        int received_bytes_tcp;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            printf("[TakiProducer] Error opening socket!\n");
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        //portno = 44005;
        portno = 5901;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("[TakiProducer] ERROR while binding\n");
            return -1;
        }
        std::cout << "[TakiProducer] Socket bound. Starting to listen..." << std::endl;

        listen(sockfd,5);
        clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);
        std::cout << "[TakiProducer] Connection found. Trying to accept..." << std::endl;
        if (newsockfd < 0)
        {
            printf("[TakiProducer] Error on accept!\n");
            return -1;
        }
        printf("[TakiProducer] Connection accepted!\n");
        bzero(p_uc_buffer_TCP,i_bufferlength_TCP);

        ul_TID_prev = 0UL-1UL;

        unsigned int iterationsCounter = 0;
        // == do it only 50000 times ==
        while(iterationsCounter++<50000)
        {
                received_bytes_tcp = read(newsockfd,p_uc_buffer_TCP,i_bufferlength_TCP);
                if (received_bytes_tcp < 0)
                {
                    printf("[TakiProducer] ERROR reading from socket\n");
                    return -1;
                }
                if (received_bytes_tcp == 0)
                {
                    printf("[TakiProducer] Read zero bytes from socket!\n");
                    continue;
                }

                ui_lengthOfEUDAQData =    (( ((unsigned int )(p_uc_buffer_TCP[1]))       ) & 0x00FF)
                                        | (( ((unsigned int )(p_uc_buffer_TCP[0])) << 8  ) & 0xFF00);

                ul_TID_read =   (( ((unsigned long)(p_uc_buffer_TCP[5]))       ) & 0x000000FF)
                              | (( ((unsigned long)(p_uc_buffer_TCP[4])) << 8  ) & 0x0000FF00)
                              | (( ((unsigned long)(p_uc_buffer_TCP[3])) << 16 ) & 0x00FF0000)
                              | (( ((unsigned long)(p_uc_buffer_TCP[2])) << 24 ) & 0xFF000000);

                //! The readout PC does not send any 0 TIDs or TIDs > 32767 ! (making it easier for us to remove garbled TIDs)
                //! So if a very high TID is received here, we just ignore it, because it must be garbage TCP data resulting from
                //! too much data arring in the TCP buffer. In that case we receive very high numbers sometimes??
                if (ul_TID_read > 32767) //erroreous TID received because TCP buffer garbled??
                {
                    continue; //the prev proper TID stays the same
                }

                ul_TID_diff = ul_TID_read - ul_TID_prev; //should always be 1 if everything went fine

                //send fake events for all missing TIDs. otherwise the online monitor will associate
                //wrong pairs of hits with each other!! (really? check that again!)
                if (ul_TID_diff != 1)
                {
                    std::cout << "[TakiProducer] RCVD TID: " << ul_TID_read << ". Number of TIDs MISSING: " << ul_TID_diff-1 << " !!!" << std::endl;
                }
                else
                {
                    std::cout << "[TakiProducer] RCVD TID: " << ul_TID_read << " correctly." << std::endl;
                }
                ul_TID_prev = ul_TID_read;
        }
       	close(newsockfd);
    	close(sockfd);
    }
	return 0;
}
