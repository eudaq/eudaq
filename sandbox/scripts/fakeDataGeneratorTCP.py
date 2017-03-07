#!/usr/bin/env python

import socket
import sys
from thread import *
import time

# HOST = '128.141.89.137'
HOST = '127.0.0.1'
PORT = 55511

def clientthread(conn):
    #Sending message to connected client
    #conn.send('Welcome to the server. Receving Data...\n')
    
    #infinite loop so that function do not terminate and thread do not end.

    global stop_it 
    while True:
        #Receiving from client
        try:
            data = conn.recv(1024)
        except socket.error:
            print 'The client socket is probably closed. break this thread'
            break

        if not data:
            break
        
        print 'Command recieved:', str(data), type(data)
        if str(data)=='blink':
            print "I am doing blink now"
            #Blink(8, 10,0.2)        
        elif str(data)=='START_RUN':
            conn.sendall('GOOD_START\n')
            time.sleep(0.2)
            stop_it = 0

            m = 0
            while not stop_it:
                print 'submitting data', m
                try:
                    conn.sendall('-->>>  Here is your data <<< --')
                except socket.error:
                    print 'The client socket is probably closed. We stop sending data.'
                    stop_it = 1
                    
                time.sleep(0.5)
                m+=1

        elif str(data)=='STOP_RUN':
            stop_it = 1
            conn.sendall('STOPPED_OK\n')
            print 'Sent STOPPED_OK confirmation'
        else:
            conn.sendall('Message Received at the server: %s\n' % str(data))


    conn.close()
            

if __name__ == "__main__":

  # blink is only for RPI tests
  # from blink import *

  #Blink(8, 3,0.1)

  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  print 'Socket created'

  try:
    s.bind((HOST, PORT))
  except socket.error , msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()

  print 'Socket bind complete'

  s.listen(3)
  print 'Socket now listening'


  try:
    while True:
      # wait to accept a connection
      conn, addr = s.accept()
      print 'Connected with ' + addr[0] + ':' + str(addr[1])

      #start new thread
      start_new_thread(clientthread ,(conn,))

  finally:
    s.close()
    
    #Blink(8, 3,1)
