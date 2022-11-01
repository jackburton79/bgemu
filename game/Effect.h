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

	int32 CountFrames() const;
	bool Finished() const;
	const ::Bitmap* Bitmap() const;
	void AdvanceFrame();
	IE::point Position() const;
private:
	VVCResource* fVVC;
	int32 fCurrentFrame;
	int32 fNumFrames;
	
	std::vector<::Bitmap*> fBitmaps;
	IE::point fWhere;

	void _LoadBitmaps();
	void _AdjustPosition();
};


#endif //__EFFECT_H

