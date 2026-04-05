#include"Core.h"
#include "Image.h"
#include "SceneManager.h"

SDL_Window* Core::window = nullptr;
SDL_Renderer* Core::renderer = nullptr;

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
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    Image::ImgInit(renderer);
    
    // 初始化场景管理器
    SceneManager::Init();
}

void Core::quit() {
    // 清理场景管理器
    SceneManager::Clean();
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
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

void Core::getWindowSize(int& width, int& height) {
    if (window) {
        SDL_GetWindowSize(window, &width, &height);
    } else {
        width = 1280;
        height = 720;
    }
}

void Core::mainLoop() {
    bool quit = false;
    SDL_Event e;
    Uint32 lastTime = SDL_GetTicks();

    while (!quit) {
        // 计算deltaTime
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        // 处理事件
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else {
                SceneManager::HandleEvents(e);
            }
        }
        
        // 更新场景
        SceneManager::Update(deltaTime);
        
        // 渲染场景
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SceneManager::Render(renderer);
        SDL_RenderPresent(renderer);
        
        // 延迟
        SDL_Delay(16); // 约60 FPS
    }
}
