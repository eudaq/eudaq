#!/usr/bin/env python

import socket
import sys
from thread import *
from blink import *

#HOST = '128.141.89.137'
HOST = '127.0.0.1'
PORT = 12345

Blink(8, 3,0.1)

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

def clientthread(conn):
    #Sending message to connected client
    conn.send('Welcome to the server. Receving Data...\n')

    #infinite loop so that function do not terminate and thread do not end.
    while True:
        #Receiving from client
        data = conn.recv(1024)
        reply = 'Message Received at the server: %s\n' % str(data)
        
        if not data:
            break
        conn.sendall(reply)
        
        print 'Command recieved:', str(data), type(data)
        if str(data)=='blink':
            print "I am doing blink now"
            Blink(8, 10,0.2)        
    conn.close()
            
#now keep talking with the client

try:
    while True:
        # wait to accept a connection
        conn, addr = s.accept()
        print 'Connected with ' + addr[0] + ':' + str(addr[1])

        #start new thread
        start_new_thread(clientthread ,(conn,))

finally:
    s.close()
    Blink(8, 3,1)
