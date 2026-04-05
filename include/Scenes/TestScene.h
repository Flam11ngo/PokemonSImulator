#pragma once
#include"Scene.h"
#include"GUIUtils/Dialog.h"
#include"GUIUtils/TextBox.h"
#include"GUIUtils/Button.h"
#include"SDLH.h"
#include"Animated.h"

class TestScene : public Scene {
private:
    Dialog testDialog;
    TextBox testTextBox;
    TextBox passwordTextBox;
    Button showDialogButton;
    Button submitButton;
    TTF_Font* font;
    bool dialogVisible;
    Animated testAnimation;

public:
    TestScene();
    ~TestScene();
    
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* renderer) override;
    void HandleEvents(SDL_Event& event) override;
    void LoadResources(SDL_Renderer* renderer) override;
    void Enter() override;
    void Exit() override;
};