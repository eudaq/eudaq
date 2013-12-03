/*
 * SimpleStandardCluster.hh
 *
 *  Created on: Jun 9, 2011
 *      Author: stanitz
 */

#ifndef SIMPLESTANDARDCLUSTER_HH_
#define SIMPLESTANDARDCLUSTER_HH_

#include "include/SimpleStandardHit.hh"

class SimpleStandardCluster {
  protected:
    std::vector<SimpleStandardHit> _hits;

  public:
    SimpleStandardCluster()  {_hits.reserve(5);}

    void addPixel(SimpleStandardHit hit) {_hits.push_back(hit);}
    int getNPixel() const { return _hits.size(); }
    int getWidthX() const {
      if (_hits.size() == 1)
      {
        return 0;
      }
      int min;
      int max;
      int temp1;
      int temp2;
      int start;
      if (_hits.size() % 2)
      {
        min = _hits.at(0).getX();
        max = _hits.at(1).getX();
        start = 2;
      }
      else
      {
        min = _hits.at(0).getX();
        max = _hits.at(0).getX();
        start = 1;
      }

      for(std::vector<SimpleStandardHit>::const_iterator vec_it = _hits.begin()+start;
          vec_it < _hits.end(); )
      {
        temp1 = (*(vec_it++)).getX();
        temp2 = (*(vec_it++)).getX();

        if(temp1 >= temp2)
        {
          if (temp1 > max)
            max = temp1;
          else if (temp2 < min)
            min = temp2;
        }
        else
        {
          if (temp2 > max)
            max = temp2;
          else if (temp1 < min)
            min = temp1;
        }
      }
      return max-min;
    }
    int getWidthY() const {
      if (_hits.size() == 1)
      {
        return 0;
      }
      int min;
      int max;
      int temp1;
      int temp2;
      int start;
      if (_hits.size() % 2)
      {
        min = _hits.at(0).getY();
        max = _hits.at(1).getY();
        start = 2;
      }
      else
      {
        min = _hits.at(0).getY();
        max = _hits.at(0).getY();
        start = 1;
      }

      for(std::vector<SimpleStandardHit>::const_iterator vec_it = _hits.begin()+start;
          vec_it < _hits.end(); )
      {
        temp1 = (*(vec_it++)).getY();
        temp2 = (*(vec_it++)).getY();

        if(temp1 >= temp2)
        {
          if (temp1 > max)
            max = temp1;
          else if (temp2 < min)
            min = temp2;
        }
        else
        {
          if (temp2 > max)
            max = temp2;
          else if (temp1 < min)
            min = temp1;
        }
      }
      //std::cout << "YWidth: " << max-min << std::endl;
      return max - min;
    }
    int getX() const {
      if (_hits.size() == 1)
      {
        return _hits.at(0).getX();
      }
      int back = 0;
      for (std::vector<SimpleStandardHit>::const_iterator hit_it = _hits.begin();
          hit_it != _hits.end(); ++hit_it)
        back += (*hit_it).getX();
      return (int)back/_hits.size();
    }
    int getY()  const  {
      if (_hits.size() == 1)
      {
        return _hits.at(0).getY();
      }
      int back = 0;
      for (std::vector<SimpleStandardHit>::const_iterator hit_it = _hits.begin();
          hit_it != _hits.end(); ++hit_it)
        back += (*hit_it).getY();
      return (int)back/_hits.size();
    }
    int getTOT()  const {
      if (_hits.size() == 1)
      {
        return _hits.at(0).getTOT();
      }
      int back = 0;
      for (std::vector<SimpleStandardHit>::const_iterator hit_it = _hits.begin();
          hit_it != _hits.end(); ++hit_it)
        back += (*hit_it).getTOT();
      return back;
    }
    int getFirstLVL1() const {
      int beck = _hits.at(0).getLVL1();
      int temp;

      for(std::vector<SimpleStandardHit>::const_iterator vec_it = _hits.begin()+1;
          vec_it != _hits.end(); ++vec_it)
      {
        temp = (*vec_it).getLVL1();
        if (temp < beck)
          beck = temp;
      }
      //std::cout << "YWidth: " << max-min << std::endl;
      return beck;
    }
    int getLVL1Width() const {
      int first = -1;
      int last = -1;
      for (unsigned int i = 0; i < _hits.size(); i++) {
        int currentLVL1 = _hits.at(i).getLVL1();
        if (first == -1 && last == -1 ) first = currentLVL1; last = currentLVL1;
        if (first > currentLVL1) first = currentLVL1;
        if (last < currentLVL1) last = currentLVL1;
      }
      return last -first;
    }
};
#endif /* SIMPLESTANDARDCLUSTER_HH_ */
