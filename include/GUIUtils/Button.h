#pragma once
#include"SDLH.h"
#include"Image.h"
#include<functional>
class Button {
private:
	enum ButtonStates {
		Normal,
		Hover,
		Pressed
	};
	SDL_Rect drect = {0,0,0,0};
	ButtonStates ButtonState = ButtonStates::Normal;
	std::function<void()> callback = nullptr;
	Image background;
public:
	Button(){};
	Button(SDL_Rect rect, std::function<void()> callback, Image background) :drect(rect), background(background) { this->callback = std::move(callback); }
    Button(SDL_Rect rect, std::function<void()> callback, Image&& background) :drect(rect),background(std::move(background)) { this->callback = std::move(callback);}
	void SetCallback(std::function<void()> callback);
	void SetBackground(Image background);
	void HandleEvents(SDL_Event& event);
	void Update(const float deltaTime);
	void Render(SDL_Renderer* render);
	const SDL_Rect* getDrect() const { return &drect; }
};