#include"GUIUtils/TextBox.h"

TextBox::TextBox() : isFocused(false), isPassword(false), 
    backgroundColor({240, 240, 240, 255}), 
    borderColor({100, 100, 100, 255}), 
    focusedBorderColor({0, 0, 255, 255}),
    cursorTime(0),
    showCursor(false),
    font(nullptr) {
}

TextBox::TextBox(SDL_Rect rect, std::string placeholder, TTF_Font* font) : 
    drect(rect), 
    textLabel({rect.x + 10, rect.y + 5, rect.w - 20, rect.h - 10}, placeholder, {150, 150, 150, 255}, font), 
    text(""), 
    isFocused(false), 
    isPassword(false), 
    backgroundColor({240, 240, 240, 255}), 
    borderColor({100, 100, 100, 255}), 
    focusedBorderColor({0, 0, 255, 255}),
    cursorTime(0),
    showCursor(false),
    font(font) {
}

void TextBox::setText(std::string text) {
    this->text = text;
    if (text.empty()) {
        textLabel.SetText("Placeholder");
        textLabel.SetColor({150, 150, 150, 255});
    } else {
        if (isPassword) {
            std::string passwordText(text.length(), '*');
            textLabel.SetText(passwordText);
        } else {
            textLabel.SetText(text);
        }
        textLabel.SetColor({0, 0, 0, 255});
    }
}

std::string TextBox::getText() const {
    return text;
}

void TextBox::setPassword(bool password) {
    isPassword = password;
    setText(text); // 重新设置文本以更新显示
}

bool TextBox::getPassword() const {
    return isPassword;
}

void TextBox::Update(float deltaTime) {
    if (isFocused) {
        cursorTime += deltaTime * 1000; // 转换为毫秒
        if (cursorTime >= 500) { // 每500毫秒切换一次光标显示状态
            showCursor = !showCursor;
            cursorTime = 0;
        }
    } else {
        showCursor = false;
        cursorTime = 0;
    }
}

void TextBox::HandleEvents(SDL_Event& event) {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        int x, y;
        SDL_GetMouseState(&x, &y);
        
        if (x >= drect.x && x <= drect.x + drect.w && y >= drect.y && y <= drect.y + drect.h) {
            isFocused = true;
            cursorTime = 0;
            showCursor = true;
        } else {
            isFocused = false;
        }
    }
    
    if (event.type == SDL_KEYDOWN && isFocused) {
        if (event.key.keysym.sym == SDLK_BACKSPACE && !text.empty()) {
            text.pop_back();
            setText(text);
        } else if (event.key.keysym.sym == SDLK_RETURN) {
            isFocused = false;
        } else if (event.key.keysym.sym >= SDLK_SPACE && event.key.keysym.sym <= SDLK_z) {
            text += event.key.keysym.sym;
            setText(text);
        }
    }
}

void TextBox::Render(SDL_Renderer* renderer) {
    // 绘制背景
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &drect);
    
    // 绘制边框
    if (isFocused) {
        SDL_SetRenderDrawColor(renderer, focusedBorderColor.r, focusedBorderColor.g, focusedBorderColor.b, focusedBorderColor.a);
    } else {
        SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    }
    SDL_RenderDrawRect(renderer, &drect);
    
    // 渲染文本
    textLabel.Render(renderer);
    
    // 渲染光标
    if (isFocused && showCursor) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        int cursorX = drect.x + 10;
        int cursorY = drect.y + 5;
        int cursorHeight = drect.h - 10;
        
        // 计算光标位置
        if (!text.empty() && font) {
            // 使用TTF_SizeText获取文本宽度
            int textWidth = 0;
            int textHeight = 0;
            std::string displayText = text;
            if (isPassword) {
                displayText = std::string(text.length(), '*');
            }
            TTF_SizeText(font, displayText.c_str(), &textWidth, &textHeight);
            cursorX += textWidth;
        }
        
        // 绘制光标
        SDL_Rect cursorRect = {cursorX, cursorY, 2, cursorHeight};
        SDL_RenderFillRect(renderer, &cursorRect);
    }
}