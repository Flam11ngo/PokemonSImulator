#pragma once
#include"SDLH.h"
#include"GUIUtils/Label.h"
#include<string>

class TextBox {
private:
    SDL_Rect drect;
    Label textLabel;
    std::string text;
    bool isFocused;
    bool isPassword;
    SDL_Color backgroundColor;
    SDL_Color borderColor;
    SDL_Color focusedBorderColor;
    Uint32 cursorTime;
    bool showCursor;
    TTF_Font* font;

public:
    TextBox();
    TextBox(SDL_Rect rect, std::string placeholder, TTF_Font* font);
    
    void setText(std::string text);
    std::string getText() const;
    void setPassword(bool password);
    bool getPassword() const;
    
    void Update(float deltaTime);
    void HandleEvents(SDL_Event& event);
    void Render(SDL_Renderer* renderer);
};