#pragma once
#include<SDLH.h>
#include"GUIUtils/Button.h"

class Scene {
	protected:
	SDL_Rect windowRect;

	public:
	virtual void Update(float deltaTime) = 0;
	virtual void Render(SDL_Renderer* Renderer) = 0;
	virtual void HandleEvents(SDL_Event& event) = 0;
	virtual void LoadResources(SDL_Renderer* renderer) {}
	virtual void Enter() = 0;
	virtual void Exit() = 0;
	virtual void SetWindowSize(int width, int height) { windowRect = {0, 0, width, height}; }
	virtual ~Scene() {}
};