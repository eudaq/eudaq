/*
 * BaseCollection.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef BASECOLLECTION_HH_
#define BASECOLLECTION_HH_
//ROOT includes
#include <TH2I.h>
#include <TFile.h>
//Project includes
#include "SimpleStandardEvent.hh"

class RootMonitor;

// types of Collections
const unsigned int HITMAP_COLLECTION_TYPE = 1;
const unsigned int CORRELATION_COLLECTION_TYPE = 2;
const unsigned int MONITORPERFORMANCE_COLLECTION_TYPE = 3;
const unsigned int EUDAQMONITOR_COLLECTION_TYPE = 4;
const unsigned int UNKNOWN_COLLECTION_TYPE = 9999;

class BaseCollection
{
protected:
	unsigned int _reduce;
	RootMonitor *_mon;
	unsigned int CollectionType; // used to specify what kind of Collection we are having
public:
		BaseCollection() : _reduce(1), _mon(NULL), CollectionType(UNKNOWN_COLLECTION_TYPE) {};
		virtual void Write(TFile *file) = 0;
		virtual void Calculate(const unsigned int currentEventNumber) = 0;
		virtual void Fill(const SimpleStandardEvent& simpev) = 0;
		virtual void Reset() = 0;
		void setReduce(const unsigned int red);
		unsigned int getCollectionType(){return CollectionType;};
};

#ifdef __CINT__
#pragma link C++ class BaseCollection-;
#endif
#endif /* BASECOLLECTION_HH_ */
