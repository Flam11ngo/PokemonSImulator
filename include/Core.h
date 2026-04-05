#pragma once
#include"SDLH.h"
#include"Image.h"

class Core{
private:
    static SDL_Window* window;
    static SDL_Renderer* renderer;

public:
    static void init();
    static void quit();
    static void mainLoop();
    static SDL_Renderer* getRenderer() { return renderer; }
    static void getWindowSize(int& width, int& height);
};
