#include"GUIUtils/Dialog.h"

Dialog::Dialog() : 
    visible(false), 
    backgroundColor({0, 0, 0, 200}), 
    borderColor({255, 255, 255, 255}),
    currentCharIndex(0),
    lastUpdateTime(0),
    charDelay(50),
    textComplete(false) {
}

Dialog::Dialog(SDL_Rect rect, std::string title, std::string message, TTF_Font* font) : 
    drect(rect), 
    titleLabel({rect.x + 10, rect.y + 10, rect.w - 20, 30}, title, {255, 255, 255, 255}, font), 
    messageLabel({rect.x + 10, rect.y + 50, rect.w - 20, rect.h - 100}, "", {255, 255, 255, 255}, font), 
    visible(true), 
    backgroundColor({0, 0, 0, 200}), 
    borderColor({255, 255, 255, 255}),
    fullMessage(message),
    currentCharIndex(0),
    lastUpdateTime(SDL_GetTicks()),
    charDelay(50),
    textComplete(false) {
    // 初始显示第一个字符
    messageLabel.SetText(fullMessage.substr(0, 1));
}

void Dialog::addButton(std::string text, std::function<void()> callback, TTF_Font* font) {
    int buttonWidth = 100;
    int buttonHeight = 40;
    int buttonSpacing = 20;
    int startX = drect.x + (drect.w - (buttons.size() + 1) * buttonWidth - buttons.size() * buttonSpacing) / 2;
    int buttonY = drect.y + drect.h - buttonHeight - 20;
    
    SDL_Rect buttonRect = {startX + buttons.size() * (buttonWidth + buttonSpacing), buttonY, buttonWidth, buttonHeight};
    
    // 创建一个临时的Image作为按钮背景
    Image buttonImage;
    SDL_Color textColor = {255, 255, 255, 255};
    Button button(buttonRect, callback, buttonImage, text, font, textColor);
    buttons.push_back(button);
}

void Dialog::setTitle(std::string title) {
    titleLabel.SetText(title);
}

void Dialog::setMessage(std::string message) {
    fullMessage = message;
    currentCharIndex = 0;
    lastUpdateTime = SDL_GetTicks();
    textComplete = false;
    if (!fullMessage.empty()) {
        messageLabel.SetText(fullMessage.substr(0, 1));
    } else {
        messageLabel.SetText("");
    }
}

void Dialog::setVisible(bool visible) {
    this->visible = visible;
    if (visible && !fullMessage.empty()) {
        currentCharIndex = 0;
        lastUpdateTime = SDL_GetTicks();
        textComplete = false;
        messageLabel.SetText(fullMessage.substr(0, 1));
    }
}

bool Dialog::isVisible() const {
    return visible;
}

void Dialog::HandleEvents(SDL_Event& event) {
    if (!visible) return;
    
    // 处理鼠标点击事件，左键点击跳过文本
    if (event.type == SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
            SkipText();
        }
    }
    
    for (auto& button : buttons) {
        button.HandleEvents(event);
    }
}

void Dialog::Update() {
    if (!visible || textComplete) return;
    
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - lastUpdateTime >= charDelay) {
        currentCharIndex++;
        if (currentCharIndex >= fullMessage.length()) {
            textComplete = true;
        } else {
            messageLabel.SetText(fullMessage.substr(0, currentCharIndex + 1));
            lastUpdateTime = currentTime;
        }
    }
}

void Dialog::SkipText() {
    if (!textComplete) {
        currentCharIndex = fullMessage.length() - 1;
        messageLabel.SetText(fullMessage);
        textComplete = true;
    }
}

void Dialog::Render(SDL_Renderer* renderer) {
    if (!visible) return;
    
    // 绘制背景
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &drect);
    
    // 绘制边框
    SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
    SDL_RenderDrawRect(renderer, &drect);
    
    // 渲染标题和消息
    titleLabel.Render(renderer);
    messageLabel.Render(renderer);
    
    // 渲染按钮
    for (auto& button : buttons) {
        button.Render(renderer);
    }
}