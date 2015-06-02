/*
 * SimpleStandardPlane.cxx
 *
 *  Created on: Jun 9, 2011
 *      Author: stanitz
 */

#include <string>
#include <vector>
#include "include/SimpleStandardPlane.hh"

SimpleStandardPlane::SimpleStandardPlane(const std::string & name, const int id, const int maxX, const int maxY, const int tlu_event, const int pivot_pixel, OnlineMonConfiguration* mymon) : _name(name), _id(id), _maxX(maxX), _maxY(maxY),_binsX(maxX), _binsY(maxY)
{
  const int hits_reserve=500;
  _hits.reserve(hits_reserve);
  _badhits.reserve(hits_reserve); // allocate memory
  _clusters.reserve(hits_reserve);
  _rawhits.reserve(hits_reserve);
  for (int i=0; i< 4; i++)
  {
    _section_hits[i].reserve(hits_reserve);//FIXME hard-coded for Mimosa
    _section_clusters[i].reserve(hits_reserve);
  }

  mon=mymon;
  AnalogPixelType=false; //per default these are digital pixel planes
  // init these settings
  is_MIMOSA26=false;
  is_DEPFET=false;
  is_APIX=false;
  is_USBPIX=false;
  is_USBPIXI4=false;
  is_FORTIS=false;
  is_EXPLORER=false;
  is_UNKNOWN=true; // per default we don't know this plane
  isRotated=false;
  setPixelType(name); //set the pixel type
  _tlu_event=tlu_event;
  _pivot_pixel=pivot_pixel;
}

SimpleStandardPlane::SimpleStandardPlane(const std::string & name, const int id) : _name(name), _id(id), _maxX(-1), _maxY(-1) //FIXME we actually only need this type of constructor to form a map for histogramm allocation
{
  _hits.reserve(400);
  _badhits.reserve(400); //
  _clusters.reserve(40);
  mon=NULL; // no monitor given
  AnalogPixelType=false; //per default these are digital pixel planes
  // init these settings
  is_MIMOSA26=false;
  is_DEPFET=false;
  is_APIX=false;
  is_USBPIX=false;
  is_USBPIXI4=false;
  is_FORTIS=false;
  is_EXPLORER=false;
  is_UNKNOWN=true ; // per default we don't know this plane
  isRotated=false;
  setPixelType(name); //set the pixel type
  _binsX=-1;
  _binsY=-1;
}


void SimpleStandardPlane::addHit(SimpleStandardHit oneHit) {
  //oneHit.reduce(_binsX, _binsY); //also fills the appropriate sections //FIXME a better definition of badhits is needed

  _hits.push_back(oneHit);

  if ( (oneHit.getX()<0) || (oneHit.getY()<0) ||(oneHit.getX()>_maxX) ||(oneHit.getY()>_maxY))
  {
    _badhits.push_back(oneHit);
  }

  if  (is_MIMOSA26)
  {
    int section =oneHit.getX()/mon->getMimosa26_section_boundary();
    if ((section <0)||(section >=(int) mon->getMimosa26_max_sections()))
    {
      std::cout << "Error Section invalid: " << section << " "<< oneHit.getX()<< " "<< oneHit.getY()<<std::endl;
    }
    else
    {
      _section_hits[section].push_back(oneHit);
    }
  }
}

void SimpleStandardPlane::addRawHit(SimpleStandardHit oneHit)
{
  _rawhits.push_back(oneHit);
}

void SimpleStandardPlane::reducePixels(const int reduceX, const int reduceY) {
  _binsX = reduceX;
  _binsY = reduceY;
  //std::cout << "Reducing " << reduce << " -> " << _reduce <<" " << _maxX << " -> " << _binsX << " " << _maxY << " -> " << _binsY << std::endl;
}

void SimpleStandardPlane::doClustering() {
  int nClusters = 0;
  std::vector<int> clusterNumber;
  const int NOCLUSTER = -1000;
  const unsigned int minXDistance = 1;
  const unsigned int minYDistance = 1;
  const unsigned int npixels_hit=_hits.size();
  bool continue_flag;
  clusterNumber.assign(npixels_hit, NOCLUSTER);
  //std::cout << "Before big loop" << std::endl;
  //std::cout << "Name: " << _name << std::endl;

  // which planes to cluster, reject planes of Type Fortis
  if (is_FORTIS)
  {
    return;
  }

  if (npixels_hit > 1)// (npixels_hit < 2000 ))
  {
    std::sort(_hits.begin(), _hits.end(), SortHitsByXY());
    for (unsigned int aPixel = 0; aPixel < npixels_hit; aPixel++)
    {
      continue_flag = true;
      const SimpleStandardHit& aPix = _hits.at(aPixel);
      for (unsigned int bPixel = aPixel+1; bPixel < npixels_hit; bPixel++)
      {
        if (continue_flag != true)
          break;
        //for (std::vector<SimpleStandardHit>::const_iterator hit = _hits.begin();
        //   hit != _hits.end();
        //std::cout << aPixel << " " << bPixel << " " << _hits.size() << std::endl;
        const SimpleStandardHit& bPix = _hits.at(bPixel);
        unsigned int xDist = abs(aPix.getX() - bPix.getX());
        unsigned int yDist = abs(aPix.getY() - bPix.getY());

        if ( ( xDist <= minXDistance) && (yDist <= minYDistance)  )
        { //this means they are neighbors in x-direction && / this means they are neighbors in y-direction
          if ( (clusterNumber.at(aPixel) == NOCLUSTER) && clusterNumber.at(bPixel) == NOCLUSTER)
          { // none of these pixels have been assigned to a cluster
            //++nClusters;
            clusterNumber.at(aPixel) = ++nClusters;
            clusterNumber.at(bPixel) = nClusters;
          }
          else if ( (clusterNumber.at(aPixel) == NOCLUSTER) && (clusterNumber.at(bPixel) != NOCLUSTER))
          { // b was assigned already, a not
            clusterNumber.at(aPixel) = clusterNumber.at(bPixel);
          }
          else if ( (clusterNumber.at(aPixel) != NOCLUSTER) && (clusterNumber.at(bPixel) == NOCLUSTER))
          { // a was assigned already, b not
            clusterNumber.at(bPixel) = clusterNumber.at(aPixel);
          }
          else
          { //both pixels have a cluster number already
            int min = std::min(clusterNumber.at(aPixel), clusterNumber.at(bPixel));
            clusterNumber.at(aPixel) = min;
            clusterNumber.at(bPixel) = min;
          }
        }
        else
        {   // these pixels are not neighbored
          continue_flag = false;
        }
        /*
           if ( clusterNumber.at(aPixel) == NOCLUSTER )
           {
           ++nClusters;
           clusterNumber.at(aPixel) = nClusters;
           }
           if ( clusterNumber.at(bPixel) == NOCLUSTER )
           {
           ++nClusters;
           clusterNumber.at(bPixel) = nClusters;
           }
           }*/
    } // inner for loop
  } //outer for loop
  for(unsigned int aPixel = 0; aPixel < npixels_hit; aPixel++)
    if ( clusterNumber.at(aPixel) == NOCLUSTER )
    {
      ++nClusters;
      clusterNumber.at(aPixel) = nClusters;
    }
}
else
{ //You can't use the clustering algorithm with only one pixel
  if (npixels_hit == 1) clusterNumber.at(0) = 1;
}

//std::cout << "After big loop" << std::endl;
std::set<int> clusterSet (clusterNumber.begin(), clusterNumber.end());

/*for (unsigned int i=0; i<npixels_hit;++i) {
  if ( clusterNumber.at(i) != NOCLUSTER) {
//std::cerr << "Cluster with ID -1 in " << i << std::endl;
//} else {
clusterSet.insert(clusterNumber.at(i));
}
}*/
nClusters = clusterSet.size();

std::set<int>::iterator it;
for (it=clusterSet.begin();it!=clusterSet.end();++it)
{
  SimpleStandardCluster cluster;
  for (unsigned int i=0;i< npixels_hit;i++)
  {
    if (clusterNumber.at(i) == *it)
    { //Put only these pixels in that ClusterCollection that belong to that cluster
      cluster.addPixel(_hits.at(i));
    }
  }
  _clusters.push_back(cluster);
  //std::cout << "Cluster at: " << cluster.getX() << std::endl;
}
// if we have a mimosa, we need to fill the section information

if (is_MIMOSA26)
{
  for (unsigned int mycluster=0; mycluster<_clusters.size(); mycluster++)
  {
    unsigned int cluster_section=_clusters[mycluster].getX()/mon->getMimosa26_section_boundary();
    if (cluster_section<mon->getMimosa26_max_sections()) //fixme
    {
      _section_clusters[cluster_section].push_back(_clusters[mycluster]);
    }
  }
}
}

void SimpleStandardPlane::setPixelType(std::string name)
{
  if (name=="MIMOSA26")
  {
    is_MIMOSA26=true;
    is_UNKNOWN=false;
  }
  else if (name=="FORTIS")
  {
    is_FORTIS=true;
    is_UNKNOWN=false;
  }
  else if(name=="DEPFET")
  {
    is_DEPFET=true;
    is_UNKNOWN=false;
    AnalogPixelType=true;
  }
  else if(name=="APIX")
  {
    is_APIX=true;
    is_UNKNOWN=false;
    AnalogPixelType=true;
  }
  else if(name=="USBPIX")
  {
    is_USBPIX=true;
    is_UNKNOWN=false;
    AnalogPixelType=true;
  }
  else if(name=="USBPIXI4" || name=="USBPIXI4B")
  {
    is_USBPIXI4=true;
    is_UNKNOWN=false;
    AnalogPixelType=true;
  }
  else if(name=="Explorer20x20" || name=="Explorer30x30")
  {
    is_EXPLORER=true;
    is_UNKNOWN=false;
    AnalogPixelType=true;
  }
  else if(name=="pALPIDEfs")
  {
    is_UNKNOWN=false;
  }
  else
  {
    is_UNKNOWN=true;
  }
}
