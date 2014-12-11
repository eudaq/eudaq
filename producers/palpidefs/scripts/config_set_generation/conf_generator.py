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
                                args   =(self.i_file, vbb, vcasn, ithr, vaux, vrst, idb, dut_pos, scs)
                                cmd    ="/bin/bash conf_gen_helper.sh %d %d %d %d %d %d %d %d %d"%args
                                os.system(cmd)
                                self.i_file+=1

####################################################################################################
####################################################################################################

s=settings()

### load the standard settings
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
s.dut_pos_list=[97,295] # 97 = in the beam, 295 out of the beam
s.scs_list    =[ 0,  1] # 0  = no S-Curve scan, 1 = do S-Curve scan

### apply modifications
s.vbb_prio=[4] # -4V only
s.ithr_prio=[0,1,2,3,4,5] # omit Ithr=70DAC
s.vcasn_prio=[2,3,4]
s.generate_files()

s.vbb_prio=[0,1,2,3,4,5,6] # all back-bias voltages
s.vcasn_prio=[2,3,4,0,1]
s.ithr_prio=[6]
s.generate_files()

s.vbb_prio=[6] # -6V only
s.vcasn_prio=[0,1,3,4]    # central value was already measured
s.ithr_prio=[0,1,2,3,4,5] # 10-51 DAC
s.generate_files()

print "%d config files produced" % s.i_file
print "Estimated measurement time %d min (%0.1f h) assuming 7.5 min per config file" % (s.i_file*7.5, s.i_file*7.5/60.)
