#!/usr/bin/env python

import socket   
import sys 
import struct
import time

def recv_timeout(the_socket,timeout=2):
    #make socket non blocking
    the_socket.setblocking(0)

    #total data partwise in an array
    total_data=[];
    data='';

    #beginning time
    begin=time.time()
    while 1:
	print 'I am whiling in the loop'

        print "Time since begin:", time.time()-begin

        #if you got some data, then break after timeout
        if total_data and time.time()-begin > timeout:
            print 'got data, break the while loop'
            break

        #if you got no data at all, wait a little longer, twice the timeout
        elif time.time()-begin > timeout*2:
            print 'Timout bro'
            break

        #recv something
        try:
            data = the_socket.recv(8192)
            if data:
                total_data.append(data)
		print 'Recieved: ', data
                #change the beginning time for measurement
                begin=time.time()
            else:
                #sleep for sometime to indicate a gap
                time.sleep(0.2)
        except:
            time.sleep(0.2)
            pass

    #join all parts to make final string
    return ''.join(total_data)

if __name__ == "__main__":

  print sys.argv
  if len(sys.argv) < 2:
    print 'Usage : python tcpclient.py command'
    sys.exit()
    
  host = 'localhost'
  command = sys.argv[1]
  port = 55511

  #create an INET, STREAMing socket
  try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  except socket.error:
    print 'Failed to create socket'
    sys.exit()

  print 'Socket Created'

  try:
    remote_ip = socket.gethostbyname( host )
    print host, remote_ip
    s.connect((host, port))

  except socket.gaierror:
    print 'Hostname could not be resolved. Exiting'
    sys.exit()

  except socket.error:
    print 'Some other socket error', socket.error
    sys.exit()

  print 'Socket Connected to ' + host + ' on ip ' + remote_ip

  #Send some data to remote server
  try :
    #Set the whole string
    s.send(command)
    print 'Sending...'
    time.sleep(1)
    print 'Message sent successfully'

  except socket.error:
    #Send failed
    print 'Send failed'
    sys.exit()


  #get reply and print
  m=0
  while m<10:
    print recv_timeout(s, 0.5)
    m+=1

  #s.close()



