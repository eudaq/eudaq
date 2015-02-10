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
        # Vrst
        self.vrst_list=[]
        self.vrst_prio=[]
        # Vcasn
        self.vcasn_list=[[]]
        self.vcasn_prio=[]
        # Vcasn
        self.vcasn_list=[]
        self.vcasn_prio=[]
        # Ithr
        self.ithr_list=[]
        self.ithr_prio=[]
        # Vlight
        self.vlight_list=[]
        self.vlight_prio=[]
        # Acq_time
        self.acq_time_list=[]
        self.acq_time_prio=[]
        # Trig_delay
        self.trig_delay_list=[]
        self.trig_delay_prio=[]

    def generate_files( self ):
        for i_vbb in range(len(self.vbb_prio)):
            vbb_index=self.vbb_prio[i_vbb]
            vbb=self.vbb_list[vbb_index]
            for i_vrst in range(len(self.vrst_prio)):
                vrst=self.vrst_list[self.vrst_prio[i_vrst]]
                for i_vcasn in range(len(self.vcasn_prio)):
                    vcasn=self.vcasn_list[vbb_index][self.vcasn_prio[i_vcasn]]
                    for i_vcasp in range(len(self.vcasp_prio)):
                        vcasp=self.vcasp_list[self.vcasp_prio[i_vcasp]]
                        for i_ithr in range(len(self.ithr_prio)):
                            ithr=self.ithr_list[self.ithr_prio[i_ithr]]
                            for i_vlight in range(len(self.vlight_prio)):
                                vlight=self.vlight_list[self.vlight_prio[i_vlight]]
                                for i_acq_time in range(len(self.acq_time_list)):
                                    acq_time=self.acq_time_list[self.acq_time_prio[i_acq_time]]
                                    for i_trig_delay in range(len(self.trig_delay_list)):
                                        trig_delay=self.trig_delay_list[self.trig_delay_prio[i_trig_delay]]
                                        args   =(self.i_file, vbb, vrst, vcasn, vcasp, ithr, vlight, acq_time, trig_delay)
                                        cmd    ="/bin/bash conf_gen_helper.sh %d %f %f %f %f %f %f %f %f"%args
                                        os.system(cmd)
                                        self.i_file+=1

####################################################################################################
####################################################################################################

s=settings()

#### load the standard settings
# Vbb (V)
s.vbb_list=[ 0, 0.5, 1, 2, 3, 4, 5, 6 ]
s.vbb_prio=[ 0, 3, 2, 4, 1, 5, 6 ]

# Vrst (V)
s.vrst_list=[ 1.6 ]
s.vrst_prio=[   0 ]

# back-bias dependent Vcasn (V)      # Vbb:
s.vcasn_list=[[ 0.40, 0.50, 0.60 ],  # 0.0 V
              [ 0.60, 0.70, 0.75 ],  # 0.5 V
              [ 0.70, 0.80, 0.90 ],  # 1.0 V
              [ 0.90, 1.05, 1.10 ],  # 2.0 V
              [ 1.00, 1.10, 1.20 ],  # 3.0 V
              [ 1.20, 1.30, 1.40 ],  # 4.0 V
              [ 1.35, 1.40, 1.45 ],  # 5.0 V
              [ 1.40, 1.45, 1.50 ]]  # 6.0 V
s.vcasn_prio=[ 0, 1, 2 ]

# Vcasp (V)
s.vcasp_list=[ 0.6 ]
s.vcasp_prio=[   0 ]

# Ithr (uA)
s.ithr_list=[ 1.02, 1.54, 2.05, 2.87 ]
s.ithr_prio=[ 2, 0, 1, 3 ]

# Vlight (V)
s.vlight_list=[ 0., 10.25 ]
s.vlight_prio=[  0,     1 ]

# Acq_time
s.acq_time_list=[ 1.54 ]
s.acq_time_prio=[    0 ]
# Trig_delay
s.trig_delay_list=[ 0. ]
s.trig_delay_prio=[  0 ]

### apply modifications
s.vlight_prio=[  0  ]

s.generate_files()

print "%d config files produced" % s.i_file
print "Estimated measurement time %d min (%0.1f h) assuming 7.5 min per config file" % (s.i_file*7.5, s.i_file*7.5/60.)
