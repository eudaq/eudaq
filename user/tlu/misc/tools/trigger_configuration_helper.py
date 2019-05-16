###  trigger_configurator_helper
###  Developed for the configuration of the AIDA2020 Trigger Logic Unit
###  
###  P. Baesso - April 2019
###  paolo.baesso@bristol.ac.uk
###
###  For documentation visit:
###  https://ohwr.org/project/fmc-mtlu 


import time
import pprint
import cmd

from cmd import Cmd

class MyPrompt(Cmd):

    trigReq= [-1]*6 # This contains the user-required setting
        
    def do_start(self, args):
        """Start the procedure to determine the trigger configuration word"""
        # Get user to define the status of 6 trigger inputs and the number
        # of active inputs required to generate a trigger (minActive)
        print ("\n  Please specify, for each input, how it takes part in the trigger")
        print ("  1= active, 0= veto or -1= do not care (default)")
        #for iTrig in range (0, 6):
        for iTrig in range(len(self.trigReq)):
            try:
                inprompt= "\tEnter requirement for INPUT[" + str(iTrig) + "]:  "
                newVal = int(input( inprompt ))
                if ((newVal== 1) or (newVal== 0) or (newVal== -1)):
                    self.trigReq[iTrig]= newVal
                else:
                    print ("\t Not a valid value, using default (-1).")
                    self.trigReq[iTrig]= -1
            except ValueError:
                print("\t Not a valid value, using default (-1).")
                self.trigReq[iTrig]= -1
        print ("\n")
        print ("  Summary of your configuration:")
        print ("\tIn5\tIn4\tIn3\tIn2\tIn1\tIn0")
        #print ("\t", self.trigReq[::-1])
        print("   ", *self.trigReq[::-1], sep='\t')
        print ("\n")
        try:
            inprompt= "  Trigger if at least N active inputs: (N=?) "
            minActive = int(input( inprompt ))
            if ((minActive < 0) or (minActive > 5)):
                print ("\t Not a valid value, using default (1).")
                minActive= 1
        except ValueError:
            print("\t Not a valid value, using default (1).")
            minActive= 1
        print ("\n")
    # Now do the actual matching
        self.matchWords(minActive)
    
    def help_start(self):
        print ("\n  Starts the procedure to determine the trigger configuration words")
        print ("  You will be asked to define how the TLU trigger inputs are connected")
        print ("  in your setup.")
        print ("  Each input can be defined as ACTIVE (1), VETO (0) or DO-NOT-CARE (-1):\n")
        print ("  - ACTIVE: this input must be asserted for the TLU to generate a trigger.\n  If M inputs are defined as ACTIVE, it is possible to require\n  that at least N of them are asserted at the same time to have\n  a valid TLU trigger.\n  Typically, this is the setting to use for scintillators, pulsers, etc.\n")
        print ("  - VETO: when asserted, the TLU does not issue triggers.\n  All VETO inputs must be deasserted in order to have any trigger.\n  Use this if you have a INHIBIT signal.\n  If you use the TLU SHUTTER functionality\n  set the shutter input to VETO to avoid potential issues.\n")
        print ("  - DNC: the state of these inputs can be either asserted or deasserted.\n  They do not contribute to a valid trigger.\n  This option can be used for inputs\n  that must be timestamped but not take part in the trigger.\nheleda")
        
    def do_readme(self, args):
        print ("\n  NOTE: this tool provides a quick and simple way to generate the\n  configuration words for the TLU trigger.")
        print ("  As such, it can only generate basic configurations.\n  The TLU trigger is very powerful and flexible")
        print ("  and allows the user to input complex trigger functions.\n  Please refer to the TLU manual to understand how to")
        print ("  manually generate configuration parameter for more\n  advanced trigger functionalities.")
        
    def do_quit(self, args):
        """Quits the program."""
        print ("\n  Quitting.")
        raise SystemExit
        
    def matchWords(self, minActive):
        lowWord= 0
        highWord= 0
        longWord= 0
        print ("  The following input combinations will produce a TLU trigger:")
        print ("_______________________________________________________")
        print ("\tIn5\tIn4\tIn3\tIn2\tIn1\tIn0")
        for iCombination in range (0, 64):
            patternList= self.intToBinList(iCombination)
            trigMajority= sum(x > 0 for x in patternList)
            if (trigMajority >= minActive):
            # Only test current pattern if it has enough
            # active inputs to be a possible candidate
                isValid= self.elementWiseCheck(self.trigReq, patternList, minActive)
                #print (patternList[::-1], " = ", isValid)
                if (isValid):
                    print("   ", *patternList[::-1], sep='\t')
                
                longWord =(isValid << iCombination) | longWord
        lowWord= 0xFFFFFFFF & longWord
        highWord= longWord >> 32
        print ("_______________________________________________________")
        print ("  All done. Copy the following two lines into the EUDAQ config file:\n")
        #print ("Whatabout= ", '0x{0:0{1}X}'.format(longWord, 16) )
        print ("\ttrigMaskLo= ", '0x{0:0{1}X}'.format(lowWord, 8) )
        print ("\ttrigMaskHi= ", '0x{0:0{1}X}'.format(highWord, 8) )
        print ("\n")
        if ((lowWord== 0x1) & (highWord== 0x0)): 
            print ("\tWARNING: setting this trigger configuration means that\n\tthe TLU will trigger if and only if all inputs are LOW.")
        if ((lowWord== 0xFFFFFFFF) & (highWord== 0xFFFFFFFF)): 
            print ("\tWARNING: setting this trigger configuration means that\n\tthe TLU always be triggering.")
        if ((lowWord== 0x0) & (highWord== 0x0)): #actually this is impossible
            print ("\tWARNING: setting this trigger configuration means that\n\tthere is no configuration for which the TLU will trigger.")    
    
        return
        
    def elementWiseCheck(self, request, pattern, tolerance):
        """Check element-wise but allows for 'do not care' elements"""
        # Return 1 if all zeros match and at least "tolerance" ones match
        # Return 0 in all other cases        
        areSame= 0
        matching=0
        for iElement in range(len(request)):
            if ((request[iElement]== 0) and (pattern[iElement]== 1)):
            # As soon as a VETO requirement fails, dismiss the pattern
                areSame= 0
                return areSame
            if ( (request[iElement]== 1) and (pattern[iElement]== 1) ):
                matching= matching+1
            if ( (request[iElement]== -1)  ):
                matching= matching
        if (matching >=tolerance):
            areSame=1
        return areSame
        
    def intToBinList(self, inputInt):
        outList=[0]*6
        for iElement in range(0, 6):
            outList[iElement]= (inputInt >> iElement) & 0x1
        return outList
            
if __name__ == '__main__':
    prompt = MyPrompt()
    prompt.prompt = '> '
    prompt.intro = '_______________________________________________________\nWelcome to the AIDA TLU trigger configuration helper.\nType help or ? to list commands.\n\nTo begin, type \'start\': the helper will ask questions\nand generate the values to be included in the AIDA\nconfiguration file.\nFor queries, contact: paolo.baesso@bristol.ac.uk\n'
    #prompt.cmdloop('Starting prompt. Type help for a list of commands.')
    prompt.cmdloop()