/*
 * CHUIResource.h
 *
 *  Created on: 17/ott/2012
 *      Author: stefano
 */

#ifndef __CHUIRESOURCE_H_
#define __CHUIRESOURCE_H_

#include "Resource.h"

class Bitmap;
class CHUIResource;
class Window {
public:
	Window();
	Bitmap* Background();

private:
	friend class CHUIResource;
	Bitmap* fBackground;
};


class CHUIResource: public Resource {
public:
	CHUIResource(const res_ref &name);
	virtual ~CHUIResource();

	uint32 CountWindows() const;
	Window* GetWindow(uint32 num);

	void Dump();
private:
	virtual bool Load(Archive *archive, uint32 key);

	uint32 fNumWindows;
	uint32 fControlTableOffset;
	uint32 fWindowsOffset;
};

#endif /* __CHUIRESOURCE_H_ */
