#include "MoviePlayer.h"

#include "GraphicsEngine.h"
#include "MveResource.h"
#include "SoundEngine.h"
#include "Timer.h"

#include <iostream>

#include <SDL.h>


void
MoviePlayer::Play(MVEResource* resource)
{
	GraphicsEngine::Get()->SaveCurrentMode();

	bool quitting = false;
	bool paused = false;
	SDL_Event event;
	while (!quitting) {
		if (!paused) {
			if (!resource->DecodeNextChunk())
				break;
		}

		while (SDL_PollEvent(&event) != 0) {
			switch (event.type) {
				case SDL_KEYDOWN: {
					switch (event.key.keysym.sym) {
						// Any of these skips the movie.
						case SDLK_q:
						case SDLK_ESCAPE:
						case SDLK_SPACE:
						case SDLK_RETURN:
							quitting = true;
							break;
						case SDLK_p:
							paused = !paused;
							break;
						default:
							break;
					}
				}
				break;

				case SDL_MOUSEBUTTONDOWN:
					quitting = true;
					break;

				case SDL_QUIT:
					// Ends the movie and stays in the queue for the game loop.
					SDL_PushEvent(&event);
					quitting = true;
					break;
				default:
					break;
			}
		}

		if (resource->ConsumeFrameReady()) {
			GraphicsEngine::Get()->BlitToScreen(resource->CurrentFrame(), NULL, NULL);
			GraphicsEngine::Get()->Update();
		}

		uint32 frameDelay = resource->FrameDelay();
		if (frameDelay != 0 && !quitting) {
			uint32 currentTime = Timer::Ticks();
			uint32 nextFrameTime = resource->LastFrameTime() + frameDelay;
			if (currentTime < nextFrameTime)
				Timer::Wait(nextFrameTime - currentTime);
		}
	}

	if (SoundEngine::Get() != NULL)
		SoundEngine::Get()->DestroyBuffers();
	std::cout << "MoviePlayer::Play() returns..." << std::endl;

	GraphicsEngine::Get()->RestorePreviousMode();
}
