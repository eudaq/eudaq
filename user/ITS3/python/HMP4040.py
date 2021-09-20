import serial
from time import sleep

class HMP4040:
    def __init__(self,path):
        self.con=serial.Serial(path,rtscts=True,timeout=1)
        self.idn=None
        for _ in range(10):
            try:
                self.idn=self.ask_('*IDN?')
                break
            except IOError as e:
                print(e)
        if not self.idn: raise IOError('dsaf')
                
        if not ('HAMEG' and 'HMP4040') in self.idn:
            raise ValueError('Wrong device: %s'%self.idn)
        self.cmd_('SYSTEM:REMOTE')

    #TODO: does this work after a trip?
    def power(self,on=True,channel=None):
        if channel:
            self.set_channel_(channel)
            if on:
                self.cmd_('OUTPUT:STATE ON')
            else:
                self.cmd_('OUTPUT:STATE OFF')
        else:
            if on:
                self.cmd_('OUTPUT:GENERAL ON')
            else:
                self.cmd_('OUTPUT:GENERAL OFF')

    def set_fuse(ch,on=True,delayms='MIN',link=None):
        self.set_channel_(ch)
        self.cmd_('FUSE:DELAY %s',str(delayms))
        if not link: link=[]
        for ch2 in link:
            self.cmd_('FUSE:LINK %d'%ch2)
        if on:
            self.cmd_('FUSE:STATE ON')
        else:
            self.cmd_('FUSE:STATE OFF')
      
    def status(self):
        sel  =[]
        fused=[]
        curr_meas=[]
        curr_set =[]
        volt_meas=[]
        volt_set =[]
        for ch in range(1,5):
            self.set_channel_(ch)
            sel      .append(int  (self.ask_('OUTPut:SELECT?'                           ))!=0)
            volt_set .append(float(self.ask_('SOURCE:VOLTAGE:LEVEL:IMMEDIATE:AMPLITUDE?'))   )
            curr_set .append(float(self.ask_('SOURCE:CURRENT:LEVEL:IMMEDIATE:AMPLITUDE?'))   )
            volt_meas.append(float(self.ask_('MEASURE:SCALAR:VOLTAGE:DC?'               ))   )
            curr_meas.append(float(self.ask_('MEASURE:SCALAR:CURRENT:DC?'               ))   )
            fused    .append(int  (self.ask_('FUSE:TRIPPED?'                            ))!=0)
        return sel,volt_set,curr_set,volt_meas,curr_meas,fused

    def set_volt(self,ch,volt):
        self.cmd_('INSTRUMENT:NSELECT %d'%ch)
        self.cmd_('SOURCE:VOLTAGE:LEVEL:IMMEDIATE:AMPLITUDE %f'%volt)

    def set_curr(self,ch,curr):
        self.cmd_('INSTRUMENT:NSELECT %d'%ch)
        self.cmd_('SOURCE:CURRENT:LEVEL:IMMEDIATE:AMPLITUDE %f'%curr)

    def set_channel_(self,ch):
        self.cmd_('INSTRUMENT:NSELECT %d'%ch)

    def cmd_(self,c):
        self.con.write(bytes(c.strip()+'\n',encoding='ascii'))
    def ask_(self,c):
        self.cmd_(c)
        ret=self.con.readline()
        if not ret: raise IOError('Timeout')
        return ret.decode('ascii').strip()

if __name__=='__main__':
    import argparse
    from time import sleep
    import colorama
    from colorama import Style,Fore,Back

    parser=argparse.ArgumentParser(description='HMP4040 control',formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--path' ,'-p',default='/dev/hmp4040')
    parser.add_argument('--channel' ,'-c',type=int,choices=[1,2,3,4])
    parser.add_argument('--set-current' ,'-i',type=float,help='Set current in A (not mA)')
    parser.add_argument('--set-voltage' ,'-v',type=float,help='Set current in V')
    parser.add_argument('--on' ,action='store_true',help='Turn channel on')
    parser.add_argument('--off',action='store_true',help='Turn channel off')
    parser.add_argument('--GPIB' ,'-g')
    args=parser.parse_args()
    if args.set_current and not args.channel: raise ValueError('need to specify channel')
    if args.set_voltage and not args.channel: raise ValueError('need to specify channel')
    if args.on          and not args.channel: raise ValueError('need to specify channel')
    if args.off         and not args.channel: raise ValueError('need to specify channel')

    h=HMP4040(args.path)
    print('Connected to "%s" at "%s.'%(h.idn,args.path))
    if args.set_voltage is not None: 
        print('- setting voltage of channel %d to %f V'%(args.channel,args.set_voltage))
        h.set_volt(args.channel,args.set_voltage)
    if args.set_current is not None: 
        print('- setting current of channel %d to %f A'%(args.channel,args.set_current))
        h.set_curr(args.channel,args.set_current)
    if args.on:
        print('- turning on channel %d'%(args.channel))
        h.power(True,args.channel)
        sleep(1)
    if args.off:
        print('- turning off channel %d'%(args.channel))
        h.power(False,args.channel)
        sleep(1)

    if args.GPIB:
         print('- sending custom GPIB message:')
         if 'channel' in args:
              print('  - setting channel to: %d'%args.channel)
              h.set_channel_(args.channel)
         if '?' in args.gpib:
              print('  - sending: "%s"'%args.GPIB)
              ret=h.ask_(args.GPIB)
              print('  - reply: "%s"'%ret)
         else:
              print('  - sending %s'%args.GPIB)
              h.cmd_(args.GPIB)

    colorama.init()
    on =Style.RESET_ALL+Fore.GREEN+Style.BRIGHT
    off=Style.RESET_ALL+Fore.RED
    oo =Style.RESET_ALL+Back.RED+Fore.WHITE+Style.BRIGHT

    print(Fore.WHITE+Style.BRIGHT,'== STATUS ==== STATUS ==== STATUS ==== STATUS === STATUS ===== STATUS ==')
    sel,volt_set,curr_set,volt_meas,curr_meas,fused=h.status()
    print(Style.RESET_ALL+' '*14,end='') 
    for ch in range(1,5):
        print(Style.RESET_ALL,'  ',end='')
        if fused[ch-1]: print(oo,end='')
        elif sel[ch-1]: print(on,end='')
        else          : print(off,end='')
        print('=== CH %d ==='%ch,end='')
    print(Style.RESET_ALL)
    print(Style.RESET_ALL+'        State:',end='')
    for ch in range(1,5):
        print(Style.RESET_ALL,'  ',end='')
        if  fused[ch-1]: print(oo +'  TRIPPED   ',end='')
        elif sel[ch-1]: print(on +'     ON     ',end='')
        else          : print(off+'    OFF     ',end='')
    print(Style.RESET_ALL)
    print(Style.RESET_ALL+'  Set voltage:',end='')
    for ch in range(1,5): print(Style.RESET_ALL+'   %8.2f V  '%volt_set[ch-1],end='')
    print(Style.RESET_ALL)
    print(Style.RESET_ALL+'  Set current:',end='')
    for ch in range(1,5): print(Style.RESET_ALL+'   %8.2f mA '%(curr_set[ch-1]/1e-3),end='')
    print(Style.RESET_ALL)
    print(Style.RESET_ALL+'Meas. voltage:',end='')
    for ch in range(1,5): print(Style.RESET_ALL+'   %8.2f V  '%volt_meas[ch-1],end='')
    print(Style.RESET_ALL)
    print(Style.RESET_ALL+'Meas. current:',end='')
    for ch in range(1,5): print(Style.RESET_ALL+'   %8.2f mA '%(curr_meas[ch-1]/1e-3),end='')
    print(Style.RESET_ALL)

