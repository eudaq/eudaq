#ifndef EUDAQ_INCLUDED_Desynccorr_Clustering
#define EUDAQ_INCLUDED_Desynccorr_Clustering

#include <vector>

struct pixel_hit {
  int x;
  int y;
  pixel_hit(int x, int y): 
    x(x), y(y) {}
  pixel_hit() = delete;
};

struct cluster {
  double x;
  double y;
  std::vector<pixel_hit> pixel_hits;
  cluster(double x, double y) : x(x), y(y) {}
  cluster(double x, double y, std::vector<pixel_hit> &&pixel_hits) : x(x), y(y), pixel_hits(std::move(pixel_hits)) {}
  cluster() = delete;
};

//poor man's (woman's, person's?) clustering
std::vector<cluster> clusterHits(std::vector<pixel_hit> const & hits, int spatCutSqrd){
  //single pixel_hit events are easy, we just got one single hit cluster
  if(hits.size() == 1){
    std::vector<cluster> result;
    auto const & pix = hits[0];
    //For single pixel clusters the cluster position is in the centre of the pixel
    //Since we start counting at pixel 1 (not at 0), this is shifter by -0.5 px //check if this holds in EUDAQ, this was for something else
    result.emplace_back(pix.x-0.5, pix.y-0.5);
    result.back().pixel_hits = hits;
    return result;
  //multi pixel_hit events are more complicated
  } else {
    std::vector<pixel_hit> hitPixelVec = hits;
    std::vector<pixel_hit> newlyAdded;    
    std::vector<cluster> clusters;

    while( !hitPixelVec.empty() ) {
      //this is just a placeholder cluster so far, we will add hits and do the CoG computation later
      clusters.emplace_back(-1.,-1);      
      newlyAdded.push_back( hitPixelVec.front() );
      clusters.back().pixel_hits.push_back( hitPixelVec.front() );
      hitPixelVec.erase( hitPixelVec.begin() );
      
      while( !newlyAdded.empty() ) {
        bool newlyDone = true;
        int  x1, x2, y1, y2, dX, dY;

        for( auto candidate = hitPixelVec.begin(); candidate != hitPixelVec.end(); ++candidate ){
          //get the relevant infos from the newly added pixel
          x1 = newlyAdded.front().x;
          y1 = newlyAdded.front().y;

          //and the pixel we test against
          x2 = candidate->x;
          y2 = candidate->y;
          dX = x1-x2;
          dY = y1-y2;
          int spatDistSqrd = dX*dX+dY*dY;
  
          if( spatDistSqrd <= spatCutSqrd ) {
            newlyAdded.push_back( *candidate );  
            clusters.back().pixel_hits.push_back( *candidate );
            hitPixelVec.erase( candidate );
            newlyDone = false;
            break;
          }
        }
        if(newlyDone) {
          newlyAdded.erase(newlyAdded.begin());
        }
      }
    }
    //do the CoG cluster computation
    for(auto& c: clusters) {
      float x_sum = 0;
      float y_sum = 0;
      for(auto const & h: c.pixel_hits) {
        x_sum += h.x;
        y_sum += h.y;
            }
      c.x = x_sum/c.pixel_hits.size()-0.5;
      c.y = y_sum/c.pixel_hits.size()-0.5;
    }

  return clusters;
  }      
}

#endif // EUDAQ_INCLUDED_Desynccorr_Clustering