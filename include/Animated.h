#pragma once
#include"SDLH.h"
#include<string>
#include<vector>

class Animated {
private:
    std::vector<SDL_Texture*> frames;
    std::vector<int> frameDelays; // 每帧的延迟时间（毫秒）
    int currentFrame; 
    Uint32 lastFrameTime;
    float elapsedTime; // 累积时间（毫秒）
    bool isPlaying;
    bool isLooping;
    SDL_Rect srcRect;
    SDL_Rect destRect;
    std::string filepath;
    bool isGif; // 标记是否为GIF文件
    
    // 加载方法
    bool loadFromDirectory(SDL_Renderer* renderer, const std::string& dirPath);
    bool loadFromGif(SDL_Renderer* renderer, const std::string& gifPath);

public:
    static SDL_Renderer* rendererStatic;
    static void AnimatedInit(SDL_Renderer* renderer);
    
    Animated();
    Animated(SDL_Renderer* renderer, std::string filepath, bool looping = true);
    ~Animated();
    
    // 加载动画
    bool LoadAnimation(SDL_Renderer* renderer, std::string filepath, bool looping = true);
    
    // 控制方法
    void Play();
    void Pause();
    void Stop();
    void SetLooping(bool looping);
    
    // 更新和渲染
    void Update(float deltaTime);
    void Render(SDL_Renderer* renderer, SDL_Rect* destRect = nullptr);
    
    // 获取信息
    int GetFrameCount() const;
    int GetCurrentFrameIndex() const;
    bool IsPlaying() const;
    bool IsLooping() const;
    
    // 清理资源
    void CleanUp();
    
    // 设置位置和大小
    void setPosition(int x, int y);
    void setSize(int w, int h);
};