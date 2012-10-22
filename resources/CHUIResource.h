/*
 * CHUIResource.h
 *
 *  Created on: 17/ott/2012
 *      Author: stefano
 */

#ifndef __CHUIRESOURCE_H_
#define __CHUIRESOURCE_H_

#include "Resource.h"

struct control;
struct control_table;
class Bitmap;
class CHUIResource;
class Window;
class CHUIResource: public Resource {
public:
	CHUIResource(const res_ref &name);
	virtual ~CHUIResource();

	uint32 CountWindows() const;
	Window* GetWindow(uint32 num);

	void Dump();
private:
	virtual bool Load(Archive *archive, uint32 key);

	IE::control* _ReadControl(control_table& table);
	uint32 fNumWindows;
	uint32 fControlTableOffset;
	uint32 fWindowsOffset;
};

#endif /* __CHUIRESOURCE_H_ */
