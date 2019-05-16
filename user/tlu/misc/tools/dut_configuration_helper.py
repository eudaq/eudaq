'''DUT configuration helper
adopted from  paolo.baesso@bristol.ac.uk
written by  jan.dreyling-eschweiler@desy.de'''

import time
from cmd import Cmd

class dut_prompt(Cmd):

    dut_mode = [0]*4        # DUTMask, HDMIx_set, HDMIx_clk 
    dut_triggerID = [0]*4   # DUTMaskmode
    dut_ignore_busy = [0]*4 # DUTIgnoreBusy
        
    def do_start(self, args):
        """Starts the helper to generate the values of parameters to be included in the AIDA TLU section of the EUDAQ configuration file."""
        # TEST
        #self.dut_mode       = [2, 2, 0, 0]
        #self.dut_triggerID  = [1, 1, 0, 0]
        #self.dut_ignore_busy = [1, 0, 0, 0]
        #self.create_conf('1')
        #return
        # TEST end
        # Get user to define the status of 4 DUT channels
        # 1) AIDA (clock line from the TLU; busy as DUT veto) or EUDET (clock line from the DUT; trigger-busy-handshake) mode
        # 2) optional TriggerID read-out
        # 3) optional ignoring DUT busy / disable handshake

        print("  Please specify the mode for each DUT channel")
        print("  1 = AIDA, 2 = EUDET or 0 = off (default)")
        print("  (AIDA = clock line from the TLU; busy as DUT veto")
        print("   EUDET = clock line from the DUT; trigger-busy-handshake")
        for index, channel in enumerate(range(1, len(self.dut_mode)+1)):
            try:
                inprompt = "\tEnter setting for DUT channel [" + str(channel) + "]:  "
                value = int(input( inprompt ))
                if ((value == 1) or (value == 2) or (value == 0)):
                    self.dut_mode[index] = value
                else:
                    print("\t Not a valid value, using default (0).")
                    self.dut_mode[index] = 0
            except ValueError:
                print("\t Not a valid value, using default (0).")
                self.dut_mode[index] = 0
        print("  Current DUT modes are:")
        print("\tDUT1\tDUT2\tDUT3\tDUT4")
        print("   ", *self.dut_mode, sep='\t')
        print("\n")
        
        print("  Enable the TriggerID clocking-out?")
        print("  1 = enable, 0 = disable (default)")
        print("  (In AIDA mode  = clocking out from TLU")
        print("   In EUDET mode = clocking out from DUT (EUDET standard)")
        for index, channel in enumerate(range(1, len(self.dut_triggerID)+1)):
            try:
                inprompt = "\tEnter setting for DUT channel [" + str(channel) + "]:  "
                value = int(input( inprompt ))
                if ((value == 1) or (value == 0)):
                    self.dut_triggerID[index] = value
                else:
                    print("\t Not a valid value, using default (0).")
                    self.dut_trigger_ID[index] = 0
            except ValueError:
                print("\t Not a valid value, using default (0).")
                self.dut_triggerID[index] = 0
        print("  These channels reading out the triggerID:")
        print("\tDUT1\tDUT2\tDUT3\tDUT4")
        print("   ", *self.dut_triggerID, sep='\t')
        print("\n")

        print("  Ignoring a DUT's busy for vetoing trigger?")
        print("  1 = enable, 0 = disable (default)")
        print("  (Note: Busy holds still for the individual channel.")
        for index, channel in enumerate(range(1, len(self.dut_ignore_busy)+1)):
            try:
                inprompt = "\tEnter setting for DUT channel [" + str(channel) + "]:  "
                value = int(input( inprompt ))
                if ((value == 1) or (value == 0)):
                    self.dut_ignore_busy[index] = value
                else:
                    print("\t Not a valid value, using default (0).")
                    self.dut_trigger_ID[index] = 0
            except ValueError:
                print("\t Not a valid value, using default (0).")
                self.dut_ignore_busy[index] = 0
        print("  These channels reading out the ignore_busy:")
        print("\tDUT1\tDUT2\tDUT3\tDUT4")
        print("   ", *self.dut_ignore_busy, sep='\t')
        print("\n")
        print("  Print all possible configuration parameter of AidaTluProducer?")
        full_parameters = input("  1 = yes, 0 = no (default): ")
        if not full_parameters == '1':
            full_parameters = False
        else:
            full_parameters = True
        print("\n")
    
        # now create the conf file
        self.create_conf(full_parameters)
       
    def do_show(self, args):
        """Shows the EUDAQ parameters for the DUT part of the AIDA TLU."""
        # now create the conf file
        self.create_conf(True)
        
    def do_exit(self, args):
        """Quits the program."""
        print ("Quitting.")
        raise SystemExit
        
    def create_conf(self, full_parameters):
        print("_______________________________________________________")
        print("\tDONE: copy the following two lines into the EUDAQ config file:\n")
        if full_parameters == True:
            print("\tconfid =", time.strftime("%Y%m%d", time.gmtime()))
            print("\tverbose =", "0")
            print("\tskipconf =", "0")
            print("")
        print("\tDUTMask =", hex(self.create_DUTMask()))
        print("")
        if full_parameters == True:
            print("\t# Change the line direction (expert mode)")
            print("\t# bit mask: 0 CONT, 1 SPARE, 2 TRIG, 3 BUSY (1 = driven by TLU, 0 = driven by DUT)")
            print("\tHDMI1_set = 0x7")
            print("\tHDMI2_set = 0x7")
            print("\tHDMI3_set = 0x7")
            print("\tHDMI4_set = 0x7")
        #print("\t# clock line: 1 = from TLU (AIDA mode), 0 = from DUT (EUDET mode), 2 = from FPGA")
        for index, value in enumerate(self.dut_mode):
            if value != 1:
                print("\tHDMI" + str(index+1) + "_clk =", 0)
            elif value == 1:
                print("\tHDMI" + str(index+1) + "_clk =", self.dut_mode[index])
        print("")
        print("\tDUTMaskMode =", '0x{:00X}'.format(self.create_DUTMaskMode(), 2))
        print("\tDUTIgnoreBusy =", hex(self.create_DUTIgnoreBusy()))


    def create_DUTMaskMode(self):
        dut_mask_mode = 0
        for index, value in enumerate(self.dut_triggerID):
            #print(index, value, self.dut_mode[index])
            # if wo trigger id (aida/eudet)
            if value == 0 or self.dut_mode[index] == 0:
                dut_mask_mode += 2**(2*index+1) * 1 + 2**(2*index) * 1
                #print(dut_mask_mode)
            # if with trigger id in eudet mode
            if self.dut_mode[index] == 2 and value == 1:
                dut_mask_mode += 2**(2*index+1) * 0 + 2**(2*index) * 0
                #print(dut_mask_mode)
            # if with trigger id in aida mode
            if self.dut_mode[index] == 1 and value == 1:
                dut_mask_mode += 2**(2*index+1) * 0 + 2**(2*index) * 1
                #print(dut_mask_mode)
        return dut_mask_mode
    
    def create_DUTIgnoreBusy(self):
        dut_ignore_busy = 0
        for index, channel in enumerate(self.dut_ignore_busy):
            #print(index, channel)
            if channel > 0:
                dut_ignore_busy += 2**index * 1
                #print(dut_mask)
        return dut_ignore_busy

    def create_DUTMask(self):
        dut_mask = 0
        for index, channel in enumerate(self.dut_mode):
            #print(index, channel)
            if channel > 0:
                dut_mask += 2**index * 1
                #print(dut_mask)
        return dut_mask

if __name__ == '__main__':
    prompt = dut_prompt()
    prompt.prompt = '> '
    prompt.intro = '''_______________________________________________________
        Welcome to the AIDA TLU DUT configuration helper.
        Type help or ? to list commands.\n
        To begin, type \'start\'. To show full parameters, type \'show\'.'''
    prompt.cmdloop()
