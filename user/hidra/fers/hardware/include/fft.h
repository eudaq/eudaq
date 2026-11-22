/******************************************************************************
* 
* CAEN SpA - Front End Division
* Via Vetraia, 11 - 55049 - Viareggio ITALY
* +390594388398 - www.caen.it
*
***************************************************************************//**
* \note TERMS OF USE:
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation. This program is distributed in the hope that it will be useful, 
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. The user relies on the 
* software, documentation and results solely at his own risk.
******************************************************************************/

#ifndef __FFT_H
#define __FFT_H

#define FFT_BASELINE  0.0000001  // Baseline for low clipping (-140dB)
#define NORM_FACTOR   4096       // Normalize the amplitide (12bit ADC)

// Types of windowing
#define HANNING_FFT_WINDOW    0
#define HAMMING_FFT_WINDOW    1
#define BLACKMAN_FFT_WINDOW   2
#define RECT_FFT_WINDOW       3

#define SAMPLETYPE_UINT8      0
#define SAMPLETYPE_UINT16     1
#define SAMPLETYPE_FLOAT      2

/*
  Input Parameters: 
  --------------------------------------------------------------------------
  wave: pointer to the input vector (waveform samples)
  fft: pointer to the output vector (fft amplitude in dB, half lobe)
  ns: number of samples of the input vector wave
  WindowType: Type of windowing for the FFT (see fft.h for more details)

  Return:
  --------------------------------------------------------------------------
  Number of pointf of the output vector fft
*/

int FFT (void *wave, double *fft, int ns, int WindowType, int SampleType);

#endif // __FFT_H
