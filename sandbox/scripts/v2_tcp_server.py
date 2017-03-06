#!/usr/bin/env python

import socket
import sys
from thread import *
import time

# HOST = '128.141.89.137'
HOST = '127.0.0.1'
PORT = 55511

#stop_it = 0

def clientthread(conn):
    #Sending message to connected client
    #conn.send('Welcome to the server. Receving Data...\n')

    #infinite loop so that function do not terminate and thread do not end.
    while True:
        #Receiving from client
        data = conn.recv(1024)
        
        if not data:
            break
        
        print 'Command recieved:', str(data), type(data)
        if str(data)=='blink':
            print "I am doing blink now"
            #Blink(8, 10,0.2)        
        elif str(data)=='START_RUN':
            conn.sendall('GOOD_RUN\n')
            time.sleep(0.2)
            # Start sending data
            #while not stop_it:
            m = 0
            while m<20:
                print 'submitting data', m
                conn.sendall('\t -->>>  Here is your data')
                time.sleep(1)
                m+=1

        elif str(data)=='STOP_RUN':
            stop_it = 1
            conn.sendall('STOPPED_RUN\n')
            print 'Sent STOPPED_RUN confirmation'
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
