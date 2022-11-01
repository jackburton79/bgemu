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

	virtual void VideoModeChanged(uint16 newWidth, uint16 newHeight, uint16 newDepth);
};

#endif /* __LISTENER_H */
