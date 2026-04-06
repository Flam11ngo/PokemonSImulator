#pragma once
#include "SDLH.h"

class HealthBar {
private:
    SDL_Rect backgroundRect;
    SDL_Rect fillRect;
    float currentHealth;
    float maxHealth;
    float displayHealth;
    float smoothSpeed;
    SDL_Color backgroundColor;
    SDL_Color fillColor;

public:
    HealthBar();
    HealthBar(SDL_Rect bgRect, float maxHP, float smoothSpeed = 50.0f);
    
    void setPosition(int x, int y);
    void setSize(int width, int height);
    void setMaxHealth(float maxHP);
    void setCurrentHealth(float currentHP);
    void setCurrentHealthImmediately(float currentHP); // 直接设置当前生命值，跳过动画效果
    void update(float deltaTime);
    void render(SDL_Renderer* renderer);
    
    float getCurrentHealth() const;
    float getMaxHealth() const;
    
private:
    // 辅助方法：更新血条填充矩形
    void updateFillRect();
};