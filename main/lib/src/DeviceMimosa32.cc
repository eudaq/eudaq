#include "eudaq/DeviceMimosa32.hh"
#include <sys/types.h>
#include <fcntl.h> // File control definitions
#include <errno.h> // Error number definitions
#include <iostream>
#include <fstream> 
#include <string> 

// il numero di byte che dovrebbe leggere e' 2608 sovradimensionato
#define mtu_date 3000

// legge word di 4 byte
#define mtu_xport 4 

using namespace std;
DeviceMimosa32::DeviceMimosa32()
{
}

DeviceMimosa32::~DeviceMimosa32()
{
}

//Crea un TCP Client per la comunicazione con il dispositivo. 
//Richiamare durante la fase di configurazione.
//Imposta il valore di fSD Socket Descriptor.
//Restituisce -1 se fallisce la creazione del socket. Oppure il descrittore del socket. 
//----------------------------------------------------------------------------------------------
int DeviceMimosa32::create_client_tcp(int port,char *ip){
    printf ("ip_xport %s port_xport %d\n",ip,port);	
    fClientAddr.sin_family	    = AF_INET;
    fClientAddr.sin_port	    = htons((u_short)port);
    fClientAddr.sin_addr.s_addr     = inet_addr(ip);	     
    if((fSD = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	perror ("Created client TCP .................... FAILED ");
    	return -1;	
    	}
    printf("Created Client TCP ................... OK\n");    
    return fSD;   
}

//Connette il Client al Server del dispositivo. 
//Richiamare durante la fase di Start di un run.
//Prende in ingresso il descrittore del socket.
//Restituisce -1 se ci sono problemi durante la fase di connessione.
//----------------------------------------------------------------------------------------------
int DeviceMimosa32::connect_client_tcp(int sd){	
    if(connect(sd, (struct sockaddr *)&fClientAddr, sizeof(fClientAddr)) < 0){	
       perror("Connected from client TCP to Xport..... FAILED ");
       exit(0);
       return -1;		  
       }
    printf("Connected Client TCP ................. OK\n"); 
    return 1;  
}

//Chiude la connessione con il dispositivo. 
//Da richiamare durante la fase di Stop di un run.
//Non restituisce nulla.
//Prende in ingresso il descrittore del socket.
//----------------------------------------------------------------------------------------------
void DeviceMimosa32::close_client_tcp(int sd){
	 close(sd);
	 printf("Closed Client TCP .................... OK\n");	
} 

//Legge i dati dal file di configurazione Cfg_mimosa32.txt.
// Da richiamare durante la fase di configurazione.
// Il file di configurazione deve essere formatato nel seguente modo:
// # riga di commento.
//IP: indirizzo ip della ZRC
//PORT: Numero della porta dove mi aspetto che arrivino i dati.
// scrive il valore delle variabili fIp e fPort.
//----------------------------------------------------------------------------------------------
int DeviceMimosa32::read_file_cfg(){
    ifstream file("../MimosaProducer/src/");
    string line;
    int count=0;
    int isIp=0;
    int isPort=0; 
    int isNumChip=0;
    char * word;
    char char_ip[]="IP";
    char char_port[]="PORT";
    char char_num_chip[]="NUM_CHP";
    if(!file) {
       cout<<"The File is not exist!";
       return -1;
    }

    while(file.good()){
          getline(file, line);
         // cout<<line<<endl;
          count=0;
          isIp=0;
          isPort=0;       
          word= strtok ((char*) line.c_str(),": ");
          while (word != NULL){
     	         if (isIp==1)  {fIp=word; isIp=0;}
     	         if (isPort==1){ fPort=atoi(word);isPort=0;}
		 if (isNumChip==1){ fNumChip=atoi(word);isNumChip=0;}
     	         if (count==0 && strcmp(word,char_ip)   == 0) {isIp=1;}
     	         if (count==0 && strcmp(word,char_port) == 0 ){isPort=1;}
		 if (count==0 && strcmp(word,char_num_chip)== 0){isNumChip=1;}
     	         word= strtok (NULL, ": ");
     	         count++;
     	         }
    }
    fNumChip=1;
    file.close(); //chiude il file
    return 1;
}
//Legge un evento. La prima word contiene il numero di word dell'evento.
//Dalla word numero 2 alla word numero 9 abbiamo il CDH che viene salvato nella stringa fCDH.
//Dalla word numero 10 alla word numero 13 abbiamo il DH che viene salvato nella stringa fDH.
//Dalla word numero 14 in poi abbiamo i dati che vengono savati nella stringa fData.
//l'evento completto di CDH, DH e Dati viene salvato nella stringa fAllData.
//La stringa data_inv contienele stesse informazioni di fAllData e viene usata per la conversione
//dei dati in esadecimale che poi vengono scritti nel file.
//Restituisce 1 quando termina un evento. Viene passato il file dove scrivere i dati.
//----------------------------------------------------------------------------------------------
int DeviceMimosa32::readout_event(int num_word_event, FILE *fp, char *cdh, char *dh, char *all_data){        
    char data[mtu_xport];// word di 4byte ricevuta dal detector.
    char data_inv[mtu_date];// word di 4byte ricevuta dal detector invertita.
    int count_word=0;//Numero di word
    int i,n; // d e n contattori decrescenti usati rispettivamente per la stringa istruzione e per la stringa dati.
    int num_byte_event=0; //Numero totale di byte presi durante l'evento.
    int pos_start=0;//indice di posizione di inizio per la memorizzazione della word nella struttura dati.
    int pos_end=3;//indice di posizione di fine per la memorizzazione della word nella struttura dati.
    int tot_byte_rcv=0;//Conteggio del totale dei byte ricevuti
    int byte_rcv=0;//Numero di byte ricevuti dal dispositivo.
    int id_event=0;//Id dell'evento.	    

//inizializza a 0 tutte le stringhe	
    //memset(&data,0,sizeof(data));   
    // memset(&data_inv,0,sizeof(data_inv));   
    // memset(&cdh,0,sizeof(cdh));
    //memset(&dh,0,sizeof(dh));
    //memset(&all_data,0,sizeof(all_data));
    //printf("Size of CDH : %d\n",sizeof(cdh));
    		     
    while (1){
    	   count_word++;		     
//RICEZIONE DEI DATI			    			   
	   byte_rcv=recv(fSD,data,mtu_xport,0);
//WORD DI ISTRUZIONE			     
	   if (count_word==1) {
	       num_byte_event=num_word_event *4;
	       printf ("  Num word for this event are: %d\n", num_word_event);  				 
	       printf ("  Num byte for this event are: %d\n", num_byte_event);
	       printf("\n---------------- Event Number ----------------\n");			       
	       }
//INSERIMENTO DEI VALORI NELLA STRUTTURA DATI			      
	   if (count_word!=1){ 
	       n=mtu_xport; 
	       for (i=pos_start; i<=pos_end; i++){
	   	    n--;			  
	   	    data_inv[i]=data[n]; // dati completti di CDH e DH
		    // printf ("test scrittura Data Inv [%d] %02X \n",i,(unsigned char)data_inv[i]);
		    if ( count_word > 1 && count_word < 10) *cdh++ = data[n];//cdh
		    // printf ("test scrittura cdh [%d] %02X \n",i,(unsigned char)cdh[i]);
		    if ( count_word > 9 && count_word < 14) *dh++ = data[n];//dh
		    //  printf ("test scrittura dh [%d] %02X \n",i,(unsigned char)dh[i]);
		    if ( count_word > 13 ) *all_data++ = data[n];//data
		    //  printf ("test scrittura all_data [%d] %02X \n",i,(unsigned char)all_data[i]);	 
	   	    } 
	       pos_start=pos_start+4;
	       pos_end=pos_end+4;
	       }
//CONTATORE DI BYTE RICEVUTI.			      
	   if (count_word!=1 && num_word_event<=count_word){				   
	       tot_byte_rcv=tot_byte_rcv + byte_rcv;
	       }	 
//FINE DI UN EVENTO			       			  
	   if ( num_word_event==count_word-1) {    //-1 da verificare
	     print_data(data_inv, pos_end, fp);			     
	    	printf ("\n ----------------------- End Event %i -----------------------\n",id_event);
	    	return 1;
	    	}
	   } //end while							    
    }

//Stampa i pacchetti  e scrittura su file.
//Viene passato il vettore di caratteri data che contiene i dati dell'evento comprensivi di CDH e DH.
//La posizione finale della stringa e il puntatore al file descriptor.
//La funzione non restituisce nulla.
//----------------------------------------------------------------------------------------------
void DeviceMimosa32::print_data(char *data, int pos_end, FILE * /*fp*/){
     int num_word=0; // Count num word
     int c=0;        // flag	
     int i;          // iterator for
     int n=0;
     pos_end =pos_end -3;//o -4 da verificare
     printf("0001     ");    
     for (i=0; i< pos_end; i++){
          n++;	               					 
	  printf ("%02X",(unsigned char)data[i]);
	  if (n%4 == 0 ){
	      printf ("     ");
	      num_word++;
	      c=2;		     
	      }
	     if (c==2){
	         c=1;
	         if (num_word%4==0){ 
		     printf("\n");
		     printf ("%04X     ",num_word);
		     }
	      } 		      	       		 		 		                
	  }	  
      }
//Numero di word presenti nell'evento.
//In buffer e' contenuta la word istruzione.
//All'interno della funzione viene scritto il valore num_word_event tramite l'uso di un puntatore.
//----------------------------------------------------------------------------------------------
 int DeviceMimosa32::get_num_word_event(){     
     int num_word=-1;   
     if (fNumChip==1)  num_word=652;
     return num_word;
     } 
  
//Un carattere 0 o 1 deve essere convertito in bit di 0 o 1 per essere mandato al dispositivo.
//Una stringa istruzione che viene e' composta da 4 byte ovvero 32 bit ovvero32 caratteri.
//Questi caratteri contenuti nella stringa buffer vengono convertiti in bit e inseriti nella stringa
//word che avra' dimensione 4.
//-------------------------------------------------------------------------------------------------
int DeviceMimosa32::from_carachter_to_word(char *buffer, char *word){
    int i, a, b ,flag , num_byte,show_data_string;
    i=a=flag=num_byte=show_data_string=0;
    b=0;
    word[a]=0;
    printf("    Instruction String: ");
    while(buffer[i] != '\0'){
        if(a==4 && show_data_string==0){ 
           printf("\n    Data String:        ");
	   show_data_string=1;
	   } 
	flag=1;        
	printf ("%c",buffer[i]);
	word[a]=(word[a])+((buffer[i]-48)<<b);
	i++;
	b++;
	if (b==8){
	    //printf ("	BYTE: %02X\n",(unsigned char)word[a]);
	    printf (" ");
	    a++;
	    word[a]=0;
	    b=0;
	    flag=2;		   
	    }	  	  
    }
    if(flag==1) num_byte=a+1;
    if(flag==2) num_byte=a;
    if (flag==1){
        //printf ("   BYTE: %02X\n",(unsigned char)word[a]);
	printf (" ");
	}
    printf ("\n");	
    return (num_byte);
}

//Invia un istruzione al dispositivo e rilegge la conferma di istruzione.
// Se la conferma non arriva dopo due secondi va in timeout. Word contiena la word di istruzione da spedire.
//-------------------------------------------------------------------------------------------------
int DeviceMimosa32::send_instruction(char *word){
     int byte_send;
     int byte_rcv;
     char read_byte[4];
     fd_set set;
     struct timeval timeout;
     FD_ZERO(&set);
     timeout.tv_sec = 2;
     timeout.tv_usec = 0;
     FD_SET(fSD, &set);
     int rv;
     byte_send=send (fSD,word,4,0);
     if(byte_send < 1) return -1;
     rv = select(fSD+ 1, &set, NULL, NULL, &timeout);
     if(rv == -1){ perror("select: "); return -1;} /* an error accured */
     else if(rv == 0) {/* a timeout occured */
          printf("TIMEOUT, ZRC or Aux don't answere.\n");
          return 0;
          } 
     else{
         byte_rcv=recv(fSD,read_byte,4,0);	     
         printf("  Number byte received from device %d \n",byte_rcv);
	 return 1;
	 }
     return -1;	 
     }

//Commando di Start run. restituisce 1 se va a buon fine. -1 se fallisce.
//-------------------------------------------------------------------------------------------------
int DeviceMimosa32::cmdStartRun(){
     char start_string[]="01011000000000000000000000000010";
     char word[4];
     int num_byte;
     int control;
     memset(&word,0,sizeof(word));  
     num_byte=from_carachter_to_word(start_string,word);
     if (num_byte<=0) return -1;
     control=send_instruction(word);
     if (control<0) return -1;
     if (control==0) return 0;
     printf("\n----------------------- Start Run ----------------------\n");
     return 1;
     }
     
//Commando di Stop run. restituisce 1 se va a buon fine. -1 se fallisce.  
//-------------------------------------------------------------------------------------------------
int DeviceMimosa32::cmdStopRun(){
     char stop_string[]="11011000000000000000000000000000";
     char word[4];
     int num_byte;
     int control;
     memset(&word,0,sizeof(word)); 
     num_byte=from_carachter_to_word(stop_string,word);
     if (num_byte<=0) return -1;
     control=send_instruction(word);
     if (control<=0) return -1;
     printf("\n----------------------- Stop Run ----------------------\n");
     return 1;	
     }  
// Restituisce il numero di chip     
//-------------------------------------------------------------------------------------------------     
int  DeviceMimosa32::get_num_chip(){
     return fNumChip;
     }
// Restituisce il numero di chip     
//-------------------------------------------------------------------------------------------------     
int  DeviceMimosa32::get_port(){
     return fPort;
     }
 // Restituisce il numero ip    
//-------------------------------------------------------------------------------------------------    
void DeviceMimosa32::get_ip(char *ip){
     strcpy(ip,fIp);
     }
