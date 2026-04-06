#include "GUIUtils/HealthBar.h"
#include <cmath>

HealthBar::HealthBar() : currentHealth(0), maxHealth(100), displayHealth(0), smoothSpeed(50.0f) {
    backgroundRect = {0, 0, 100, 10};
    fillRect = {0, 0, 100, 10};
    backgroundColor = {100, 100, 100, 255}; // 灰色背景
    fillColor = {0, 255, 0, 255}; // 绿色血条
}

HealthBar::HealthBar(SDL_Rect bgRect, float maxHP, float smoothSpeed) : 
    backgroundRect(bgRect), 
    currentHealth(maxHP), 
    maxHealth(maxHP), 
    displayHealth(maxHP), 
    smoothSpeed(smoothSpeed) {
    fillRect = bgRect;
    backgroundColor = {100, 100, 100, 255}; // 灰色背景
    fillColor = {0, 255, 0, 255}; // 绿色血条
}

void HealthBar::setPosition(int x, int y) {
    backgroundRect.x = x;
    backgroundRect.y = y;
    fillRect.x = x;
    fillRect.y = y;
}

void HealthBar::setSize(int width, int height) {
    backgroundRect.w = width;
    backgroundRect.h = height;
    fillRect.h = height;
    updateFillRect();
}

void HealthBar::setMaxHealth(float maxHP) {
    maxHealth = maxHP;
    if (currentHealth > maxHealth) {
        currentHealth = maxHealth;
        displayHealth = maxHealth;
    }
    updateFillRect();
}

void HealthBar::setCurrentHealth(float currentHP) {
    currentHealth = currentHP;
    if (currentHealth < 0) {
        currentHealth = 0;
    } else if (currentHealth > maxHealth) {
        currentHealth = maxHealth;
    }
}

void HealthBar::setCurrentHealthImmediately(float currentHP) {
    currentHealth = currentHP;
    if (currentHealth < 0) {
        currentHealth = 0;
    } else if (currentHealth > maxHealth) {
        currentHealth = maxHealth;
    }
    displayHealth = currentHealth; // 直接设置显示值，跳过动画效果
    updateFillRect();
}

void HealthBar::update(float deltaTime) {
    if (displayHealth != currentHealth) {
        float healthDiff = currentHealth - displayHealth;
        float healthChange = smoothSpeed * deltaTime;
        if (fabs(healthDiff) < healthChange) {
            displayHealth = currentHealth;
        } else {
            displayHealth += healthDiff > 0 ? healthChange : -healthChange;
        }
        updateFillRect();
    }
}

void HealthBar::render(SDL_Renderer* renderer) {
    // 渲染背景
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &backgroundRect);
    
    // 渲染血条
    SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    SDL_RenderFillRect(renderer, &fillRect);
}

float HealthBar::getCurrentHealth() const {
    return currentHealth;
}

float HealthBar::getMaxHealth() const {
    return maxHealth;
}

// 辅助方法：更新血条填充矩形
void HealthBar::updateFillRect() {
    float healthRatio = displayHealth / maxHealth;
    fillRect.w = static_cast<int>(backgroundRect.w * healthRatio);
}