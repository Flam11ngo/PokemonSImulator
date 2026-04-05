#include"GUIUtils/Button.h"

void Button::SetCallback(std::function<void()> callback) {
    this->callback = std::move(callback);
}

void Button::SetBackground(Image background) {
    this->background = background;
}

void Button::SetText(std::string text) {
    this->text = text;
}

void Button::SetFont(TTF_Font* font) {
    this->font = font;
}

void Button::SetTextColor(SDL_Color color) {
    this->textColor = color;
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
            // 渲染悬停状态 - 加厚白色边框
            SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
            for (int i = 0; i < 3; i++) {
                SDL_Rect borderRect = {drect.x - i, drect.y - i, drect.w + 2 * i, drect.h + 2 * i};
                SDL_RenderDrawRect(render, &borderRect);
            }
            break;
        case ButtonStates::Pressed:
            // 渲染按下状态 - 加厚白色边框
            SDL_SetRenderDrawColor(render, 255, 255, 255, 255);
            for (int i = 0; i < 3; i++) {
                SDL_Rect borderRect = {drect.x - i, drect.y - i, drect.w + 2 * i, drect.h + 2 * i};
                SDL_RenderDrawRect(render, &borderRect);
            }
            break;
    }
    
    // 渲染按钮文本
    if (!text.empty() && font) {
        SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(render, surface);
            if (texture) {
                int width, height;
                SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
                
                // 计算文本居中的位置
                SDL_Rect destRect = drect;
                destRect.w = width;
                destRect.h = height;
                destRect.x += (drect.w - width) / 2;
                destRect.y += (drect.h - height) / 2;
                
                SDL_RenderCopy(render, texture, nullptr, &destRect);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }
}