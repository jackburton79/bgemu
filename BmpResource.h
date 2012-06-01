#ifndef __BMPRESOURCE_H
#define __BMPRESOURCE_H

#include "Resource.h"


class Bitmap;
class BMPResource : public Resource {
public:
	BMPResource(uint8 *data, uint32 size, uint32 key);
	BMPResource(const res_ref &name);

	~BMPResource();

	bool Load(Archive *archive, uint32 key);

	Bitmap *Image();

private:
	void _Init();

};

#endif // __BMPRESOURCE_H
