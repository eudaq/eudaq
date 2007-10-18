/* The calling syntax is:

            [v] = oradNN(u,setpar)

*/

#include <stdio.h>
#include <math.h>

void radon(double *v,double *u,double *setpar)
{
  int m,n, M;
  int r, R;
  int t, T;
  int mmin,mmax,nmin,nmax;
  double sum,x_min,Delta_x,Delta_rho,rho_min,Delta_theta;
  double costheta,sintheta,theta; /*,x,y;*/
  double dx_isintheta,dx_icostheta/*,rho*/,rhooffset;
  double alpha,beta,betap,theta_min;
  double eps=1e-9;
  
  M            = setpar[0];
  x_min        = setpar[1];
  Delta_x      = setpar[2];
  T            = setpar[3];
  Delta_theta  = setpar[4];
  R            = setpar[5];
  rho_min      = setpar[6];
  Delta_rho    = setpar[7];
  theta_min    = setpar[8];
  for(t=0; t<T;t++)
  {
    theta=t*Delta_theta+theta_min;
    sintheta=sin(theta);
    costheta=cos(theta);
    rhooffset=x_min*(costheta+sintheta);
    if (sintheta>sqrt(0.5))
    {
      dx_isintheta=Delta_x/sintheta;
      alpha=-costheta/sintheta;
      for(r=0; r<R; r++ ) 
      {
        beta=(r*Delta_rho+rho_min-rhooffset)/(Delta_x*sintheta);
        betap=beta+0.5;
        if (alpha>eps)
        {
          mmin=(int)ceil(-(betap-eps)/alpha);
          mmax=1+(int)floor((M-betap-eps)/alpha);
        }
        else
          if (alpha<-eps)
          {
            mmin=(int)ceil((M-betap-eps)/alpha);
            mmax=1+(int)floor(-(betap-eps)/alpha);
          }
          else
            if ((betap>0) && (betap<M))
            {
              mmin=0;
              mmax=M;
            }
            else
            {
              mmin=0;
              mmax=-1;
            }
        if (mmin<0) mmin=0;
        if (mmax>M) mmax=M;

        sum=0.0;
        for (m=mmin;m<mmax;m++)
          sum+=u[m+M*(int)(betap+m*alpha)];
        v[t+T*r]=sum*dx_isintheta;
      }
    }
    else
    {
      dx_icostheta=Delta_x/fabs(costheta);
      alpha=-sintheta/costheta;
      for(r=0; r<R; r++ ) 
      {
        beta=(r*Delta_rho+rho_min-rhooffset)/(Delta_x*costheta);
        betap=beta+0.5;
        if (alpha>eps)
        {
          nmin=(int)ceil(-(beta+0.5-eps)/alpha);
          nmax=1+(int)floor((M-0.5-beta-eps)/alpha);
        }
        else 
          if (alpha<-eps)
          {
            nmin=(int)ceil((M-0.5-beta-eps)/alpha);
            nmax=1+(int)floor(-(beta+0.5-eps)/alpha);
          }
          else
            if ((betap>0) && (betap<M))
            {
              nmin=0;
              nmax=M;
            }
            else
            {
              nmin=0;
              nmax=-1;
            }
        if (nmin<0) nmin=0;
        if (nmax>M) nmax=M;

        sum=0.0;
        for (n=nmin;n<nmax;n++)
          sum+=u[(int)(betap+n*alpha)+n*M];
        v[t+T*r]=sum*dx_icostheta;
      }
    }
  }
}
