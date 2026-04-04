#pragma once
#include"Scene.h"
#include"Battle/Battle.h"
#include"Battle/Pokemon.h"
#include"Battle/Moves.h"
#include"GUIUtils/Button.h"
#include"Image.h"
#include<vector>

class BattleScene : public Scene {
private:
    Battle* battle;
    Image background;
    Image playerPokemon;
    Image opponentPokemon;
    std::vector<Button> moveButtons;
    std::vector<Image> moveButtonBackgrounds;
    int selectedMoveIndex;
    bool battleActive;
    
    // 初始化方法
    void initializeButtons();
    void handleMoveSelection();
    
    // 辅助方法
    std::string getTypeString(Type type);
    
public:
    BattleScene();
    ~BattleScene();
    
    // Scene方法
    void Update(float deltaTime) override;
    void Render(SDL_Renderer* Renderer) override;
    void HandleEvents(SDL_Event& event) override;
    void LoadResources(SDL_Renderer* renderer) override;
    void Enter() override;
    void Exit() override;
    
    // 战斗相关方法
    void startBattle(Battle& battle);
};