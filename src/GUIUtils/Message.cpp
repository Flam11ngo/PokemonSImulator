#include"GUIUtils/Message.h"

void Message::addMessage(std::string message) {
    messages.push_back(message);
    if (messages.size() > maxMessages) {
        messages.erase(messages.begin());
    }
    lastMessageTime = SDL_GetTicks();
}

void Message::Update(float deltaTime) {
    // 检查消息是否应该被清除
    if (!messages.empty() && SDL_GetTicks() - lastMessageTime > displayTime) {
        messages.erase(messages.begin());
        lastMessageTime = SDL_GetTicks();
    }
}

void Message::Render(SDL_Renderer* render) {
    if (messages.empty()) return;
    
    // 构建消息文本
    std::string messageText;
    for (const auto& message : messages) {
        messageText += message + "\n";
    }
    
    // 更新标签文本并渲染
    label.SetText(messageText);
    label.Render(render);
}