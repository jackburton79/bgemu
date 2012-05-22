#ifndef __POLYGON_H
#define __POLYGON_H

#include "SDL.h"
#include "IETypes.h"


class Polygon {
public:
	Polygon();
	~Polygon();
	SDL_Rect Frame() const;
	void SetFrame(const int16 x_min, const int16 x_max,
			const int16 y_min, const int16 y_max);

	bool IsHole() const;
	void SetIsHole(const bool hole);

	bool AddPoints(point *points, int32 count);
	point *Points() const;
	point PointAt(int32 i) const;
	int32 CountPoints() const;


	void Print() const;

private:
	point *fPoints;
	int32 fCount;
	SDL_Rect fFrame;
	bool fIsHole;
};


#endif // __POLYGON_H
