#include"Core.h"
#include "Image.h"

SDL_Window* Core::window = nullptr;
std::shared_ptr<SDL_Renderer> Core::renderer = nullptr;

void Core::init() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return;
    }

    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
    }

    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
    }

    if ((Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3) & (MIX_INIT_OGG | MIX_INIT_MP3)) == 0) {
        SDL_Log("Mix_Init failed: %s", Mix_GetError());
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio failed: %s", Mix_GetError());
    }

    window = SDL_CreateWindow("Pokemon", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return;
    }

    SDL_Renderer* rawRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!rawRenderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return;
    }

    renderer = std::shared_ptr<SDL_Renderer>(rawRenderer, [](SDL_Renderer* r) {
        if (r) {
            SDL_DestroyRenderer(r);
        }
    });

    Image::ImgInit(renderer.get());
}

void Core::quit() {
    if (renderer) {
        renderer.reset();
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

void Core::mainLoop() {
    bool quit = false;
    SDL_Event e;
    SDL_Renderer* rawRenderer = renderer.get();
    if (!rawRenderer) {
        SDL_Log("Renderer is not initialized.");
        return;
    }

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(rawRenderer, 0, 0, 0, 255);
        SDL_RenderClear(rawRenderer);
        SDL_RenderPresent(rawRenderer);
    }
}

std::shared_ptr<SDL_Renderer> Core::getRenderer() {
    return renderer;
}
