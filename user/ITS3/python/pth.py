import usb

class PTH:
    VID=0x289B
    PID=0x0505
        
    def __init__(self):
        self.dev=usb.core.find(idVendor=self.VID,idProduct=self.PID)
        assert self.get_channels()=={0:0x15,1:0x20,2:0x19}

    def cmd(self,cmd_,id_,n=8):
        assert cmd_|0xFF==0xFF
        res=self.dev.ctrl_transfer(1<<7|2<<5|0<<0,cmd_,id_,0,n)
        assert len(res)>=2
        assert res[0]==cmd_
        #print(len(res),'-'.join('%02X'%b for b in res))
        xor=0
        for b in res: xor^=b
        assert xor==0
        return res[1:-1]

    def get_channels(self):
        nch=int(self.cmd(0x02,0)[0]) # get number of channles
        chs={}
        for i in range(nch):
            chs[i]=int(self.cmd(0x01,i)[0]) # get chip id
        return chs
        
    def getT(self):
        ret=self.cmd(0x10,1) #get raw
        raw=int(ret[0]<<8|ret[1])
        return -45 + 175 * raw/65535

    def getH(self):
        ret=self.cmd(0x10,2) #get raw
        raw=int(ret[0]<<8|ret[1])
        return 100 * raw/65535

    def getP(self):
        ret=self.cmd(0x11,0)+self.cmd(0x11,1) #get cal
        C1=ret[ 0]|ret[ 1]<<8
        C2=ret[ 2]|ret[ 3]<<8
        C3=ret[ 4]|ret[ 5]<<8
        C4=ret[ 6]|ret[ 7]<<8
        C5=ret[ 8]|ret[ 9]<<8
        C6=ret[10]|ret[11]<<8
        ret=self.cmd(0x10,0) #get raw
        D1=ret[0]|ret[1]<<8|ret[2]<<16
        D2=ret[3]|ret[4]<<8|ret[5]<<16
        #print(C1,C2,C3,C4,C5,C6,D1,D2)

        dT=D2-C5*2**8
        T=2000+dT*C6/2**23
        OFF=C2*2**16+(C4*dT)/2**7
        SENS=C1*2**15+(C3*dT)/2**8
        P=(D1*SENS/2**21-OFF)/2**15

        assert T>2000,'Two cold to program'

        return P/100


if __name__=='__main__':
    pth=PTH()
    pth.cmd(2,0,8) # get number of channles
    #print(pth.get_channels())
    print(pth.getP())
    print(pth.getT())
    print(pth.getH())
    
