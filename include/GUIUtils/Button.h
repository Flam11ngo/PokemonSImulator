#pragma once
#include"SDLH.h"
#include"Image.h"
#include<functional>
#include<string>
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
	std::string text;
	TTF_Font* font;
	SDL_Color textColor;
	bool visible = true; // 按钮是否可见
public:
	Button(){};
	Button(SDL_Rect rect, std::function<void()> callback, Image background) :drect(rect), background(background) { this->callback = std::move(callback); }
    Button(SDL_Rect rect, std::function<void()> callback, Image&& background) :drect(rect),background(std::move(background)) { this->callback = std::move(callback);}
	Button(SDL_Rect rect, std::function<void()> callback, Image background, std::string text, TTF_Font* font, SDL_Color textColor) : 
		drect(rect), background(background), text(text), font(font), textColor(textColor) { this->callback = std::move(callback); }
	void SetCallback(std::function<void()> callback);
	void SetBackground(Image background);
	void SetText(std::string text);
	void SetFont(TTF_Font* font);
	void SetTextColor(SDL_Color color);
	void setVisible(bool visible);
	bool isVisible() const;
	void HandleEvents(SDL_Event& event);
	void Update(const float deltaTime);
	void Render(SDL_Renderer* render);
	const SDL_Rect* getDrect() const { return &drect; }
};