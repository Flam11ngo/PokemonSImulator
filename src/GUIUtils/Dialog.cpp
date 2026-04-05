#include"GUIUtils/Dialog.h"

Dialog::Dialog() : visible(false), backgroundColor({0, 0, 0, 200}), borderColor({255, 255, 255, 255}) {
}

Dialog::Dialog(SDL_Rect rect, std::string title, std::string message, TTF_Font* font) : 
    drect(rect), 
    titleLabel({rect.x + 10, rect.y + 10, rect.w - 20, 30}, title, {255, 255, 255, 255}, font), 
    messageLabel({rect.x + 10, rect.y + 50, rect.w - 20, rect.h - 100}, message, {255, 255, 255, 255}, font), 
    visible(true), 
    backgroundColor({0, 0, 0, 200}), 
    borderColor({255, 255, 255, 255}) {
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
    messageLabel.SetText(message);
}

void Dialog::setVisible(bool visible) {
    this->visible = visible;
}

bool Dialog::isVisible() const {
    return visible;
}

void Dialog::HandleEvents(SDL_Event& event) {
    if (!visible) return;
    
    for (auto& button : buttons) {
        button.HandleEvents(event);
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