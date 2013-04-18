/*
 * BaseCollection.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include "BaseCollection.hh"

BaseCollection::BaseCollection()
  : _reduce(1),
  _mon(NULL),
  CollectionType(UNKNOWN_COLLECTION_TYPE){

  }

void BaseCollection::setReduce(const unsigned int red){
  _reduce = red;
}

unsigned int BaseCollection::getCollectionType(){
  return CollectionType;
}
