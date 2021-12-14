#include "AnimationTester.h"
#include "BamResource.h"
#include "GraphicsEngine.h"
#include "GUI.h"
#include "Label.h"
#include "ResManager.h"
#include "Timer.h"
#include "Window.h"

#include <sstream>

class AnimationTesterWindow : public Window {
public:
	AnimationTesterWindow();
};


AnimationTester::AnimationTester(std::string name)
	:
	fResource(NULL),
	fCycleNum(0),
	fFrameNum(0)
{
	fResource = dynamic_cast<BAMResource*>(gResManager->GetResource(name.c_str()));
}


void
AnimationTester::SelectCycle(uint8 cycle)
{
	if (cycle >= fResource->CountCycles())
		return;
	
	Bitmap* bitmap = fResource->FrameForCycle(fCycleNum, fFrameNum);
	if (bitmap != NULL) {
		GFX::rect screenFrame = GraphicsEngine::Get()->ScreenFrame();
		GFX::rect frame = bitmap->Frame();
		fPosition.x = (screenFrame.w - frame.w) / 2;
		fPosition.y = (screenFrame.h - frame.h) / 2;
		bitmap->Release();
	}
	fCycleNum = cycle;
	fFrameNum = 0;
}


void
AnimationTester::UpdateAnimation()
{
	if (fFrameNum++ >= fResource->CountFrames(fCycleNum))
		fFrameNum = 0;
	Bitmap* bitmap = fResource->FrameForCycle(fCycleNum, fFrameNum);
	if (bitmap != NULL) {
		GFX::rect bitmapFrame = bitmap->Frame();
		bitmapFrame.x = fPosition.x;
		bitmapFrame.y = fPosition.y;
		GFX::rect animationFrame(fPosition.x, fPosition.y, 100, 100);
		GraphicsEngine::Get()->ScreenBitmap()->FillRect(animationFrame, 0);
		GraphicsEngine::Get()->BlitToScreen(bitmap, NULL, &bitmapFrame);
		bitmap->Release();
	}
	Window* window = GUI::Get()->GetWindow(999);
	if (window != NULL) {
		Label* label = (Label*)window->GetControlByID(1);
		if (label != NULL) {
			std::ostringstream stringStream;
			stringStream << "frame " << fFrameNum;
			label->SetText(stringStream.str().c_str());
		}
	}
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
	//gui->AddWindow(new AnimationTesterWindow());
	//gui->ShowWindow(999);
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
		
		Timer::WaitSync(startTicks, 100);
	}
}


AnimationTesterWindow::AnimationTesterWindow()
	:
	Window(999, 0, 0, GraphicsEngine::Get()->ScreenFrame().w, GraphicsEngine::Get()->ScreenFrame().h, NULL)
{
	IE::label* control = new IE::label;
	control->id = 1;
	control->x = 30;
	control->y = 400;
	control->w = 300;
	control->h = 15;
	control->type = IE::CONTROL_LABEL;
	control->font_bam = "TOOLFONT";
	control->flags = IE::LABEL_USE_RGB_COLORS;
	control->color1_r = control->color1_g = control->color1_b = control->color1_a = 0;
	control->color2_r = control->color2_g = control->color2_b = control->color2_a = 255;
	
	Label* frameLabel = new Label(control);
	frameLabel->SetText("");
	Add(frameLabel);
}
