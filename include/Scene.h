#pragma once
#include<SDLH.h>
#include"GUI/Button.h"

class Scene {
	public:
	virtual void Update(float deltaTime) = 0;
	virtual void Render(SDL_Renderer* Renderer) = 0;
	virtual void HandleEvents(SDL_Event& event) = 0;
	virtual void LoadResources(SDL_Renderer* renderer) {}
	virtual void Enter() = 0;
	virtual void Exit() = 0;
	virtual ~Scene() {}
};