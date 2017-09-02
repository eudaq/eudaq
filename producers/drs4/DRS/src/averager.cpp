/********************************************************************\

  Name:         averager.cpp
  Created by:   Stefan Ritt

  Contents:     Robust averager

  $Id: averager.cpp 21210 2013-12-12 11:36:59Z ritt $

\********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "averager.h"

/*----------------------------------------------------------------*/

Averager::Averager(int nx, int ny, int nz, int dim)
{
   fNx = nx;
   fNy = ny;
   fNz = nz;
   fDim = dim;
   
   int size = sizeof(float)*nx*ny*nz * dim;
   fArray = (float *)malloc(size);
   assert(fArray);
   memset(fArray, 0, size);
   size = sizeof(float)*nx*ny*nz;
   fN = (unsigned short *)malloc(size);
   assert(fN);
   memset(fN, 0, size);
}

/*----------------------------------------------------------------*/

Averager::~Averager()
{
   if (fN)
      free(fN);
   if (fArray)
      free(fArray);
   fN = NULL;
   fArray = NULL;
}

/*----------------------------------------------------------------*/

void Averager::Add(int x, int y, int z, float value)
{
   assert(x < fNx);
   assert(y < fNy);
   assert(z < fNz);
   
   int nIndex = (x*fNy + y)*fNz + z;
   if (fN[nIndex] == fDim - 1) // check if array full
      return;
   
   int aIndex = ((x*fNy + y)*fNz + z) * fDim + fN[nIndex];
   fN[nIndex]++;
   fArray[aIndex] = value;
}

/*----------------------------------------------------------------*/

void Averager::Reset()
{
   int size = sizeof(float)*fNx*fNy*fNz * fDim;
   memset(fArray, 0, size);
   size = sizeof(float)*fNx*fNy*fNz;
   memset(fN, 0, size);
}

/*----------------------------------------------------------------*/

int compar(const void *a, const void *b);

int compar(const void *a, const void *b)
{
   if (*((float *)a) == *((float *)b))
      return 0;
   
   return (*((float *)a) < *((float *)b)) ? -1 : 1;
}
           
double Averager::Average(int x, int y, int z)
{
   assert(x < fNx);
   assert(y < fNy);
   assert(z < fNz);

   double a = 0;
   
   int nIndex = (x*fNy + y)*fNz + z;
   int aIndex = ((x*fNy + y)*fNz + z) * fDim;

   for (int i=0 ; i<fN[nIndex] ; i++)
      a += fArray[aIndex + i];
   
   if (fN[nIndex] > 0)
      a /= fN[nIndex];
   
   return a;
}

/*----------------------------------------------------------------*/

double Averager::Median(int x, int y, int z)
{
   assert(x < fNx);
   assert(y < fNy);
   assert(z < fNz);
   
   double m = 0;
   
   int nIndex = (x*fNy + y)*fNz + z;
   int aIndex = ((x*fNy + y)*fNz + z) * fDim;
   
   qsort(&fArray[aIndex], fN[nIndex], sizeof(float), compar);
   m = fArray[aIndex + fN[nIndex]/2];
   
   return m;
}

/*----------------------------------------------------------------*/

double Averager::RobustAverage(double range, int x, int y, int z)
{
   assert(x < fNx);
   assert(y < fNy);
   assert(z < fNz);
   
   double ra = 0;
   int n = 0;
   double m = Median(x, y, z);
   
   int nIndex = (x*fNy + y)*fNz + z;
   int aIndex = ((x*fNy + y)*fNz + z) * fDim;
   
   for (int i=0 ; i<fN[nIndex] ; i++) {
      if (fArray[aIndex + i] > m - range && fArray[aIndex + i] < m + range) {
         ra += fArray[aIndex + i];
         n++;
      }
   }
   
   if (n > 0)
      ra /= n;

   //if (y == 0 && z == 7 && fN[nIndex] > 10)
   //   printf("%d %lf %lf %lf\n", fN[nIndex], a, m, ra);
   
   return ra;
}

/*----------------------------------------------------------------*/

int Averager::SaveNormalizedDistribution(const char *filename, int x, float range)
{
   assert(x < fNx);
   FILE *f = fopen(filename, "wt");
   
   if (!f)
      return 0;
   
   fprintf(f, "X, Y, Z, Min, Max, Ave, Sigma\n");
   
   for (int y=0 ; y<fNy ; y++)
      for (int z=0 ; z<fNz ; z++) {
         
         int nIndex = (x*fNy + y)*fNz + z;
         int aIndex = ((x*fNy + y)*fNz + z) * fDim;
         
         if (fN[nIndex] > 1) {
            fprintf(f, "%d,%d, %d, ", x, y, z);
            
            double s = 0;
            double s2 = 0;
            double min = 0;
            double max = 0;
            int n = fN[nIndex];
            double m = Median(x, y, z);
            
            for (int i=0 ; i<n ; i++) {
               double v = fArray[aIndex + i] - m;
               s += v;
               s2 += v*v;
               if (v < min)
                  min = v;
               if (v > max)
                  max = v;
            }
            double sigma = sqrt((n * s2 - s * s) / (n * (n-1)));
            double average = s / n;
            
            fprintf(f, "%3.1lf, %3.1lf, %3.1lf, %3.3lf, ", min, max, average, sigma);
            
            if (min < -range || max > range) {
               for (int i=0 ; i<n ; i++)
                  fprintf(f, "%3.1lf,", fArray[aIndex + i] - m);
            }
            
            fprintf(f, "\n");
         }
      }
   
   fclose(f);
   return 1;
}

