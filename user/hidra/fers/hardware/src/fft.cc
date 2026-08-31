/******************************************************************************

  CAEN SpA - Viareggio
  www.caen.it

  Files: 
  -----------------------------------------------------------------------------
  fft.c, fft.h

  Description:
  -----------------------------------------------------------------------------
  This is a function that calculates the FFT for a vector of 'ns' samples.
  The number ns must be a power of 2. In case it isn't, the closest power of 2
  smaller than ns will be considered and exceeding samples ignored.
  The calculation of the FFT is based on imaginary numbers (x = real part,
  y = imaginary part). However, the input vector is represented as real numbers
  (unsigned short) and the function returns a vector of real numbers (double)
  that are the amplitude (i.e. sqrt(x^2 + y^2) ) of the FFT points. 
  The amplitude is also normalized respect to the maximum amplitude (for 
  example 4096 for 12 bit samples) and expressed in dB. A contant baseline (for
  example 0.0000001 which is -140dB) is also added to the value in order to 
  low clip the FFT points. Since the FFT has two symmetrical lobes, only half
  points are returned.

  Input Parameters: 
  --------------------------------------------------------------------------
  wave: pointer to the input vector (waveform samples)
  fft: pointer to the output vector (fft amplitude in dB, half lobe)
  ns: number of samples of the input vector wave
  WindowType: Type of windowing for the FFT (see fft.h for more details)

  Return:
  --------------------------------------------------------------------------
  Number of pointf of the output vector fft

******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "fft.h"

#ifndef M_PI
#define M_PI 3.141592653
#endif

void getWindowingF(double *x, double *y, float *wave, int n, int WindowType) {
    int i;

    // apply the windowing to the input vector
    for(i=0; i<n; i++) {
        y[i] = 0.0; // imaginary part of the input vector (always 0)
        switch (WindowType) {
            case HANNING_FFT_WINDOW  :  
                x[i] = wave[i] * (0.5 - 0.5 * cos(2*M_PI * i/n));
                break;
            case HAMMING_FFT_WINDOW  :  
                x[i] = wave[i] * (0.54 - 0.46 * cos(2*M_PI * i/n)); 
                break;
            case BLACKMAN_FFT_WINDOW  :  
                x[i] = wave[i] * (2.4 * (0.42323 - 0.49755*cos(2*M_PI*i/n) + 0.07922*cos(4*M_PI*i/n)));
                break;
            case RECT_FFT_WINDOW  :  
                x[i] = wave[i];
                break;
            default :  
                x[i] = wave[i] * (2.4*(0.42323-0.49755*cos(2*M_PI*(i)/n)+0.07922*cos(4*M_PI*(i)/n)));
                break;
        }
    }
}

void getWindowing16(double *x, double *y, unsigned short *wave, int n, int WindowType) {
    int i;

    // apply the windowing to the input vector
    for(i=0; i<n; i++) {
        y[i] = 0.0; // imaginary part of the input vector (always 0)
        switch (WindowType) {
            case HANNING_FFT_WINDOW  :  
                x[i] = wave[i] * (0.5 - 0.5 * cos(2*M_PI * i/n));
                break;
            case HAMMING_FFT_WINDOW  :  
                x[i] = wave[i] * (0.54 - 0.46 * cos(2*M_PI * i/n)); 
                break;
            case BLACKMAN_FFT_WINDOW  :  
                x[i] = wave[i] * (2.4 * (0.42323 - 0.49755*cos(2*M_PI*i/n) + 0.07922*cos(4*M_PI*i/n)));
                break;
            case RECT_FFT_WINDOW  :  
                x[i] = wave[i];
                break;
            default :  
                x[i] = wave[i] * (2.4*(0.42323-0.49755*cos(2*M_PI*(i)/n)+0.07922*cos(4*M_PI*(i)/n)));
                break;
        }
    }
}

void getWindowing8(double *x, double *y, unsigned char *wave, int n, int WindowType) {
    int i;

    // apply the windowing to the input vector
    for(i=0; i<n; i++) {
        y[i] = 0.0; // imaginary part of the input vector (always 0)
        switch (WindowType) {
            case HANNING_FFT_WINDOW  :  
                x[i] = wave[i] * (0.5 - 0.5 * cos(2*M_PI * i/n));
                break;
            case HAMMING_FFT_WINDOW  :  
                x[i] = wave[i] * (0.54 - 0.46 * cos(2*M_PI * i/n)); 
                break;
            case BLACKMAN_FFT_WINDOW  :  
                x[i] = wave[i] * (2.4 * (0.42323 - 0.49755*cos(2*M_PI*i/n) + 0.07922*cos(4*M_PI*i/n)));
                break;
            case RECT_FFT_WINDOW  :  
                x[i] = wave[i];
                break;
            default :  
                x[i] = wave[i] * (2.4*(0.42323-0.49755*cos(2*M_PI*(i)/n)+0.07922*cos(4*M_PI*(i)/n)));
                break;
        }
    }
}

int FFT (void *wave, double *fft, int ns, int WindowType, int SampleType)
{
    int m,n,ip,le,le1,nm1,k,l,j,i,nv2;
    double u1,u2,u3,arg,c,s,t1,t2,t3,t4;
    double *x, *y;

    // ns should be a power of 2. If it is not, find the maximum m
    // such that n = 2^m < ns and get n samples instead of ns.
    i = 1;
    while((int)pow(2,i) < ns)
        i++;
    if(ns == (int)(pow(2,i))) {
        m = i;
        n = ns;
    }
    else {
        m = i - 1;
        n = (int)pow(2,m);
    }

    // allocate the memory buffers for the real and imaginary parts of the fft
    x = static_cast<double*>(malloc(n * sizeof(double)));
    y = static_cast<double*>(malloc(n * sizeof(double)));
    //x = malloc(n * sizeof(double));
    //y = malloc(n * sizeof(double));

    // apply the windowing to the input vector
    if(SampleType==SAMPLETYPE_UINT8)
        getWindowing8(x, y, (unsigned char *)wave, n, WindowType);
    else if(SampleType==SAMPLETYPE_UINT16)
        getWindowing16(x, y, (unsigned short *)wave, n, WindowType);
    else if(SampleType==SAMPLETYPE_FLOAT)
        getWindowingF(x, y, (float *)wave, n, WindowType);
    else {
        free(x);
        free(y);
        return -1;
    }

    // now the vectors x and y represents the input waveform expressed as imaginary numbers
    // after the appplication of the windowing.

    // calculate the FFT
    for(l=0; l<m; l++) {
        le=(int)pow(2,m-l);
        le1=le/2;
        u1=1.0;
        u2=0.0;
        arg=M_PI/le1;
        c=cos(arg);
        s=-sin(arg);

        for (j=0; j<le1; j++) {
            for (i=j; i<n; i=i+le) {
                ip=i+le1;
                t1=x[i]+x[ip];
                t2=y[i]+y[ip];
                t3=x[i]-x[ip];
                t4=y[i]-y[ip];
                x[ip]=t3*u1-t4*u2;
                y[ip]=t4*u1+t3*u2;
                x[i]=t1;
                y[i]=t2;
            }
        u3=u1*c-u2*s;
        u2=u2*c+u1*s;
        u1=u3;
        }
    }

    nv2=n/2;
    nm1=n-1;
    j=0;
    i=0;
    while (i<nm1) {
        if(i>j)
            goto rif;
        t1=x[j];
        t2=y[j];
        x[j]=x[i];
        y[j]=y[i];
        x[i]=t1;
        y[i]=t2;
        rif:
        k=nv2;
        rife:
        if (k>j)
            goto rifer;
        j=j-k;
        k=k/2;
        goto rife;
        rifer:
        j=j+k;
        i++;
    }

    // get the amplitude of the FFT (modulo)
    y[0]=y[0]/n;
    x[0]=x[0]/n;
    fft[0]=sqrt(x[0]*x[0] + y[0]*y[0]);
    for(i=1;i<n/2;i++) {
        y[i]=2*y[i]/n;
        x[i]=2*x[i]/n;
        fft[i]=sqrt(x[i]*x[i] + y[i]*y[i]);
    }

    // Add the baseline, normalize and transform in dB
    for(i=0; i<n/2; i++) 
        fft[i] = 20 * log10( fft[i]/NORM_FACTOR + FFT_BASELINE);

    // free the buffers and return the number of points (only half FFT)
    free(x);
    free(y);

    return (n/2);
}

