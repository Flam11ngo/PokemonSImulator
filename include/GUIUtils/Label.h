#pragma once
#include"SDLH.h"
#include<string>
class Label {
private:
	SDL_Rect drect = {0,0,0,0};
	std::string text = "";
	SDL_Color color = {255,255,255,255};
	TTF_Font* font = nullptr;
public:
	Label(){};
	Label(SDL_Rect rect, std::string text, SDL_Color color, TTF_Font* font) :drect(rect), text(text), color(color), font(font) {}
	void SetText(std::string text);
	void SetColor(SDL_Color color);
	void SetFont(TTF_Font* font);
	void Render(SDL_Renderer* render);
	const SDL_Rect* getDrect() const { return &drect; }
};