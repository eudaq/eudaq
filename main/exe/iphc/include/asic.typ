
/*******************************************************************************
File      : x:\lib\com\asic\asic.typ
Goal      : Types definition of ASIC common constants / strcutures librairy
Prj date  : 29/06/2009
File date : 29/06/2009
Doc date  : //200
Author    : Gilles CLAUS
E-mail    : claus@lepsi.in2p3.fr
----------------------------------------------------------------------------------
License   : You are free to use this source files for your own development as long
          : as it stays in a public research context. You are not allowed to use it
          : for commercial purpose. You must put this header with laboratory and
          : authors names in all development based on this library.
----------------------------------------------------------------------------------
Labo      : IPHC */
/*******************************************************************************/


#ifndef ASIC_TYP
#define ASIC_TYP


/* ============== */
/*  */
/* ============== */


typedef struct {
  
  SInt8  AsicNo;
  SInt32 AcqNo;
  SInt32 FrameNoInAcq;
  SInt32 FrameNoInRun;
  
  SInt32 HitCnt; // Used for monitoring, if not set HitCnt = -1
    
  //
  
  SInt32 ATrigRes[ASIC__ENUM_TRIG_RES_NB];
  
    
} ASIC__TFrameStatus;


#endif

