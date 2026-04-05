#include"GUIUtils/Button.h"
#include <sstream>

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

void Button::setVisible(bool visible) {
    this->visible = visible;
}

bool Button::isVisible() const {
    return visible;
}

void Button::HandleEvents(SDL_Event& event) {
    if (!visible) return;
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
    if (!visible) return;
    
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
        // 支持多行文本，按换行符分割
        std::string line;
        std::istringstream stream(text);
        int lineHeight = 20; // 每行文本的高度
        int startY = drect.y + 10; // 文本起始Y坐标
        
        while (std::getline(stream, line)) {
            SDL_Surface* surface = TTF_RenderText_Solid(font, line.c_str(), textColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(render, surface);
                if (texture) {
                    int width, height;
                    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
                    
                    // 计算文本居中的位置
                    SDL_Rect destRect = {
                        drect.x + (drect.w - width) / 2,
                        startY,
                        width,
                        height
                    };
                    
                    SDL_RenderCopy(render, texture, nullptr, &destRect);
                    SDL_DestroyTexture(texture);
                }
                SDL_FreeSurface(surface);
            }
            startY += lineHeight; // 下移到下一行
        }
    }
}