/*
 * SimpleStandardHit.hh
 *
 *  Created on: Jun 9, 2011
 *      Author: stanitz
 */

#ifndef SIMPLESTANDARDHIT_HH_
#define SIMPLESTANDARDHIT_HH_


class SimpleStandardHit {
  protected:
    int _x;
    int _y;
    int _tot;
    int _lvl1;
    int _reduceX;
    int _reduceY;


  public:
    SimpleStandardHit(const int x, const int y) : _x(x), _y(y), _tot(-1), _lvl1(-1), _reduceX(-1), _reduceY(-1) {}
    SimpleStandardHit(const int x, const int y, const int tot, const int lvl1) : _x(x), _y(y), _tot(tot), _lvl1(lvl1), _reduceX(-1), _reduceY(-1) {}
    SimpleStandardHit(const int x, const int y, const int tot, const int lvl1, const int reduceX, const int reduceY) : _x(x), _y(y), _tot(tot), _lvl1(lvl1), _reduceX(reduceX), _reduceY(-reduceY){}

    int getX() const { return _x;}
    int getY() const { return _y;}

    int getTOT() const {return _tot; }
    int getLVL1() const {return _lvl1; }
    void setLVL1(const int lvl1) {_lvl1 = lvl1; }
    void setTOT(const int tot) {_tot = tot; }
    //void reduce(const int reduceX, const int reduceY) {_reduceX = reduceX; _reduceY = reduceY;}
    // does this pixel use analog information ?
};


#endif /* SIMPLESTANDARDHIT_HH_ */
