#include"GUIUtils/Button.h"

void Button::SetCallback(std::function<void()> callback) {
    this->callback = std::move(callback);
}

void Button::SetBackground(Image background) {
    this->background = background;
}

void Button::HandleEvents(SDL_Event& event) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    
    bool isHovering = (mouseX >= drect.x && mouseX <= drect.x + drect.w && 
                       mouseY >= drect.y && mouseY <= drect.y + drect.h);
    
    if (event.type == SDL_MOUSEMOTION) {
        if (isHovering) {
            ButtonState = ButtonStates::Hover;
        } else {
            ButtonState = ButtonStates::Normal;
        }
    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (isHovering) {
            ButtonState = ButtonStates::Pressed;
        }
    } else if (event.type == SDL_MOUSEBUTTONUP) {
        if (ButtonState == ButtonStates::Pressed && isHovering) {
            if (callback) {
                callback();
            }
        }
        ButtonState = isHovering ? ButtonStates::Hover : ButtonStates::Normal;
    }
}

void Button::Update(const float deltaTime) {
    // 这里可以添加按钮的动画效果
}

void Button::Render(SDL_Renderer* render) {
    // 渲染按钮背景
    background.Render(render, &drect);
    
    // 根据按钮状态渲染不同的效果
    switch (ButtonState) {
        case ButtonStates::Normal:
            // 渲染正常状态
            break;
        case ButtonStates::Hover:
            // 渲染悬停状态
            SDL_SetRenderDrawColor(render, 255, 255, 255, 100);
            SDL_RenderFillRect(render, &drect);
            break;
        case ButtonStates::Pressed:
            // 渲染按下状态
            SDL_SetRenderDrawColor(render, 200, 200, 200, 100);
            SDL_RenderFillRect(render, &drect);
            break;
    }
}