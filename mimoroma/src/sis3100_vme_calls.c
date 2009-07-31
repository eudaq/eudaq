/*===========================================================================*/
/*                                                                           */
/* File:             sis3100_vme_calls.c                                     */
/*                                                                           */
/* OS:               LINUX (Kernel >= 2.4.18                                 */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Version:          1.2                                                     */
/*                                                                           */
/*                                                                           */
/* Generated:        18.12.01                                                */
/* Modified:         05.11.03                                                */
/* Modified:         21.06.04                                                */
/*                                                                           */
/* Author:           TH                                                      */
/*                                                                           */
/* Last Change:                       Installation                           */
/*---------------------------------------------------------------------------*/
/* SIS GmbH                                                                  */
/* Harksheider Str. 102A                                                     */
/* 22399 Hamburg                                                             */
/*                                                                           */
/* http://www.struck.de                                                      */
/*                                                                           */
/*===========================================================================*/

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "sis1100_var.h"  /* pfad im Makefile angeben */


#include "sis3100_vme_calls.h"









/**********************/
/*                    */
/*    VME SYSReset    */
/*                    */
/**********************/

int vmesysreset(int p)
{
  if (s3100_control_write(p, 0x100 /*offset*/, 0x2 /*data*/) != 0x0)   return -1 ;
  usleep(500000); /* 500ms (min. 200ms) */
  if (s3100_control_write(p, 0x100 /*offset*/, 0x20000 /*data*/)  != 0x0)   return -1 ;
  return 0 ;
}

/********************************/
/*                              */
/*    VME Read IRQ Ackn. Cycle  */
/*                              */
/********************************/

/* VME Read IRQ Ackn.  Cycles */

int vme_IACK_D8_read(int p, u_int32_t vme_irq_level, u_int8_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=1; /* driver does not change any field except data */
  req.am=0x4000; /*  */
  req.addr= (vme_irq_level << 1) + 1;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)  return -1 ;
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;

}








/*****************/
/*               */
/*    VME A16    */
/*               */
/*****************/

/* VME A16  Read Cycles */

int vme_A16D8_read(int p, u_int32_t vme_adr, u_int8_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=1; /* driver does not change any field except data */
  req.am=0x29; /*  */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;

}


int vme_A16D16_read(int p, u_int32_t vme_adr, u_int16_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=2; /* driver does not change any field except data */
  req.am=0x29; /* "" */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;
}



int vme_A16D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=4; /* driver does not change any field except data */
  req.am=0x29; /* "" */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;
}


/* VME A16  Write Cycles */

int vme_A16D8_write(int p, u_int32_t vme_adr, u_int8_t vme_data )
{
struct sis1100_vme_req req;
  req.size=1;
  req.am=0x29;
  req.addr= vme_adr;
  req.data= (u_int32_t)vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}

int vme_A16D16_write(int p, u_int32_t vme_adr, u_int16_t vme_data )
{
struct sis1100_vme_req req;
  req.size=2;
  req.am=0x29;
  req.addr= vme_adr;
  req.data= (u_int32_t)vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}



int vme_A16D32_write(int p, u_int32_t vme_adr, u_int32_t vme_data )
{
struct sis1100_vme_req req;
  req.size=4;
  req.am=0x29;
  req.addr= vme_adr;
  req.data= vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}






/*****************/
/*               */
/*    VME A24    */
/*               */
/*****************/

/* VME A24  Read Cycles */

int vme_A24D8_read(int p, u_int32_t vme_adr, u_int8_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=1; /* driver does not change any field except data */
  req.am=0x39; /*  */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;
}


int vme_A24D16_read(int p, u_int32_t vme_adr, u_int16_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=2; /* driver does not change any field except data */
  req.am=0x39; /* "" */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;
}



int vme_A24D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=4; /* driver does not change any field except data */
  req.am=0x39; /* "" */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;
}




int vme_A24DMA_D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x39;
   block_req.addr=vme_adr ;
   block_req.data=(u_int8_t*)vme_data ;

   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}


int vme_A24BLT32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x3b;
   block_req.addr=vme_adr ;
   block_req.data=(u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}



int vme_A24MBLT64_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x38;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}



int vme_A24BLT32FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0x3b;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}



int vme_A24MBLT64FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0x38;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}







/* VME A24  Write Cycles */

int vme_A24D8_write(int p, u_int32_t vme_adr, u_int8_t vme_data )
{
struct sis1100_vme_req req;
  req.size=1;
  req.am=0x39;
  req.addr= vme_adr;
  req.data= (u_int32_t)vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}

int vme_A24D16_write(int p, u_int32_t vme_adr, u_int16_t vme_data )
{
struct sis1100_vme_req req;
  req.size=2;
  req.am=0x39;
  req.addr= vme_adr;
  req.data= (u_int32_t)vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}



int vme_A24D32_write(int p, u_int32_t vme_adr, u_int32_t vme_data )
{
struct sis1100_vme_req req;
  req.size=4;
  req.am=0x39;
  req.addr= vme_adr;
  req.data= vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}



int vme_A24DMA_D32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x39;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_WRITE, &block_req)<0)  return -1 ;  /* NEW */
   *put_num_of_lwords = block_req.num;
   return block_req.error ;        /* NEW */
}


int vme_A24BLT32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x3b;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_WRITE, &block_req)<0)  return -1 ;  /* NEW */
   *put_num_of_lwords = block_req.num;
   return block_req.error ;        /* NEW */
}


int vme_A24MBLT64_write(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x38;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_WRITE, &block_req)<0)  return -1 ;  /* NEW */
   *put_num_of_lwords = block_req.num;
   return block_req.error ;        /* NEW */
}



























/*****************/
/*               */
/*    VME A32    */
/*               */
/*****************/


/* VME A32  Read Cycles */

int vme_A32D8_read(int p, u_int32_t vme_adr, u_int8_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=1; /* driver does not change any field except data */
  req.am=0x9; /*  */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;
}


int vme_A32D16_read(int p, u_int32_t vme_adr, u_int16_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=2; /* driver does not change any field except data */
  req.am=0x9; /* "" */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;
  return 0 ;
}



int vme_A32D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data )
{
struct sis1100_vme_req req;

  req.size=4; /* driver does not change any field except data */
  req.am=0x9; /* "" */
  req.addr= vme_adr;
  if (ioctl(p, SIS3100_VME_READ, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  *vme_data = req.data;    /* NEW */
  return 0 ;
}







int vme_A32DMA_D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x9;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;

   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}






int vme_A32BLT32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0xb;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;

   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}



int vme_A32MBLT64_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x8;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}



int vme_A32_2EVME_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x20;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}






int vme_A32DMA_D32FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0x9;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}





int vme_A32BLT32FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0xb;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}



int vme_A32MBLT64FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0x8;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}


int vme_A32_2EVMEFIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=1;
   block_req.size=4;
   block_req.am=0x20;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_READ, &block_req)<0)  return -1 ;   /* NEW */
   *got_num_of_lwords = block_req.num;
   return block_req.error ;            /* NEW */
}











/* VME A32  Write Cycles */

int vme_A32D8_write(int p, u_int32_t vme_adr, u_int8_t vme_data )
{
struct sis1100_vme_req req;
  req.size=1;
  req.am=0x9;
  req.addr= vme_adr;
  req.data= (u_int32_t)vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}

int vme_A32D16_write(int p, u_int32_t vme_adr, u_int16_t vme_data )
{
struct sis1100_vme_req req;
  req.size=2;
  req.am=0x9;
  req.addr= vme_adr;
  req.data= (u_int32_t)vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}



int vme_A32D32_write(int p, u_int32_t vme_adr, u_int32_t vme_data )
{
struct sis1100_vme_req req;
  req.size=4;
  req.am=0x9;
  req.addr= vme_adr;
  req.data= vme_data;
  if (ioctl(p, SIS3100_VME_WRITE, &req)<0)   return -1;        /* NEW */
  if (req.error) return req.error ;
  return 0 ;
}



int vme_A32DMA_D32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x9;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_WRITE, &block_req)<0)  return -1 ;  /* NEW */
   *put_num_of_lwords = block_req.num;
   return block_req.error ;        /* NEW */
}

int vme_A32BLT32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0xb;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;

   if (ioctl(p, SIS3100_VME_BLOCK_WRITE, &block_req)<0)  return -1 ;  /* NEW */
   *put_num_of_lwords = block_req.num;
   return block_req.error ;        /* NEW */
}


int vme_A32MBLT64_write(int p, u_int32_t vme_adr, u_int32_t* vme_data,
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords)
{
struct sis1100_vme_block_req block_req;

   block_req.num=req_num_of_lwords   ; /*  */
   block_req.fifo=0;
   block_req.size=4;
   block_req.am=0x8;
   block_req.addr=vme_adr ;
   block_req.data = (u_int8_t*)vme_data ;
   if (ioctl(p, SIS3100_VME_BLOCK_WRITE, &block_req)<0)  return -1 ;  /* NEW */
   *put_num_of_lwords = block_req.num;
   return block_req.error ;        /* NEW */
}
















/***********************/
/*                     */
/*    s3100_control    */
/*                     */
/***********************/


int s3100_control_read(int p, int offset, u_int32_t* data)
{
struct sis1100_ctrl_reg reg;
int error ;
  reg.offset = offset;
  error = (ioctl(p, SIS1100_CTRL_READ, &reg)<0)  ;
  *data = reg.val;
  return error ;
}



int s3100_control_write(int p, int offset, u_int32_t data)
{
struct sis1100_ctrl_reg reg;
int error ;
  reg.offset = offset; 
  reg.val  = data; 
  error = (ioctl(p, SIS1100_CTRL_WRITE, &reg)<0)  ;
  return error ;
}










/***********************/
/*                     */
/*    s3100_sharc      */
/*                     */
/***********************/


int s3100_sharc_write(int p_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )
{
int return_code ;

  lseek(p_sharc_desc, byte_adr, SEEK_SET);   /* must be longword aligned */
  return_code=write(p_sharc_desc, ptr_data, num_of_lwords*4);

 
/* return_code = length ? */
/*
res=write(p_sharc_desc, data, 4);
    if (res<0) {
        printf("write(0x%08lx, 0x%x): %s\n", offset, data, strerror(errno));
        exit(1);
    }
    if (res!=4) {
        printf("write(0x%08lx, 0x%x): res=%d\n", offset, data, res);
        exit(1);
    }
*/    

  return return_code ;
}






int s3100_sharc_read(int p_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )
{
int return_code ;

 
  lseek(p_sharc_desc, byte_adr, SEEK_SET);   /* must be longword aligned */
  return_code=read(p_sharc_desc, ptr_data, num_of_lwords*4);

  return return_code ;
}





/***********************/
/*                     */
/*    s3100_sdram      */
/*                     */
/***********************/


int s3100_sdram_write(int p_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )
{
int return_code ;


  lseek(p_sdram_desc, byte_adr, SEEK_SET);   /* must be longword aligned */
  return_code=write(p_sdram_desc, ptr_data, num_of_lwords*4);
  
/* return_code = length ? */
/*
res=write(p, &data, 4);
    if (res<0) {
        printf("write(0x%08lx, 0x%x): %s\n", offset, data, strerror(errno));
        exit(1);
    }
    if (res!=4) {
        printf("write(0x%08lx, 0x%x): res=%d\n", offset, data, res);
        exit(1);
    }
*/    

  return return_code ;
}






int s3100_sdram_read(int p_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )
{
int return_code ;

  
  lseek(p_sdram_desc, byte_adr, SEEK_SET);   /* must be longword aligned */
  return_code=read(p_sdram_desc, ptr_data, num_of_lwords*4);


  return return_code ;
}









































