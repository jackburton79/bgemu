/*
 * Timer.h
 *
 *  Created on: 11/ago/2012
 *      Author: stefano
 */

#ifndef __TIMER_H
#define __TIMER_H

#include "SupportDefs.h"


class Timer {
public:
	Timer();
	~Timer();

	void SetExpiration(uint32 timer);
	bool Expired() const;

private:
	uint32 fExpiration;
};

#endif /* __TIMER_H */
