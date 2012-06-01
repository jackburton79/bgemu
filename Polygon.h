#ifndef __POLYGON_H
#define __POLYGON_H

#include "Bitmap.h" // TODO: Move GFX::Rect away from here
#include "IETypes.h"


class Polygon {
public:
	Polygon();
	~Polygon();
	GFX::rect Frame() const;
	void SetFrame(const int16 x_min, const int16 x_max,
			const int16 y_min, const int16 y_max);

	bool IsHole() const;
	void SetIsHole(const bool hole);

	bool AddPoints(IE::point *points, int32 count);
	IE::point *Points() const;
	IE::point PointAt(int32 i) const;
	int32 CountPoints() const;


	void Print() const;

private:
	IE::point *fPoints;
	int32 fCount;
	GFX::rect fFrame;
	bool fIsHole;
};


#endif // __POLYGON_H
