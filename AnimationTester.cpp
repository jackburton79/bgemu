#include "AnimationTester.h"
#include "BamResource.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "ResManager.h"
#include "Timer.h"

AnimationTester::AnimationTester(std::string name)
	:
	fResource(NULL),
	fCycleNum(0),
	fFrameNum(0)
{
	fResource = gResManager->GetBAM(name.c_str());
}


void
AnimationTester::SelectCycle(uint8 cycle)
{
	if (cycle >= fResource->CountCycles())
		return;
	
	Bitmap* bitmap = fResource->FrameForCycle(fCycleNum, fFrameNum);

	GFX::rect screenFrame = GraphicsEngine::Get()->ScreenFrame();	
	GFX::rect frame = bitmap->Frame();
	fPosition.x = (screenFrame.w - frame.w) / 2;
	fPosition.y = (screenFrame.h - frame.h) / 2;
	
	bitmap->Release();
	fCycleNum = cycle;
	fFrameNum = 0;
}


void
AnimationTester::UpdateAnimation()
{
	if (fFrameNum++ >= fResource->CountFrames(fCycleNum))
		fFrameNum = 0;
	Bitmap* bitmap = fResource->FrameForCycle(fCycleNum, fFrameNum);
	GFX::rect bitmapFrame = bitmap->Frame();
	bitmapFrame.x = fPosition.x;
	bitmapFrame.y = fPosition.y;
	GraphicsEngine::Get()->BlitToScreen(bitmap, NULL, &bitmapFrame);
	bitmap->Release();
}


void
AnimationTester::Loop()
{
	if (!GUI::Initialize(GraphicsEngine::Get()->ScreenFrame().w,
						 GraphicsEngine::Get()->ScreenFrame().h)) {
		throw std::runtime_error("Initializing GUI failed");
	}
	SelectCycle(0);
	SDL_Event event;
	bool quitting = false;
	GUI* gui = GUI::Get();
	while (!quitting) {
		uint32 startTicks = Timer::Ticks();
		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					gui->MouseDown(event.button.x, event.button.y);
					break;
				case SDL_MOUSEBUTTONUP:
					gui->MouseUp(event.button.x, event.button.y);
					break;
				case SDL_MOUSEMOTION:
					gui->MouseMoved(event.motion.x, event.motion.y);
					break;
				case SDL_QUIT:
					quitting = true;
					break;
				default:
					break;
			}
		}

		gui->Draw();

		UpdateAnimation();
		GraphicsEngine::Get()->Update();
		
		Timer::WaitSync(startTicks, 35);
	}
}
