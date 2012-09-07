/*
 * EventSanityChecker.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef EVENTSANITY_H_
#define EVENTSANITY_H_

class EventSanityChecker
{
public:

	EventSanityChecker();
	EventSanityChecker(int nplanes);
	virtual ~EventSanityChecker();
    unsigned int getNPlanes() const;
    void setNPlanes(int NPlanes);
private:
	unsigned int NPlanes;

};

#endif /* EVENTSANITY_H_ */
