#include"Scenes/BattleScene.h"
#include"SceneManager.h"
#include"Core.h"
#include"Battle/Species.h"
#include"Battle/Types.h"
#include"Battle/Natures.h"
#include"Battle/Abilities.h"
#include<iostream>

BattleScene::BattleScene() : battle(nullptr), selectedMoveIndex(0), battleActive(false) {}

BattleScene::~BattleScene() {
    if (battle) {
        delete battle;
        battle = nullptr;
    }
}

void BattleScene::initializeButtons() {
    // 初始化技能按钮
    moveButtons.reserve(4);
    
    // 技能按钮的位置和大小，根据窗口大小均匀分布
    int buttonWidth = windowRect.w / 5; // 宽度为窗口的1/5
    int buttonHeight = windowRect.h / 10; // 高度为窗口的1/10
    int buttonSpacingX = windowRect.w / 20; // 水平间距为窗口宽度的1/20
    int startX = (windowRect.w - 4 * buttonWidth - 3 * buttonSpacingX) / 2; // 水平居中
    int startY = windowRect.h - buttonHeight - 50; // 底部留出50像素
    std::cout << "startX: " << startX << ", startY: " << startY << std::endl;
    for (int i = 0; i < 4; i++) {
        SDL_Rect rect = {startX + i * (buttonWidth + buttonSpacingX), startY, buttonWidth, buttonHeight};
        Image buttonImage;
        moveButtons.emplace_back(rect, [this, i]() { selectedMoveIndex = i; handleMoveSelection(); }, buttonImage);
    }
}

void BattleScene::LoadResources(SDL_Renderer* renderer) {
    // 加载背景图片
    background.LoadImage(renderer, "../assets/battle/background.png");
    
    // 加载宝可梦图片
    playerPokemon.LoadImage(renderer, "../assets/pokemon/charizard.png");
    opponentPokemon.LoadImage(renderer, "../assets/pokemon/blastoise.png");
    
    // 加载技能按钮背景板图片
    moveButtonBackgrounds.clear();
    std::vector<Type> types = {
        Type::Normal, Type::Fire, Type::Water, Type::Electric, Type::Grass,
        Type::Ice, Type::Fighting, Type::Poison, Type::Ground, Type::Flying,
        Type::Psychic, Type::Bug, Type::Rock, Type::Ghost, Type::Dragon,
        Type::Dark, Type::Steel, Type::Fairy
    };
    
    for (Type type : types) {
        std::string typeString = getTypeString(type);
        std::string path = "../assets/move/" + typeString + "-panel.png";
        Image image(renderer, path.c_str());
        moveButtonBackgrounds.push_back(image);
    }
    // 这些代码移到Enter方法中执行，因为battle对象在Enter方法中才创建
    // auto moves = battle->getSideA().getActivePokemon()->getMoves();
    // for (int i = 0; i < 4; i++) {
    //     moveButtons[i].SetBackground(moveButtonBackgrounds[static_cast<int>(moves[i].getType())]);
    // }
}

void BattleScene::Enter() {
    std::cout << "Entering Battle Scene!" << std::endl;
    // 创建默认战斗实例
    // 创建宝可梦物种
    Species charizardSpecies{
        6,
        "Charizard",
        Type::Fire,
        Type::Flying,
        {78, 84, 78, 109, 85, 100},
        {EggGroup::Monster, EggGroup::Dragon},
        {},
        0.875f,
        -1,
        0,
        {AbilityType::Blaze}
    };

    Species blastoiseSpecies{
        9,
        "Blastoise",
        Type::Water,
        Type::Count,
        {79, 83, 100, 85, 105, 78},
        {EggGroup::Monster, EggGroup::Water1},
        {},
        0.875f,
        -1,
        0,
        {AbilityType::Torrent}
    };

    // 创建宝可梦
    std::array<int, static_cast<int>(StatIndex::Count)> ivs{};
    std::array<int, static_cast<int>(StatIndex::Count)> evs{};
    ivs.fill(31);
    evs.fill(31);

    Pokemon* charizard = new Pokemon(&charizardSpecies, Nature::Adamant, AbilityType::Blaze, 50, ivs, evs);
    Pokemon* blastoise = new Pokemon(&blastoiseSpecies, Nature::Modest, AbilityType::Torrent, 50, ivs, evs);

    // 添加技能
    charizard->addMove(Move{"Flamethrower", Type::Fire, Category::Special, 90, 100, 15, MoveEffect::Burn, 10});
    charizard->addMove(Move{"Dragon Claw", Type::Dragon, Category::Physical, 80, 100, 15});
    charizard->addMove(Move{"Fly", Type::Flying, Category::Physical, 90, 95, 15});
    charizard->addMove(Move{"Earthquake", Type::Ground, Category::Physical, 100, 100, 10});

    blastoise->addMove(Move{"Hydro Pump", Type::Water, Category::Special, 110, 80, 5, MoveEffect::Paralyze, 10});
    blastoise->addMove(Move{"Ice Beam", Type::Ice, Category::Special, 90, 100, 10});
    blastoise->addMove(Move{"Earthquake", Type::Ground, Category::Physical, 100, 100, 10});
    blastoise->addMove(Move{"Rapid Spin", Type::Normal, Category::Physical, 20, 100, 40});

    // 创建队伍
    Side sideA("Player");
    Side sideB("Opponent");
    sideA.addPokemon(charizard);
    sideB.addPokemon(blastoise);

    // 创建战斗
    battle = new Battle(sideA, sideB);
    battleActive = true;
    initializeButtons();
    
    // 设置技能按钮的背景图片
    if (battle && !moveButtonBackgrounds.empty()) {
        auto moves = battle->getSideA().getActivePokemon()->getMoves();
        for (int i = 0; i < 4 && i < moves.size(); i++) {
            if (i < moveButtons.size()) {
                moveButtons[i].SetBackground(moveButtonBackgrounds[static_cast<int>(moves[i].getType())]);
            }
        }
    }
}

void BattleScene::Exit() {
    std::cout << "Exiting Battle Scene!" << std::endl;
    battleActive = false;
    
    // 释放资源
    if (battle) {
        delete battle;
        battle = nullptr;
    }
    
    // 释放图片资源
    background.CleanUp();
    playerPokemon.CleanUp();
    opponentPokemon.CleanUp();
    
    // 释放技能按钮背景图片资源
    for (auto& image : moveButtonBackgrounds) {
        image.CleanUp();
    }
    moveButtonBackgrounds.clear();
    
    // 清空按钮
    moveButtons.clear();
}

void BattleScene::Update(float deltaTime) {
    if (!battleActive || !battle) return;
    
    // 更新战斗
    // 这里可以添加战斗逻辑的更新
}

void BattleScene::Render(SDL_Renderer* renderer) {
    if (!battleActive || !battle) return;
    
    // 渲染背景
    SDL_Rect backgroundRect = {0, 0, 1280, 720};
    // 检查背景图片是否加载成功
    if (background.getTexture()) {
        background.Render(renderer, &backgroundRect);
    } else {
        // 图片加载失败，使用默认颜色填充
        SDL_SetRenderDrawColor(renderer, 0, 100, 200, 255);
        SDL_RenderFillRect(renderer, &backgroundRect);
    }
    
    // 渲染玩家宝可梦
    Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
    if (playerPokemonPtr) {
        // 检查宝可梦图片是否加载成功
        if (playerPokemon.getTexture()) {
            playerPokemon.Render(renderer, &PlayerPokemonRect);
        } else {
            // 图片加载失败，使用默认颜色填充
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
            SDL_RenderFillRect(renderer, &PlayerPokemonRect);
        }
        
        // 渲染玩家宝可梦信息
        std::string playerName = playerPokemonPtr->getName();
        std::string playerHP = "HP: " + std::to_string(playerPokemonPtr->getCurrentHP()) + "/" + std::to_string(playerPokemonPtr->getMaxHP());
        
        // 这里应该使用Text类来渲染文本，现在使用占位符
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect textRect = {50, 280, 200, 20};
        SDL_RenderDrawRect(renderer, &textRect);
    }
    
    // 渲染对手宝可梦
    Pokemon* opponentPokemonPtr = battle->getSideB().getActivePokemon();
    if (opponentPokemonPtr) {
        // 检查宝可梦图片是否加载成功
        if (opponentPokemon.getTexture()) {
            opponentPokemon.Render(renderer, &OpponentPokemonRect);
        } else {
            // 图片加载失败，使用默认颜色填充
            SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
            SDL_RenderFillRect(renderer, &OpponentPokemonRect); 
        }
        // 渲染对手宝可梦信息
        std::string opponentName = opponentPokemonPtr->getName();
        std::string opponentHP = "HP: " + std::to_string(opponentPokemonPtr->getCurrentHP()) + "/" + std::to_string(opponentPokemonPtr->getMaxHP());
        
        // 这里应该使用Text类来渲染文本，现在使用占位符
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect textRect = {800, 30, 200, 20};
        SDL_RenderDrawRect(renderer, &textRect);
    }
    
    // 渲染技能按钮
    if (playerPokemonPtr) {
        std::vector<Move> moves = playerPokemonPtr->getMoves();
        for (int i = 0; i < moveButtons.size(); i++) {
            moveButtons[i].Render(renderer);
            // 高亮选中的按钮
            if (i == selectedMoveIndex) {
                const SDL_Rect* rect = moveButtons[i].getDrect();
                if (rect) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                    SDL_Rect highlightRect = {rect->x - 5, rect->y - 5, rect->w + 10, rect->h + 10};
                    SDL_RenderDrawRect(renderer, &highlightRect);
                }
            }
        }
    }
};

void BattleScene::HandleEvents(SDL_Event& event) {
    if (!battleActive || !battle) return;
    
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_UP:
                selectedMoveIndex = (selectedMoveIndex - 2 + 4) % 4;
                break;
            case SDLK_DOWN:
                selectedMoveIndex = (selectedMoveIndex + 2) % 4;
                break;
            case SDLK_LEFT:
                selectedMoveIndex = (selectedMoveIndex - 1 + 4) % 4;
                break;
            case SDLK_RIGHT:
                selectedMoveIndex = (selectedMoveIndex + 1) % 4;
                break;
            case SDLK_RETURN:
                handleMoveSelection();
                break;
            case SDLK_ESCAPE:
                // 由于SceneID是私有成员，我们无法直接访问它
                // 这里可以添加退出战斗的逻辑
                break;
        }
    }
    
    // 处理按钮事件
    for (int i = 0; i < moveButtons.size(); i++) {
        moveButtons[i].HandleEvents(event);
    }
}

void BattleScene::handleMoveSelection() {
    if (!battle) return;
    
    // 获取玩家宝可梦
    Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
    if (!playerPokemonPtr) return;
    
    // 获取选中的技能
    std::vector<Move> moves = playerPokemonPtr->getMoves();
    if (selectedMoveIndex >= 0 && selectedMoveIndex < moves.size()) {
        Move selectedMove = moves[selectedMoveIndex];
        
        // 获取对手宝可梦
        Pokemon* opponentPokemonPtr = battle->getSideB().getActivePokemon();
        if (!opponentPokemonPtr) return;
        
        // 创建攻击动作
        BattleAction action = BattleAction::makeAttack(playerPokemonPtr, opponentPokemonPtr, selectedMove);
        
        // 入队动作
        battle->enqueueAction(action);
        
        // 处理对手动作
        Pokemon* opponent = battle->getSideB().getActivePokemon();
        if (opponent) {
            std::vector<Move> opponentMoves = opponent->getMoves();
            if (!opponentMoves.empty()) {
                BattleAction opponentAction = BattleAction::makeAttack(opponent, playerPokemonPtr, opponentMoves[0]);
                battle->enqueueAction(opponentAction);
            }
        }
        
        // 处理回合
        battle->processTurn();
        
        // 检查战斗是否结束
        if (playerPokemonPtr->isFainted() || opponentPokemonPtr->isFainted()) {
            battleActive = false;
            std::cout << "Battle ended!" << std::endl;
            // 战斗结束后切换到TestScene
            SceneManager::ChangeScene(SceneManager::SceneID::Test);
        }
    }
}

void BattleScene::startBattle(Battle& battle) {
    this->battle = &battle;
    battleActive = true;
    selectedMoveIndex = 0;
    std::cout << "Battle started in BattleScene!" << std::endl;
}

std::string BattleScene::getTypeString(Type type) {
    switch (type) {
        case Type::Normal: return "normal";
        case Type::Fire: return "fire";
        case Type::Water: return "water";
        case Type::Electric: return "electric";
        case Type::Grass: return "grass";
        case Type::Ice: return "ice";
        case Type::Fighting: return "fighting";
        case Type::Poison: return "poison";
        case Type::Ground: return "ground";
        case Type::Flying: return "flying";
        case Type::Psychic: return "psychic";
        case Type::Bug: return "bug";
        case Type::Rock: return "rock";
        case Type::Ghost: return "ghost";
        case Type::Dragon: return "dragon";
        case Type::Dark: return "dark";
        case Type::Steel: return "steel";
        case Type::Fairy: return "fairy";
        default: return "normal";
    }
}