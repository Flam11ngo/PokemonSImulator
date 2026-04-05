#pragma once
#include"SDLH.h"
#include"GUIUtils/Button.h"
#include"GUIUtils/Label.h"
#include<vector>
#include<string>

class Dialog {
private:
    SDL_Rect drect;
    Label titleLabel;
    Label messageLabel;
    std::vector<Button> buttons;
    bool visible;
    SDL_Color backgroundColor;
    SDL_Color borderColor;
    
    // 逐字显示相关
    std::string fullMessage;
    int currentCharIndex;
    Uint32 lastUpdateTime;
    int charDelay; // 每个字符显示的延迟时间（毫秒）
    bool textComplete;

public:
    Dialog();
    Dialog(SDL_Rect rect, std::string title, std::string message, TTF_Font* font);
    
    void addButton(std::string text, std::function<void()> callback, TTF_Font* font);
    void setTitle(std::string title);
    void setMessage(std::string message);
    void setVisible(bool visible);
    bool isVisible() const;
    
    void HandleEvents(SDL_Event& event);
    void Update();
    void Render(SDL_Renderer* renderer);
    void SkipText(); // 跳过文本显示
};