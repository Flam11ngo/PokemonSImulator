#pragma once
#include"SDLH.h"
#include"Image.h"
#include <memory>

class Core{
private:
    static SDL_Window* window;
    static std::shared_ptr<SDL_Renderer> renderer;

public:
    static void init();
    static void quit();
    static void mainLoop();
    static std::shared_ptr<SDL_Renderer> getRenderer();
};
