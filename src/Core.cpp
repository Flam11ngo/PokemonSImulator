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
    // 禁止窗口缩放
    SDL_SetWindowResizable(window, SDL_FALSE);
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
    Uint32 frameCount = 0;
    Uint32 lastFPSUpdate = SDL_GetTicks();
    float fps = 0.0f;

    while (!quit) {
        // 计算deltaTime
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        // 计算帧率
        frameCount++;
        if (currentTime - lastFPSUpdate >= 1000) { // 每秒更新一次
            fps = frameCount / ((currentTime - lastFPSUpdate) / 1000.0f);
            lastFPSUpdate = currentTime;
            frameCount = 0;
        }
        
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
        
        // 渲染帧率
        if (renderer) {
            // 创建帧率文本
            std::string fpsText = "FPS: " + std::to_string(static_cast<int>(fps));
            
            // 加载字体
            TTF_Font* font = TTF_OpenFont("../assets/fonts/arial.ttf", 16);
            if (font) {
                SDL_Color textColor = {255, 255, 255, 255};
                SDL_Surface* textSurface = TTF_RenderText_Solid(font, fpsText.c_str(), textColor);
                if (textSurface) {
                    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
                    if (textTexture) {
                        // 计算文本位置（右上角）
                        int windowWidth, windowHeight;
                        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
                        SDL_Rect textRect = {windowWidth - textSurface->w - 10, 10, textSurface->w, textSurface->h};
                        
                        // 渲染文本
                        SDL_RenderCopy(renderer, textTexture, nullptr, &textRect);
                        
                        // 清理资源
                        SDL_DestroyTexture(textTexture);
                    }
                    SDL_FreeSurface(textSurface);
                }
                TTF_CloseFont(font);
            }
        }
        
        SDL_RenderPresent(renderer);
        
        // 移除固定延迟，提高事件响应速度
        // SDL_Delay(16); // 约60 FPS
    }
}
