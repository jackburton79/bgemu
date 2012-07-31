/*
 * Listener.h
 *
 *  Created on: 25/lug/2012
 *      Author: stefano
 */

#ifndef __LISTENER_H
#define __LISTENER_H

#include "IETypes.h"

class Listener {
public:
	Listener();
	virtual ~Listener();

	virtual void VideoAreaChanged(uint16 width, uint16 height);
};

#endif /* __LISTENER_H */
