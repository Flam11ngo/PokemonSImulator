#include"GUIUtils/Button.h"
#include <sstream>

// 绘制圆角矩形的辅助函数
void drawRoundedRect(SDL_Renderer* renderer, SDL_Rect rect, int radius, SDL_Color color) {
    // 设置颜色
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    
    // 绘制中间的矩形
    SDL_Rect middleRect = {rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
    SDL_RenderFillRect(renderer, &middleRect);
    
    // 绘制顶部和底部的矩形
    SDL_Rect topRect = {rect.x, rect.y + radius, rect.w, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &topRect);
    
    // 绘制四个角的圆形
    for (int i = 0; i < 4; i++) {
        int x = rect.x + (i % 2) * (rect.w - 2 * radius) + radius;
        int y = rect.y + (i / 2) * (rect.h - 2 * radius) + radius;
        
        // 绘制一个填充的圆形
        for (int w = 0; w < 2 * radius; w++) {
            for (int h = 0; h < 2 * radius; h++) {
                int dx = radius - w;
                int dy = radius - h;
                if (dx * dx + dy * dy <= radius * radius) {
                    SDL_RenderDrawPoint(renderer, x - radius + w, y - radius + h);
                }
            }
        }
    }
}

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
    
    // 计算按钮的渲染矩形
    SDL_Rect renderRect = drect;
    
    // 根据按钮状态调整渲染矩形
    if (ButtonState == ButtonStates::Hover) {
        // 悬停状态 - 稍微放大按钮
        int scale = 5; // 放大的像素数
        renderRect.x -= scale;
        renderRect.y -= scale;
        renderRect.w += 2 * scale;
        renderRect.h += 2 * scale;
    } else if (ButtonState == ButtonStates::Pressed) {
        // 按下状态 - 稍微缩小按钮
        int scale = 2; // 缩小的像素数
        renderRect.x += scale;
        renderRect.y += scale;
        renderRect.w -= 2 * scale;
        renderRect.h -= 2 * scale;
    }
    
    // 渲染按钮背景 - 优先使用背景图
    if (background.getTexture()) {
        background.Render(render, &renderRect);
    } else {
        // 如果没有背景图，使用圆角矩形
        SDL_Color bgColor = {100, 100, 100, 255}; // 灰色背景
        drawRoundedRect(render, renderRect, 10, bgColor);
    }
    
    // 渲染按钮文本
    if (!text.empty() && font) {
        // 支持多行文本，按换行符分割
        std::string line;
        std::istringstream stream(text);
        int lineHeight = 20; // 每行文本的高度
        int startY = renderRect.y + 10; // 文本起始Y坐标
        
        while (std::getline(stream, line)) {
            SDL_Surface* surface = TTF_RenderText_Solid(font, line.c_str(), textColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(render, surface);
                if (texture) {
                    int width, height;
                    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
                    
                    // 计算文本居中的位置
                    SDL_Rect destRect = {
                        renderRect.x + (renderRect.w - width) / 2,
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