#!/usr/bin/env python

import os

####################################################################################################
### Settings class #################################################################################
####################################################################################################
class settings:
    def __init__( self ):
        # file index number
        self.i_file=0
        ## settings
        # list: values to be set
        # prio: sequence of the different settings
        ##
        # Vbb
        self.vbb_list=[]
        self.vbb_prio=[]
        # Vcasn
        self.vcasn_list=[[]]
        self.vcasn_prio=[]
        # Ithr
        self.ithr_list=[]
        self.ithr_prio=[]
        # Idb
        self.idb_list=[]
        self.idb_prio=[]
        # Vaux/Vrst
        self.vaux_list=[[]]
        self.vrst_list=[[]]
        self.vauxrst_prio=[]
        # dut position and position related settings
        self.dut_pos_list=[]
        self.scs_list    =[]
        # trigger delay for the DUTs
        #self.trg_dly=int(75)
        self.trg_dly_list=[75]
        # strobeBlength for the DUTs
        self.strbB_len=int(20)

    def generate_files( self ):
        for i_vbb in range(len(self.vbb_prio)):
            vbb_index=self.vbb_prio[i_vbb]
            vbb=self.vbb_list[vbb_index]
            for i_vcasn in range(len(self.vcasn_prio)):
                vcasn=self.vcasn_list[vbb_index][self.vcasn_prio[i_vcasn]]
                for i_ithr in range(len(self.ithr_prio)):
                    ithr=self.ithr_list[self.ithr_prio[i_ithr]]
                    for i_vauxrst in range(len(self.vauxrst_prio)):
                        vaux=self.vaux_list[vbb_index][self.vauxrst_prio[i_vauxrst]]
                        vrst=self.vrst_list[vbb_index][self.vauxrst_prio[i_vauxrst]]
                        for i_idb in range(len(self.idb_prio)):
                            idb=self.idb_list[self.idb_prio[i_idb]]
                            for i_dut_pos in range(len(self.dut_pos_list)):
                                dut_pos=self.dut_pos_list[i_dut_pos]
                                scs    =self.scs_list    [i_dut_pos]
                                for i_trg_dly in range(len(self.trg_dly_list)):
                                    trg_dly=self.trg_dly_list[i_trg_dly]
                                     
                                    args   =(self.i_file, vbb, vcasn, ithr, vaux, vrst, idb, dut_pos, scs, trg_dly , self.strbB_len)
                                    cmd    ="/bin/bash conf_gen_helper.sh %d %d %d %d %d %d %d %d %d %d %d"%args
                                    os.system(cmd)
                                    self.i_file+=1

####################################################################################################
####################################################################################################

s=settings()

#### load the standard settings
# Vbb (V)
s.vbb_list=[0,1,2,3,4,5,6]
s.vbb_prio=[0,3,6,2,4,1,5]

# back-bias dependent Vcasn (DAC)
s.vcasn_list=[[ 45, 51, 57, 63, 69],
              [ 71, 77, 83, 89, 95],
              [ 97,103,109,115,121],
              [123,129,135,141,147],
              [131,137,143,149,155],
              [140,146,152,158,164],
              [148,154,160,166,172]]
s.vcasn_prio=[2,3,4,0,1]

# Ithr (DAC)
s.ithr_list=[10,15,20,30,40,51,70]
s.ithr_prio=[ 6, 5, 4, 3, 2, 1, 0]

# Idb (DAC)
s.idb_list=[64]
s.idb_prio=[ 0]

# back-bias dependent Vaux/Vrst (DAC)
s.vaux_list=[[117],[118],[122],[125],[132],[139],[145]]
s.vrst_list=[[117],[118],[122],[125],[132],[139],[145]]
s.vauxrst_prio=[0]


# dut position and position related settings (mm)
#s.dut_pos_list=[97,295] # 97 = in the beam, 295 out of the beam
#s.scs_list    =[ 0,  1] # 0  = no S-Curve scan, 1 = do S-Curve scan
s.dut_pos_list=[ 0] # 97 = in the beam, 295 out of the beam
s.scs_list    =[ 0] # 0  = no S-Curve scan, 1 = do S-Curve scan
#
#### apply modifications
#s.vbb_prio=[4] # -4V only
#s.ithr_prio=[0,1,2,3,4,5] # omit Ithr=70DAC
#s.vcasn_prio=[2,3,4]
#s.generate_files()
#
#s.vbb_prio=[0,1,2,3,4,5,6] # all back-bias voltages
#s.vcasn_prio=[2,3,4,0,1]
#s.ithr_prio=[6]
#s.generate_files()
#
#s.vbb_prio=[6] # -6V only
#s.vcasn_prio=[0,1,3,4]    # central value was already measured
#s.ithr_prio=[0,1,2,3,4,5] # 10-51 DAC
#s.generate_files()
#
#s.vbb_prio=[0,3]



#############################################

s.i_file=92
s.vbb_prio=[0]
s.vcasn_prio=[0]
s.ithr_prio=[0,2,3,4,5] # omit Ithr=70DAC
s.idb_list=[128,196]
s.idb_prio=[  1,  0]

s.vbb_list=[0] # just 0v
s.vcasn_list=[[57]]
s.generate_files()

s.vbb_list=[3] # just 0v
s.vcasn_list=[[135]]
s.generate_files()


## golden settings
#############################################
#s.i_file=92
#
#s.idb_list=[64,128,196]
#s.idb_prio=[ 2,  1,  0]
#s.vbb_prio=[0]
#s.vcasn_prio=[0]
#s.ithr_prio=[0]
#s.vaux_list=[[117]]
#s.vrst_list=[[117]]
#s.vauxrst_prio=[0]
## dut position and position related settings (mm)
#s.dut_pos_list=[0] # 97 = in the beam, 295 out of the beam
#s.scs_list    =[0] # 0  = no S-Curve scan, 1 = do S-Curve scan

#s.vbb_list=[0]
#s.vcasn_list=[[67]]
#s.ithr_list=[30]
##s.trg_dly=61
#s.trg_dly_list=[61]
#s.generate_files()
#
#s.vbb_list=[0]
#s.vcasn_list=[[67]]
#s.ithr_list=[40]
##s.trg_dly=53
#s.trg_dly_list=[53]
#s.generate_files()
#
#s.vbb_list=[0]
#s.vcasn_list=[[70]]
#s.ithr_list=[35]
##s.trg_dly=61
#s.trg_dly_list=[61]
#s.generate_files()
#
#s.vbb_list=[0]
#s.vcasn_list=[[70]]
#s.ithr_list=[55]
##s.trg_dly=45
#s.trg_dly_list=[45]
#s.generate_files()
#
#
####s.vaux_list=[[125]]
####s.vrst_list=[[125]]
#
#s.vbb_list=[3]
#s.vcasn_list=[[145]]
#s.ithr_list=[30]
##s.trg_dly=81
#s.trg_dly_list=[81]
#s.generate_files()
#
#s.vbb_list=[3]
#s.vcasn_list=[[145]]
#s.ithr_list=[50]
##s.trg_dly=45
#s.trg_dly_list=[45]
#s.generate_files()
#
#s.vbb_list=[3]
#s.vcasn_list=[[150]]
#s.ithr_list=[45]
##s.trg_dly=61
#s.trg_dly_list=[61]
#s.generate_files()
#
#s.vbb_list=[3]
#s.vcasn_list=[[150]]
#s.ithr_list=[70]
##s.trg_dly=41
#s.trg_dly_list=[41]
#s.generate_files()
#
## extreme settings
#s.vbb_list=[3]
#s.vcasn_list=[[155]]
#s.ithr_list=[70]
##s.trg_dly=41
#s.trg_dly_list=[41]
#s.generate_files()
#
#s.vbb_list=[3]
#s.vcasn_list=[[160]]
#s.ithr_list=[80]
##s.trg_dly=41
#s.trg_dly_list=[41]
#s.generate_files()
#
#s.vbb_list=[3]
#s.vcasn_list=[[165]]
#s.ithr_list=[95]
##s.trg_dly=41
#s.trg_dly_list=[41]
#s.generate_files()

#s.vbb_list=[3]
#s.vcasn_list=[[170]]
#s.ithr_list=[95]
#s.trg_dly_list=[35]
#s.generate_files()

#s.idb_list=[64]
#s.idb_prio=[  0]
#s.vbb_list=[3]
#s.vcasn_list=[[135]]
#s.ithr_list=[51]
#s.trg_dly_list=[75]
#s.generate_files()

## delay scan
#############################################
#s.vbb_list=[3]
#s.vcasn_list=[[160]]
#s.ithr_list=[80]
#s.idb_list=[128]
#s.idb_prio=[  0]
#s.trg_dly_list=[0,16,31,47,71,97,111,151,191,231,271,311,351,391,431]
#s.generate_files()
#
#s.vcasn_list=[[165]]
#s.ithr_list=[95]
#s.idb_list=[128]
#s.idb_prio=[  0]
#s.trg_dly_list=[0,16,31,47,71,97,111,151,191,231,271,311,351,391,431]
#s.generate_files()
#
#s.vcasn_list=[[170]]
#s.ithr_list=[95]
#s.idb_list=[128]
#s.idb_prio=[  0]
#s.trg_dly_list=[0,16,31,47,71,97,111,151,191,231,271,311,351,391,431]
#s.generate_files()



## some other stuff
#############################################

#s.i_file=186
#s.vbb_prio=[4] # -4V only
#s.ithr_prio=[4,5] # omit Ithr=70DAC
#s.vcasn_prio=[4]
#s.generate_files()
#
#s.vbb_prio=[0,1,2,3,4,5,6] # all back-bias voltages
#s.vcasn_prio=[2,3,4,0,1]
#s.ithr_prio=[6]
#s.generate_files()
#
#s.vbb_prio=[6] # -6V only
#s.vcasn_prio=[0,1,3,4]    # central value was already measured
#s.ithr_prio=[0,1,2,3,4,5] # 10-51 DAC
#s.generate_files()

s.i_file-=92

print "%d config files produced" % s.i_file
print "Estimated measurement time %d min (%0.1f h) assuming 7.5 min per config file" % (s.i_file*7.5, s.i_file*7.5/60.)
