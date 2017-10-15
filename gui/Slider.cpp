/*
 * Slider.cpp
 *
 *  Created on: 20/ott/2012
 *      Author: stefano
 */

#include "BamResource.h"
#include "GraphicsEngine.h"
#include "MOSResource.h"
#include "ResManager.h"
#include "Slider.h"
#include "Window.h"


Slider::Slider(IE::slider* slider)
	:
	Control(slider),
	fBackground(NULL),
	fKnobImage(NULL),
	fKnobPosition(0)
{
	std::cout << "Slider " << slider->id << std::endl;
	MOSResource* mos = gResManager->GetMOS(slider->background);
	fBackground = mos->Image();
	gResManager->ReleaseResource(mos);
	fKnobImage = gResManager->GetBAM(slider->knob);
}


Slider::~Slider()
{
	if (fBackground != NULL)
		fBackground->Release();
	gResManager->ReleaseResource(fKnobImage);
}


/* virtual */
void
Slider::Draw()
{
	GFX::rect destRect(fControl->x, fControl->y,
			fControl->w, fControl->h);
	Window()->ConvertToScreen(destRect);
	GraphicsEngine::Get()->BlitToScreen(fBackground, NULL, &destRect);
}


/* virtual */
void
Slider::MouseDown(IE::point point)
{
}


/* virtual */
void
Slider::MouseUp(IE::point point)
{
}


/* virtual */
void
Slider::MouseMoved(IE::point point, uint32 transit)
{
}
