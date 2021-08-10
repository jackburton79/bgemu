#ifndef __BMPRESOURCE_H
#define __BMPRESOURCE_H

#include "Resource.h"


class Bitmap;
class BMPResource : public Resource {
public:
	BMPResource(uint8 *data, uint32 size, uint32 key);
	BMPResource(const res_ref &name);

	virtual bool Load(Archive *archive, uint32 key);

	Bitmap *Image();

	static Resource* Create(const res_ref& name);

private:
	virtual ~BMPResource();
	void _Init();
};

#endif // __BMPRESOURCE_H
