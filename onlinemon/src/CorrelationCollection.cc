/*
 * CorrelationCollection.cc
 *
 *  Created on: Jun 17, 2011
 *      Author: stanitz
 */

#include "CorrelationCollection.hh"
#include "OnlineMon.hh"

CorrelationCollection::CorrelationCollection()
: BaseCollection(),
  _mapOld(),
  _map(),
  _planes(),
  skip_this_plane(), 
  correlateAllPlanes(false),
  selected_planes_to_skip(),
  planesNumberForCorrelation(0),
  windowWidthForCorrelation(0)
{
  cout << " Initializing Correlation Collection"<<endl;
  CollectionType = CORRELATION_COLLECTION_TYPE;
}

bool checkIfClusterIsBigEnough(SimpleStandardCluster oneCluster)
{
  if (oneCluster.getNPixel() == 1){
    //(Phill) Should this say that NPixel is equal to one or greater than or equal to one?
    return true;
  }
  return false;
}

bool CorrelationCollection::isPlaneRegistered(const SimpleStandardPlane p)
{
  vector<SimpleStandardPlane>::iterator it = find(_planes.begin(), _planes.end(), p);

  if (it == _planes.end()){
    //(Phill) So this says that if the plane you are checking is exactly one beyond the last point in the vector, return false. What if p isn't in the vector at all?
    return false;
  }
  return true;
}

bool CorrelationCollection::checkCorrelations(const SimpleStandardCluster &cluster1, const SimpleStandardCluster &cluster2, const bool only_mimos_planes = true)
{
  int diffParam;

  if (only_mimos_planes == true){
    diffParam = getWindowWidthForCorrelation();
  } else{
    return false; //(Phill) Changed this from true to false because I think if it can only correlate mimosa planes then it should fail if there is anything else
  }

  if (abs(cluster1.getX() - cluster2.getX()) < diffParam && abs(cluster1.getY() - cluster2.getY()) < diffParam){
    //This says that if the x and y differences of the two clusters is less than the 'window width for correlation' then the check is successful, otherwise it fails
    return true;
  } else{
    return false;
  }
}


void CorrelationCollection::fillHistograms(const SimpleStandardPlaneDouble &simpPlaneDouble)
{
  CorrelationHistos *corrmap = _mapOld[simpPlaneDouble];

  if (corrmap != NULL) {
    const vector< SimpleStandardCluster > aClusters = simpPlaneDouble.getPlane1().getClusters();
    const vector< SimpleStandardCluster > bClusters = simpPlaneDouble.getPlane2().getClusters();

    for (unsigned int acluster = 0; acluster < aClusters.size(); acluster++)
    {
      const SimpleStandardCluster &oneAcluster = aClusters.at(acluster);
      if (oneAcluster.getNPixel() < _mon->mon_configdata.getCorrel_minclustersize()) // we are only interested in clusters with several pixels
      {
        continue;
      }
      for (unsigned int bcluster = 0; bcluster < bClusters.size();bcluster++)
      {
        const SimpleStandardCluster& oneBcluster = bClusters.at(bcluster);
        //
        if (oneBcluster.getNPixel() <_mon->mon_configdata.getCorrel_minclustersize())
        {
          continue;
        }
        corrmap->Fill(oneAcluster, oneBcluster);
      }
    }
  }
}

void CorrelationCollection::setRootMonitor(RootMonitor *mon)  {_mon = mon; }

CorrelationHistos * CorrelationCollection::getCorrelationHistos(SimpleStandardPlaneDouble pd)
{

  return _mapOld[pd];
}
CorrelationHistos * CorrelationCollection::getCorrelationHistos(const SimpleStandardPlane& p1, const SimpleStandardPlane& p2)
{
  std::pair<SimpleStandardPlane ,SimpleStandardPlane> plane (p1,p2);
  return _map[plane];
}

void CorrelationCollection::Reset()
{
  std::map<std::pair<SimpleStandardPlane,SimpleStandardPlane>, CorrelationHistos*>::iterator it;
  for (it = _map.begin(); it != _map.end(); ++it)
  {
    (*it).second->Reset();
  }
}

void CorrelationCollection::Fill(const SimpleStandardEvent &simpev)
{
  //int totalFills = 0;
  int nPlanes = simpev.getNPlanes();
  int nPlanes_disabled=0;

  unsigned int plane_vector_size=0;
  if (skip_this_plane.size()==0) // do this only at the very first event
  {
    selected_planes_to_skip=_mon->mon_configdata.getPlanes_to_be_skipped();
    skip_this_plane.reserve(nPlanes);
    //init vector
    for (int elements=0; elements<nPlanes; elements++)
    {
      skip_this_plane[elements]=false;
    }
    // now get vector of planes to be disabled and set the corresponding entries to true
    for (unsigned int skipplanes=0; skipplanes<selected_planes_to_skip.size(); skipplanes++)
    {
      if ((selected_planes_to_skip[skipplanes]>0) && (selected_planes_to_skip[skipplanes]<nPlanes))
      {
        skip_this_plane[selected_planes_to_skip[skipplanes]]=true;
        std::cout << "CorrelationCollection : Disabling Plane "<< selected_planes_to_skip[skipplanes] <<endl;
        nPlanes_disabled++;
      }
    }
    if (nPlanes_disabled > 0)
      std::cout << "CorrelationCollection : Disabling "<<  nPlanes_disabled << " Planes" << endl;
  }
  if (nPlanes-nPlanes_disabled<2)
  {
    std::cout << "CorrelationCollection : Too Many Planes Disabled ..." <<endl;
  }
  else
  {
    for (int planeA = 0; planeA < nPlanes; planeA++)
    {
      const SimpleStandardPlane& simpPlane = simpev.getPlane(planeA);
#ifdef DEBUG
      std::cout << "CorrelationCollection : Checking Plane " << simpPlane.getName() << " " <<simpPlane.getID() << "..." <<  std::endl;
      std::cout<<  "CorrelationCollection : " << planeA<<  std::endl;
#endif

      if (!isPlaneRegistered(simpPlane)  )
      {
#ifdef DEBUG
        std::cout << "CorrelationCollection: Plane " << simpPlane.getName() << " " <<simpPlane.getID() << " is not registered" << std::endl;
#endif

        plane_vector_size=_planes.size(); //how many planes we did look at beforehand
        if (correlateAllPlanes)
        {
          for (unsigned int oldPlanes = 0 ; oldPlanes <plane_vector_size ; oldPlanes++)
          {
            registerPlaneCorrelations(_planes.at(oldPlanes), simpPlane); // Correlating this plane with all the other ones
          }
        }
        else // we have deselected a few planes
        {
          if  (!skip_this_plane[planeA])
          {
            for (unsigned int oldPlanes = 0 ; oldPlanes <plane_vector_size ; oldPlanes++)
            {
              if (!skip_this_plane[oldPlanes])
              {
                registerPlaneCorrelations(_planes.at(oldPlanes), simpPlane); // Correlating this plane with all the other ones
              }

            }
          }
        }
        _planes.push_back(simpPlane); // we have to deal with all planes

      }
      for (int planeB = planeA +1; planeB < nPlanes; planeB++)
      {
        if ((skip_this_plane[planeA]==false)&& (skip_this_plane[planeB])==false)
        {
          const SimpleStandardPlane & p1=simpev.getPlane(planeA);
          const SimpleStandardPlane & p2=simpev.getPlane(planeB);
          fillHistograms(p1,p2);
        }

      }
    }
  }
}

unsigned int CorrelationCollection::FillWithTracks(const SimpleStandardEvent &simpev)
{
  int nPlanes = simpev.getNPlanes();
  int nPlanes_disabled=0;
  std::vector < vector<SimpleStandardCluster> > clustersInPlanes;
  std::vector < vector< pair<int, SimpleStandardCluster> > > reconstructedTracks;
  std::vector < pair<int, SimpleStandardCluster> > singleTrack;
  singleTrack.reserve(nPlanes);
  bool noClusterFound;
  const int lastPlane = nPlanes-1;
  SimpleStandardCluster tempCluster;
  bool correlationDecission;

  unsigned int plane_vector_size=0;
  if (skip_this_plane.size()==0) // do this only at the very first event
  {
    selected_planes_to_skip=_mon->mon_configdata.getPlanes_to_be_skipped();
    skip_this_plane.reserve(nPlanes);
    //init vector
    for (int elements=0; elements<nPlanes; elements++)
    {
      skip_this_plane[elements]=false;
    }
    // now get vector of planes to be disabled and set the corresponding entries to true
    for (unsigned int skipplanes=0; skipplanes<selected_planes_to_skip.size(); skipplanes++)
    {
      if ((selected_planes_to_skip[skipplanes]>0) && (selected_planes_to_skip[skipplanes]<nPlanes))
      {
        skip_this_plane[selected_planes_to_skip[skipplanes]]=true;
        std::cout << "CorrelationCollection : Disabling Plane "<< selected_planes_to_skip[skipplanes] <<endl;
        nPlanes_disabled++;
      }
    }
    if (nPlanes_disabled > 0)
      std::cout << "CorrelationCollection : Disabling "<<  nPlanes_disabled << " Planes" << endl;
  }
  if (nPlanes-nPlanes_disabled<2)
  {
    std::cout << "CorrelationCollection : Too Many Planes Disabled ..." <<endl;
  }
  else
  {
    clustersInPlanes.reserve(nPlanes);

    for (int planeA = 0; planeA < nPlanes; planeA++)
    {
      const SimpleStandardPlane& simpPlane = simpev.getPlane(planeA);
      if (skip_this_plane[planeA] == false) //adding plane for analysis if selected
      {
        vector<SimpleStandardCluster> clustersBeforeDeletion = simpPlane.getClusters();
        vector<SimpleStandardCluster> clustersAfterDeletion;
        clustersAfterDeletion.reserve(20);
        remove_copy_if(clustersBeforeDeletion.begin(), clustersBeforeDeletion.end(),
            back_inserter(clustersAfterDeletion), checkIfClusterIsBigEnough);
        //                cout<< "Clusters: " << clustersAfterDeletion.size() << endl;
        clustersInPlanes.push_back( clustersAfterDeletion );

        if (!isPlaneRegistered(simpPlane)  )
        {
          plane_vector_size=_planes.size(); //how many planes we did look at beforehand

          if (correlateAllPlanes)
            for (unsigned int oldPlanes = 0 ; oldPlanes <plane_vector_size ; oldPlanes++)
              registerPlaneCorrelations(_planes.at(oldPlanes), simpPlane); // Correlating this plane with all the other ones

          else // we have deselected a few planes
            if  (!skip_this_plane[planeA])
              for (unsigned int oldPlanes = 0 ; oldPlanes <plane_vector_size ; oldPlanes++)
                if (!skip_this_plane[oldPlanes])
                  registerPlaneCorrelations(_planes.at(oldPlanes), simpPlane); // Correlating this plane with all the other ones
          _planes.push_back(simpPlane); // we have to deal with all planes
        }
      }
    }
  }
  for (unsigned int plane = 0; plane < clustersInPlanes.size(); ++plane)
    std::sort(clustersInPlanes.at(plane).begin(), clustersInPlanes.at(plane).end(), SortClustersByXY());

  for (unsigned int currPlaneIndex = 0; currPlaneIndex < clustersInPlanes.size()-2; ++currPlaneIndex)
  {
    std::vector<SimpleStandardCluster> & currentPlane = clustersInPlanes.at(currPlaneIndex);

    singleTrack.clear();
    while(currentPlane.size() > 0)
    {
      if (singleTrack.size() > 0)
        singleTrack.clear();

      while(singleTrack.size() == 0 && currentPlane.size() > 0)
      {
        tempCluster = currentPlane.back();
        if(tempCluster.getNPixel() >= _mon->mon_configdata.getCorrel_minclustersize())
        {
          std::pair<int, SimpleStandardCluster> trackPair(currPlaneIndex, tempCluster);
          singleTrack.push_back(trackPair);
        }
        currentPlane.pop_back();
      }

      if (singleTrack.size() > 0)
      {
        for (int nextPlaneIndex = ++currPlaneIndex; nextPlaneIndex < nPlanes; ++nextPlaneIndex)
        {
          std::vector<SimpleStandardCluster> & clustersInNextPlane = clustersInPlanes.at(nextPlaneIndex);
          noClusterFound = false;

          //                    for(std::vector<SimpleStandardCluster>::iterator it = --nextPlane.end(); it >= nextPlane.begin(); --it)
          //                    int currentSize = clustersInNextPlane.size();
          for(int currentclusterIndex = clustersInNextPlane.size()-1;
              currentclusterIndex >= 0; --currentclusterIndex)
          {
            SimpleStandardCluster & currentCluster = clustersInNextPlane.at(currentclusterIndex);
            //                        if( currentCluster.getNPixel() >= _mon->mon_configdata.getCorrel_minclustersize()
            //                                && checkCorrelations(singleTrack.back().second, currentCluster) )

            if (simpev.getPlane(currPlaneIndex).is_MIMOSA26 && simpev.getPlane(nextPlaneIndex).is_MIMOSA26)
              correlationDecission = checkCorrelations(singleTrack.back().second, currentCluster);
            else
              correlationDecission = checkCorrelations(singleTrack.back().second, currentCluster, false);
            if( correlationDecission )
            {
              std::pair<int, SimpleStandardCluster>
                trackPair(nextPlaneIndex, currentCluster);
              singleTrack.push_back(trackPair);
              clustersInNextPlane.erase(clustersInNextPlane.begin()+currentclusterIndex);
              break;
            }
            else if(currentclusterIndex == 0)
              noClusterFound = true;
          }

          if(nextPlaneIndex == lastPlane || noClusterFound)
          {
            if (singleTrack.size() >= getPlanesNumberForCorrelation())
              reconstructedTracks.push_back(singleTrack);
            //                        std::cout<< "SIZE: " << reconstructedTracks.size() <<endl;
          }
        }
      }
    }
  }
  fillHistograms(reconstructedTracks, simpev);
  return reconstructedTracks.size();
}

void CorrelationCollection::fillHistograms(std::vector < vector<
    pair<int, SimpleStandardCluster> > > tracks,
    const SimpleStandardEvent & simpEv)
{
  //    std::vector< vector< pair<int, SimpleStandardCluster> > >::iterator track;
  //    std::vector< pair<int, SimpleStandardCluster> >::iterator planeClusterPair1;
  //    std::vector< pair<int, SimpleStandardCluster> >::iterator planeClusterPair2;

  for(unsigned int trackNr = 0; trackNr < tracks.size(); ++trackNr)
  {
    vector< pair<int, SimpleStandardCluster> > & currentTrack = tracks.at(trackNr);
    for(unsigned int clusterPair1 = 0; clusterPair1 < currentTrack.size()-1; ++clusterPair1)
    {
      for(unsigned int clusterPair2 = clusterPair1+1; clusterPair2 < currentTrack.size(); ++clusterPair2)
      {
        const SimpleStandardPlane & firstPlane = simpEv.getPlane(currentTrack.at(clusterPair1).first);
        const SimpleStandardCluster & firstCluster = currentTrack.at(clusterPair1).second;
        const SimpleStandardPlane & secondPlane = simpEv.getPlane(currentTrack.at(clusterPair2).first);
        const SimpleStandardCluster & secondCluster = currentTrack.at(clusterPair2).second;
        pair<SimpleStandardPlane, SimpleStandardPlane> planePair(firstPlane, secondPlane);
        CorrelationHistos *corrmap = _map[planePair];

        corrmap->Fill(firstCluster, secondCluster);
      }
    }
  }
}

void CorrelationCollection::fillHistograms(const SimpleStandardPlane& p1, const SimpleStandardPlane& p2)
{

  std::pair<SimpleStandardPlane ,SimpleStandardPlane> plane (p1,p2);
  CorrelationHistos *corrmap = _map[plane];
  if (corrmap == NULL)
  {
    std::cout << "CorrelationCollection: Histogram not registered ...yet  " << p1.getName()<< " "<<p1.getID() <<" / "<< p2.getName()<<" "<<p2.getID()<<std::endl;
  }
  else
  {
    const std::vector<SimpleStandardCluster> aClusters=p1.getClusters();
    const std::vector<SimpleStandardCluster> bClusters=p2.getClusters();


    for (unsigned int acluster = 0; acluster < aClusters.size();acluster++)
    {
      const SimpleStandardCluster& oneAcluster = aClusters.at(acluster);
      if (oneAcluster.getNPixel() < _mon->mon_configdata.getCorrel_minclustersize()) // we are only interested in clusters with several pixels
      {
        continue;
      }
      for (unsigned int bcluster = 0; bcluster < bClusters.size();bcluster++)
      {
        const SimpleStandardCluster& oneBcluster = bClusters.at(bcluster);
        //
        if (oneBcluster.getNPixel() < _mon->mon_configdata.getCorrel_minclustersize())
        {
          continue;
        }
        else
        {
          corrmap->Fill(oneAcluster, oneBcluster);
        }
      }
    }
  }
}

void CorrelationCollection::registerPlaneCorrelations(const SimpleStandardPlane& p1, const SimpleStandardPlane& p2)
{

#ifdef DEBUG
  std::cout << "CorrelationCollection:: Correlating: " << p1.getName() << " " <<p1.getID() << " with " <<  p2.getName() << " " <<p2.getID() << std::endl;

#endif
  CorrelationHistos *tmphisto = new CorrelationHistos( p1, p2);
  //SimpleStandardPlaneDouble planeDouble(p1,p2);
  //_map[planeDouble] = tmphisto;
  pair<SimpleStandardPlane,SimpleStandardPlane> pdouble(p1,p2);
  _map[pdouble]=tmphisto;



  if (_mon != NULL)
  {
    cout << "HitmapCollection:: Monitor running in online-mode" << endl;
    std::string dirName;

    if(_mon->getUseTrack_corr() == true)
      dirName = "Track Correlations";
    else
      dirName = "Correlations";

    char tree[1024];
    sprintf(tree,"%s/%s %i/%s %i in X", dirName.c_str(), p1.getName().c_str(),p1.getID(),p2.getName().c_str(),p2.getID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,getCorrelationHistos(p1,p2)->getCorrXHisto(), "COLZ",0);

    sprintf(tree,"%s/%s %i/%s %i in Y", dirName.c_str(), p1.getName().c_str(),p1.getID(),p2.getName().c_str(),p2.getID());
    _mon->getOnlineMon()->registerTreeItem(tree);
    _mon->getOnlineMon()->registerHisto(tree,getCorrelationHistos(p1,p2)->getCorrYHisto(), "COLZ",0);

    sprintf(tree,"%s/%s %i", dirName.c_str(), p1.getName().c_str(),p1.getID());
    _mon->getOnlineMon()->makeTreeItemSummary(tree);
  }
}

bool CorrelationCollection::getCorrelateAllPlanes() const
{
  return correlateAllPlanes;
}

std::vector<int> CorrelationCollection::getSelected_planes_to_skip() const
{
  return selected_planes_to_skip;
}

void CorrelationCollection::setCorrelateAllPlanes(bool correlateAllPlanes)
{
  this->correlateAllPlanes = correlateAllPlanes;
}

void CorrelationCollection::setSelected_planes_to_skip(std::vector<int> selected_planes_to_skip)
{
  this->selected_planes_to_skip = selected_planes_to_skip;
}

void CorrelationCollection::Write(TFile *file) {

  if (file==NULL) // if the file pointer is null jump back
  {
    cout << "Can't Write Correllation Collections " <<endl;
    return;
  }
  if (_mon->getUseTrack_corr() == true)
  {
    gDirectory->mkdir("Track Correlations");
    gDirectory->cd("Track Correlations");
  }
  else
  {
    gDirectory->mkdir("Correlations");
    gDirectory->cd("Correlations");
  }
  std::map<std::pair<SimpleStandardPlane,SimpleStandardPlane>, CorrelationHistos*>::iterator it;

  for (it = _map.begin(); it != _map.end(); ++it) {
    //char sensorfolder[255]="";
    //sprintf(sensorfolder,"%s_%d:%s_%d",it->first.getPlane1().getName().c_str(),it->first.getPlane1().getID(), it->first.getPlane2().getName().c_str(),it->first.getPlane2().getID());
    //gDirectory->mkdir(sensorfolder);
    //gDirectory->cd(sensorfolder);
    it->second->Write();


    //gDirectory->cd("..");
  }
  gDirectory->cd("..");
}
