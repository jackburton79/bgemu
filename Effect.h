#ifndef __EFFECT_H
#define __EFFECT_H

#include "IETypes.h"

#include <vector>

class Bitmap;
class VVCResource;
class Effect {
public:
	Effect(const res_ref& name, IE::point where);
	~Effect();

	uint32 CountFrames() const;
	bool Finished() const;
	const ::Bitmap* NextBitmap();

	IE::point Position() const;
private:
	VVCResource* fVVC;
	uint32 fCurrentFrame;
	uint32 fNumFrames;
	
	std::vector<Bitmap*> fBitmaps;
	IE::point fWhere;

	void _LoadBitmaps();
};


#endif //__EFFECT_H

