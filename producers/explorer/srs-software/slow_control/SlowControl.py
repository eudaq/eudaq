import socket # networking stuff
import srsCommon # common routines (setup environment using . ../env.sh)

# slow control object which keeps a socket open for a single FEC
class SlowControl:
    # constructor
    def __init__(self, fec_id):
        self.src_port = 6007
        self.fec_id = fec_id
        self.req_id = 0
        self.dst_ip = '10.0.{0:d}.2'.format(self.fec_id)
        self.src_ip = '10.0.{0:d}.3'.format(self.fec_id)

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.src_ip, self.src_port))

    # send method
    def send(self, req, expectRply = True):
        self.req = req
        # check whether the request is valid
        if len(req.addresses) == 0:
            print 'no addresses in request' \
                  '(fec_id {0:d}, req_id {1:d})'.format(self.fec_id,
                                                        self.req_id)
            return ScReq()
        # determine the slow-control mode
        self.handle_mode()
        # assemble the message to be send to the device
        self.assemble_message()
        # send the message
        self.sock.sendto(self.message, (self.dst_ip, self.req.dst_port))
        if expectRply:
            # read the reply
            self.rply, self.rply_addr = self.sock.recvfrom(self.rply_len)
            # process the reply
            self.process_rply()
            self.req_id += self.req_id + 1
        else:
            self.sc_rply = ScRply()
            self.sc_rply.success = True # without reply, can't judge
        return self.sc_rply

    # setup the communication mode
    def handle_mode(self):
        # determine mode
        if self.req.write: # write request
            if len(self.req.addresses) == 1:
                self.mode = 0 # write burst
            else:
                self.mode = 1 # write pairs
        else: # or read request
            if len(self.req.addresses) == 1:
                self.mode = 2 # read burst
            else:
                self.mode = 3 # read pairs
        # setup the communication according to the current mode
        if self.mode == 0: # write burst
            self.cmd      = 0xAA
            self.cmd_type = 0xBB
            self.cmd_info = self.req.addresses[0]
        elif self.mode == 1: # write list
            self.cmd      = 0xAA
            self.cmd_type = 0xAA
            self.cmd_info = 0x0 # 0xDEADBEEF
            if len(self.req.data) != len(self.req.addresses):
                print 'for writing a list an equal count of ', \
                      'addresses and data words is required!', \
                      '(fec_id {0:d}, req_id {1:d})'.format(self.fec_id,
                                                            self.req_id)
        elif self.mode == 2: # read burst
            self.cmd      = 0xBB
            self.cmd_type = 0xBB
            self.cmd_info = self.req.addresses[0]
        elif self.mode == 3: # read list
            self.cmd      = 0xBB
            self.cmd_type = 0xAA
            self.cmd_info = 0x0 # 0xDEADBEEF
        else:
            print 'incorrect slow-control mode!', \
                  '(fec_id {0:d}, req_id {1:d})'.format(self.fec_id,
                                                        self.req_id)

    # assemble the message to be send to the device
    def assemble_message(self):
        # header part
        self.message  = srsCommon.its((self.req_id | 0x80000000), 4)
        self.message += srsCommon.its(self.req.subaddress, 4)
        self.message += srsCommon.its(self.cmd, 1)
        self.message += srsCommon.its(self.cmd_type, 1)
        self.message += srsCommon.its(0xFFFF, 2)
        self.message += srsCommon.its(self.cmd_info, 4)
        self.rply_len = 16
        # payload part
        if self.mode == 0 or self.mode == 2:
            self.rply_len += 2*len(self.req.data)*4
            for d in range(len(self.req.data)):
                self.message += srsCommon.its(self.req.data[d], 4)
        elif self.mode == 1:
            self.rply_len += 2*len(self.req.data)*4
            for d in range(len(self.req.data)):
                self.message += srsCommon.its(self.req.addresses[d], 4)
                self.message += srsCommon.its(self.req.data[d], 4)
        elif self.mode == 3:
            self.rply_len += 2*len(self.req.addresses)*4
            for a in range(len(self.req.addresses)):
                self.message += srsCommon.its(self.req.addresses[a], 4)

    # parse the rply string and check it for errors
    def process_rply(self):
        self.sc_rply = ScRply()
        # clear the lists in the reply
        del self.sc_rply.errors[:]
        del self.sc_rply.data[:]
        # reply length
        if len(self.rply) != self.rply_len:
            print 'error incorrect reply length!', \
                  '(fec_id {0:d}, req_id {1:d})'.format(self.fec_id,
                                                        self.req_id)
        # request id
        if srsCommon.sti(self.rply[0:4], 4) != self.req_id:
            print 'incorrect request id replied!', \
                  '(fec_id {0:d}, req_id{1:d})'.format(self.fec_id,
                                                        self.req_id)
        # subaddress, cmd, cmd type and cmd info are not checked
        for d in range(16, self.rply_len, 2*4):
            self.sc_rply.errors.append(srsCommon.sti(self.rply[d:d+4], 4))
            self.sc_rply.data.append(srsCommon.sti(self.rply[d+4:d+8], 4))
        # everything is fine...
        self.sc_rply.success = True

# slow-control request descriptor
class ScReq:
    def __init__(self):
        self.dst_port = 6007
        self.subaddress = 0xFFFFFFFF
        self.addresses = list()
        self.data = list()
        self.write = False

# slow-control reply information
class ScRply:
    def __init__(self):
        self.success = False
        self.errors = list()
        self.data = list()
    def __str__(self):
        message = str()
        if len(self.errors) != len(self.data):
            print "unequal number of data and errors values"
        else:
            message += '# Word\tError\t\tData\n'
            for i in range(len(self.data)):
                message += '%d\t' % i
                message += '0x%0.8x\t' % self.errors[i]
                message += '0x%0.8x\t' % self.data[i]
                message += '(%10d)\n' % self.data[i]
        return message


## wrapper functions printing the reply on stdout
def read_burst(sc, dst_port, start_address, length, silent = True):
    req = ScReq()
    req.dst_port = dst_port
    req.addresses = [ start_address ]
    for i in range(length):
        req.data.append(0x0)
    rply = sc.send(req)
    if not silent:
        print str(rply)
    return rply

def read_list(sc, dst_port, addresses, silent = True):
    req = ScReq()
    req.dst_port = dst_port
    req.addresses = addresses
    rply = sc.send(req)
    if not silent:
        print str(rply)
    return rply

def write_burst(sc, dst_port, start_address, data, silent = True, expectRply = True):
    req = ScReq()
    req.dst_port = dst_port
    req.addresses = [ start_address ]
    req.data = data
    req.write = True
    rply = sc.send(req, expectRply)
    if not silent:
        print str(rply)
    return rply

def write_list(sc, dst_port, addresses, data, silent = True, expectRply = True):
    req = ScReq()
    req.dst_port = dst_port
    req.addresses = addresses
    req.data = data
    req.write = True
    rply = sc.send(req, expectRply)
    if not silent:
        print str(rply)
    return rply
