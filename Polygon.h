#ifndef __POLYGON_H
#define __POLYGON_H

#include "Bitmap.h" // TODO: Move GFX::Rect away from here
#include "IETypes.h"


class Polygon {
public:
	Polygon();
	Polygon(const Polygon& polygon);
	~Polygon();

	GFX::rect Frame() const;
	void SetFrame(const int16 x_min, const int16 x_max,
			const int16 y_min, const int16 y_max);

	uint8 Flags() const;
	void SetFlags(const uint8 flags);

	bool AddPoints(IE::point *points, int32 count);
	IE::point *Points() const;
	IE::point PointAt(int32 i) const;
	int32 CountPoints() const;

	void OffsetBy(int32 x, int32 y);

	bool Contains(const IE::point& point) const;

	void Print() const;

	Polygon& operator=(const Polygon&);

private:
	IE::point *fPoints;
	int32 fCount;
	GFX::rect fFrame;
	uint8 fFlags;
};


#endif // __POLYGON_H
