/*===========================================================================*/
/*                                                                           */
/* File:             sis3100_vme_calls.h                                     */
/*                                                                           */
/* OS:               LINUX (Kernel >= 2.4.4                                  */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Version:          1.1                                                     */
/*                                                                           */
/*                                                                           */
/* Generated:        18.12.01                                                */
/* Modified:         21.06.04   MKI                                          */
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





/**********************/
/*                    */
/*    VME SYSReset    */
/*                    */
/**********************/


int vmesysreset(int p) ;

/********************************/
/*                              */
/*    VME Read IRQ Ackn. Cycle  */
/*                              */
/********************************/

int vme_IACK_D8_read(int p, u_int32_t vme_irq_level, u_int8_t* vme_data ) ;




/*****************/
/*               */
/*    VME A16    */
/*               */
/*****************/

int vme_A16D8_read(int p, u_int32_t vme_adr, u_int8_t* vme_data ) ;
int vme_A16D16_read(int p, u_int32_t vme_adr, u_int16_t* vme_data ) ;
int vme_A16D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data ) ;

int vme_A16D8_write(int p, u_int32_t vme_adr, u_int8_t vme_data ) ;
int vme_A16D16_write(int p, u_int32_t vme_adr, u_int16_t vme_data ) ;
int vme_A16D32_write(int p, u_int32_t vme_adr, u_int32_t vme_data ) ;





/*****************/
/*               */
/*    VME A24    */
/*               */
/*****************/


int vme_A24D8_read(int p, u_int32_t vme_adr, u_int8_t* vme_data ) ;
int vme_A24D16_read(int p, u_int32_t vme_adr, u_int16_t* vme_data ) ;
int vme_A24D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data ) ;

int vme_A24DMA_D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;

int vme_A24BLT32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;

int vme_A24MBLT64_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;


int vme_A24BLT32FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;

int vme_A24MBLT64FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;




int vme_A24D8_write(int p, u_int32_t vme_adr, u_int8_t vme_data ) ;
int vme_A24D16_write(int p, u_int32_t vme_adr, u_int16_t vme_data ) ;
int vme_A24D32_write(int p, u_int32_t vme_adr, u_int32_t vme_data ) ;


int vme_A24DMA_D32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A24BLT32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A24MBLT64_write(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;







/*****************/
/*               */
/*    VME A32    */
/*               */
/*****************/


int vme_A32D8_read(int p, u_int32_t vme_adr, u_int8_t* vme_data ) ;
int vme_A32D16_read(int p, u_int32_t vme_adr, u_int16_t* vme_data ) ;
int vme_A32D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data ) ;


int vme_A32DMA_D32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;

int vme_A32BLT32_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;

int vme_A32MBLT64_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;


int vme_A32_2EVME_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;



int vme_A32DMA_D32FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_no_of_lwords) ;

int vme_A32BLT32FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;

int vme_A32MBLT64FIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                          u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;


int vme_A32_2EVMEFIFO_read(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* got_num_of_lwords) ;



int vme_A32D8_write(int p, u_int32_t vme_adr, u_int8_t vme_data ) ;
int vme_A32D16_write(int p, u_int32_t vme_adr, u_int16_t vme_data ) ;
int vme_A32D32_write(int p, u_int32_t vme_adr, u_int32_t vme_data ) ;


int vme_A32DMA_D32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;

int vme_A32BLT32_write(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;


int vme_A32MBLT64_write(int p, u_int32_t vme_adr, u_int32_t* vme_data, 
                      u_int32_t req_num_of_lwords, u_int32_t* put_num_of_lwords) ;





/***********************/
/*                     */
/*    s3100_control    */
/*                     */
/***********************/


int s3100_control_read(int p, int offset, u_int32_t* data) ;
int s3100_control_write(int p, int offset, u_int32_t data) ;



/***********************/
/*                     */
/*    s3100_sharc      */
/*                     */
/***********************/

int s3100_sharc_write(int p_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords) ;
int s3100_sharc_read(int p_sharc_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords) ;



/***********************/
/*                     */
/*    s3100_sdram      */
/*                     */
/***********************/

int s3100_sdram_write(int p_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords ) ;
int s3100_sdram_read(int p_sdram_desc, u_int32_t byte_adr, u_int32_t* ptr_data,  u_int32_t num_of_lwords )  ;














