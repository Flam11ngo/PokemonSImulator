#include"Scenes/TestScene.h"
#include"Core.h"
#include"SceneManager.h"
#include<iostream>

TestScene::TestScene() : font(nullptr), dialogVisible(false) {
}

TestScene::~TestScene() {
    // 字体资源在Exit方法中已经释放
}

void TestScene::LoadResources(SDL_Renderer* renderer) {
    // 加载字体
    font = TTF_OpenFont("../assets/fonts/arial.ttf", 24);
    
    // 加载测试动画（使用Pikachu图集）
    bool loaded = testAnimation.LoadAnimation(renderer, "../assets/pokemon/Pikachu", true);
    testAnimation.Play();
}

void TestScene::Enter() {
    // 初始化对话框
    SDL_Rect dialogRect = {windowRect.w / 2 - 200, windowRect.h / 2 - 150, 400, 300};
    testDialog = Dialog(dialogRect, "Test Dialog", "This is a test dialog box with buttons.", font);
    
    // 添加对话框按钮
    testDialog.addButton("OK", [this]() {
        testDialog.setVisible(false);
    }, font);
    
    testDialog.addButton("Cancel", [this]() {
        testDialog.setVisible(false);
    }, font);
    
    // 初始时隐藏对话框
    testDialog.setVisible(false);
    
    // 初始化文本框
    SDL_Rect textBoxRect = {windowRect.w / 2 - 150, windowRect.h / 2 - 50, 300, 40};
    testTextBox = TextBox(textBoxRect, "Enter text here", font);
    
    // 初始化密码文本框
    SDL_Rect passwordRect = {windowRect.w / 2 - 150, windowRect.h / 2 + 20, 300, 40};
    passwordTextBox = TextBox(passwordRect, "Enter password", font);
    passwordTextBox.setPassword(true);
    
    // 初始化显示对话框按钮
    SDL_Rect showDialogButtonRect = {windowRect.w / 2 - 100, windowRect.h / 2 + 100, 200, 40};
    Image buttonImage;
    showDialogButton = Button(showDialogButtonRect, [this]() {
        testDialog.setVisible(true);
    }, buttonImage);
    
    // 初始化提交按钮
    SDL_Rect submitButtonRect = {windowRect.w / 2 - 100, windowRect.h / 2 + 160, 200, 40};
    submitButton = Button(submitButtonRect, [this]() {
        // 切换到BattleScene
        SceneManager::ChangeScene(SceneManager::SceneID::Battle);
    }, buttonImage);
}

void TestScene::Exit() {
    // 释放资源
    if (font) {
        TTF_CloseFont(font);
        font = nullptr;
    }
    testAnimation.CleanUp();
}

void TestScene::Update(float deltaTime) {
    // 更新文本框，处理光标闪烁
    testTextBox.Update(deltaTime);
    passwordTextBox.Update(deltaTime);
    
    // 更新对话框，处理文本逐字显示
    testDialog.Update();
    
    // 更新测试动画
    testAnimation.Update(deltaTime);
}

void TestScene::Render(SDL_Renderer* renderer) {
    // 清空屏幕
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);
    
    // 渲染文本框
    testTextBox.Render(renderer);
    passwordTextBox.Render(renderer);
    
    // 渲染按钮
    showDialogButton.Render(renderer);
    submitButton.Render(renderer);
    
    // 渲染对话框
    testDialog.Render(renderer);
    
    // 渲染动画
    SDL_Rect animationRect = {100, 100, 200, 200};
    testAnimation.Render(renderer, &animationRect);
    
    // 渲染按钮文本
    if (font) {
        SDL_Color textColor = {255, 255, 255, 255};
        
        // 显示对话框按钮文本
        SDL_Surface* showDialogTextSurface = TTF_RenderText_Solid(font, "Show Dialog", textColor);
        if (showDialogTextSurface) {
            SDL_Texture* showDialogTextTexture = SDL_CreateTextureFromSurface(renderer, showDialogTextSurface);
            if (showDialogTextTexture) {
                SDL_Rect textRect = {showDialogButton.getDrect()->x + 20, showDialogButton.getDrect()->y + 10, showDialogTextSurface->w, showDialogTextSurface->h};
                SDL_RenderCopy(renderer, showDialogTextTexture, nullptr, &textRect);
                SDL_DestroyTexture(showDialogTextTexture);
            }
            SDL_FreeSurface(showDialogTextSurface);
        }
        
        // 提交按钮文本
        SDL_Surface* submitTextSurface = TTF_RenderText_Solid(font, "Submit", textColor);
        if (submitTextSurface) {
            SDL_Texture* submitTextTexture = SDL_CreateTextureFromSurface(renderer, submitTextSurface);
            if (submitTextTexture) {
                SDL_Rect textRect = {submitButton.getDrect()->x + 60, submitButton.getDrect()->y + 10, submitTextSurface->w, submitTextSurface->h};
                SDL_RenderCopy(renderer, submitTextTexture, nullptr, &textRect);
                SDL_DestroyTexture(submitTextTexture);
            }
            SDL_FreeSurface(submitTextSurface);
        }
    }
}

void TestScene::HandleEvents(SDL_Event& event) {
    // 处理文本框事件
    testTextBox.HandleEvents(event);
    passwordTextBox.HandleEvents(event);
    
    // 处理按钮事件
    showDialogButton.HandleEvents(event);
    submitButton.HandleEvents(event);
    
    // 处理对话框事件
    testDialog.HandleEvents(event);
}