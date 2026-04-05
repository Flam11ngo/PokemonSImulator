#include"GUIUtils/Label.h"

void Label::SetText(std::string text) {
    this->text = text;
}

void Label::SetColor(SDL_Color color) {
    this->color = color;
}

void Label::SetFont(TTF_Font* font) {
    this->font = font;
}

void Label::Render(SDL_Renderer* render) {
    if (!font || text.empty()) return;
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text.c_str(), color);
    if (!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(render, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    int width, height;
    SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
    
    // 左对齐文本
    SDL_Rect destRect = drect;
    destRect.w = width;
    destRect.h = height;
    destRect.x += 5; // 左边距
    destRect.y += (drect.h - height) / 2; // 垂直居中
    
    SDL_RenderCopy(render, texture, nullptr, &destRect);
    
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}