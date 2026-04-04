#pragma once
#include"SDLH.h"
#include<string>
class Image {
private:
	SDL_Surface* surface=nullptr;
	SDL_Texture* texture= nullptr;
	SDL_Rect srcRect{0,0,0,0};
	SDL_Rect destRect{0,0,0,0};
public:
	static SDL_Renderer* rendererStatic;
	static void ImgInit(SDL_Renderer* renderer);
	void LoadImage(SDL_Renderer* renderer, std::string filepath);
	std::string filepath="";
	Image() {}
	Image(SDL_Renderer* renderer, std::string filepath);
	Image(const Image& other);
	Image& operator=(const Image& other);
	Image(Image&& other) noexcept;
	void CleanUp();
	void Render(SDL_Renderer* renderer,SDL_Rect*destRect);
	SDL_Texture* getTexture() { return texture; }
	SDL_Surface* getSurface() { return surface; }
	const SDL_Rect* getSrcRect() const { return &srcRect; }
	const SDL_Rect* getDestRect() const { return &destRect; }
};