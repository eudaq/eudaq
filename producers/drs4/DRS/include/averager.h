/********************************************************************\

  Name:         averager.h
  Created by:   Stefan Ritt

  Contents:     Robust averager

  $Id: averager.h 21220 2013-12-20 13:47:43Z ritt $

\********************************************************************/

class Averager {
   int fNx, fNy, fNz, fDim;
   float *fArray;
   unsigned short *fN;
   
public:
   Averager(int nx, int ny, int nz, int dim);
   ~Averager();
   
   void Add(int x, int y, int z, float value);
   void Reset();
   double Average(int x, int y, int z);
   double Median(int x, int y, int z);
   double RobustAverage(double range, int x, int y, int z);
   int SaveNormalizedDistribution(const char *filename, int x, float range);

};
