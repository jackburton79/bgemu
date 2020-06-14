/*
 * CHUIResource.h
 *
 *  Created on: 17/ott/2012
 *      Author: stefano
 */

#ifndef __CHUIRESOURCE_H_
#define __CHUIRESOURCE_H_

#include "Resource.h"

namespace IE {
	struct control;
};
struct control_table;
class Bitmap;
class CHUIResource;
class Window;
class CHUIResource: public Resource {
public:
	CHUIResource(const res_ref &name);
	static Resource* Create(const res_ref& name);

	uint16 CountWindows() const;
	Window* GetWindow(uint16 id);

	void Dump();
private:
	virtual ~CHUIResource();
	virtual bool Load(Archive *archive, uint32 key);

	IE::control* _ReadControl(control_table& table);
	uint32 fNumWindows;
	uint32 fControlTableOffset;
	uint32 fWindowsOffset;
};

#endif /* __CHUIRESOURCE_H_ */
