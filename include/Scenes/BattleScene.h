#pragma once
#include"Scene.h"
#include"Battle/Battle.h"
#include"Battle/Pokemon.h"
#include"Battle/Moves.h"
#include"GUIUtils/Button.h"
#include"Image.h"
#include"Animated.h"
#include<vector>

class BattleScene : public Scene {
private:
    Battle* battle;
    Image background;
    Image playerPokemon;
    Image opponentPokemon;
    Animated opponentPokemonAnimated;
    std::vector<Button> moveButtons;
    std::vector<Button> switchButtons; // 切换宝可梦的按钮
    std::vector<Image> moveButtonBackgrounds;
    int selectedMoveIndex;
    int selectedSwitchIndex; // 选中的宝可梦索引
    bool battleActive;
    bool playerActionSet; // 玩家是否已设置操作
    bool opponentActionSet; // 对手是否已设置操作
    bool isPlayerTurn; // 当前是否是玩家回合
    bool isSwitching; // 是否正在切换宝可梦
    
    // 初始化方法
    void initializeButtons();
    void initializeSwitchButtons(); // 初始化切换宝可梦的按钮
    void handleMoveSelection();
    void handleOpponentAction(); // 处理对手操作
    void handleSwitchSelection(); // 处理宝可梦切换
    void updateMoveButtons(bool isPlayer); // 更新技能按钮
    void updateSwitchButtons(bool isPlayer); // 更新切换宝可梦的按钮
    SDL_Rect PlayerPokemonRect = {150, 300, 200, 200};
    SDL_Rect OpponentPokemonRect = {900, 50, 200, 200};
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