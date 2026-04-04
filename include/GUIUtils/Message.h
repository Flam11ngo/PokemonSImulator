#pragma once
#include"SDLH.h"
#include"GUIUtils/Label.h"
#include<vector>
#include<string>
class Message {
private:
	std::vector<std::string> messages;
	Label label;
	int maxMessages = 5;
	int displayTime = 2000; // 显示时间（毫秒）
	Uint32 lastMessageTime = 0;
public:
	Message(){};
	Message(SDL_Rect rect, TTF_Font* font) : label(rect, "", {255,255,255,255}, font) {}
	void addMessage(std::string message);
	void Update(float deltaTime);
	void Render(SDL_Renderer* render);
};