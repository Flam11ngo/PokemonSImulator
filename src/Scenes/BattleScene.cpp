#include"Scenes/BattleScene.h"
#include"SceneManager.h"
#include"Core.h"
#include"Battle/Species.h"
#include"Battle/Types.h"
#include"Battle/Natures.h"
#include"Battle/Abilities.h"
#include"Battle/Weather.h"
#include"Battle/Field.h"
#include<iostream>

BattleScene::BattleScene() : battle(nullptr), selectedMoveIndex(0), selectedSwitchIndex(0), battleActive(false), playerActionSet(false), opponentActionSet(false), isPlayerTurn(true), isSwitching(false) {}

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
    
    // 加载字体
    TTF_Font* font = TTF_OpenFont("../assets/fonts/arial.ttf", 12);
    SDL_Color textColor = {255, 255, 255, 255};
    
    for (int i = 0; i < 4; i++) {
        SDL_Rect rect = {startX + i * (buttonWidth + buttonSpacingX), startY, buttonWidth, buttonHeight};
        Image buttonImage;
        moveButtons.emplace_back(rect, [this, i]() {
            selectedMoveIndex = i;
            if (isPlayerTurn) {
                handleMoveSelection();
            } else {
                // 处理对手的技能选择
                Pokemon* opponent = battle->getSideB().getActivePokemon();
                if (!opponent) return;
                
                std::vector<Move> moves = opponent->getMoves();
                if (selectedMoveIndex >= 0 && selectedMoveIndex < moves.size()) {
                    Move selectedMove = moves[selectedMoveIndex];
                    
                    // 获取玩家宝可梦
                    Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
                    if (!playerPokemonPtr) return;
                    
                    // 创建攻击动作
                    BattleAction action = BattleAction::makeAttack(opponent, playerPokemonPtr, selectedMove);
                    
                    // 入队动作
                    battle->enqueueAction(action);
                    
                    // 标记对手操作已设置
                    opponentActionSet = true;
                    std::cout << "Opponent action set: " << selectedMove.getName() << std::endl;
                    
                    // 切换回玩家回合
                    isPlayerTurn = true;
                    updateMoveButtons(true);
                    std::cout << "Opponent action completed, back to player's turn..." << std::endl;
                }
            }
        }, buttonImage, "", font, textColor);
    }
}

void BattleScene::initializeSwitchButtons() {
    // 初始化切换宝可梦的按钮
    switchButtons.reserve(6); // 最多6只宝可梦
    
    // 切换按钮的位置和大小
    int buttonWidth = windowRect.w / 8; // 宽度为窗口的1/8
    int buttonHeight = windowRect.h / 12; // 高度为窗口的1/12
    int buttonSpacingX = windowRect.w / 40; // 水平间距
    int buttonSpacingY = windowRect.h / 60; // 垂直间距
    int startX = 50; // 左边距
    int startY = 50; // 上边距
    
    // 加载字体
    TTF_Font* font = TTF_OpenFont("../assets/fonts/arial.ttf", 12);
    SDL_Color textColor = {255, 255, 255, 255};
    
    for (int i = 0; i < 6; i++) {
        int row = i / 2;
        int col = i % 2;
        SDL_Rect rect = {startX + col * (buttonWidth + buttonSpacingX), startY + row * (buttonHeight + buttonSpacingY), buttonWidth, buttonHeight};
        Image buttonImage;
        switchButtons.emplace_back(rect, [this, i]() {
            selectedSwitchIndex = i;
            handleSwitchSelection();
        }, buttonImage, "", font, textColor);
    }
}

void BattleScene::LoadResources(SDL_Renderer* renderer) {
    // 加载背景图片
    background.LoadImage(renderer, "../assets/battle/background.png");
    
    // 加载宝可梦图片
    playerPokemon.LoadImage(renderer, "../assets/pokemon/blastoise.png");
    opponentPokemon.LoadImage(renderer, "../assets/pokemon/charizard.png");
    
    // 加载对手的pichu.gif动画
    opponentPokemonAnimated.LoadAnimation(renderer, "../assets/pokemon/pichu.gif", true);
    opponentPokemonAnimated.Play();
    
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
    // 重置操作状态
    playerActionSet = false;
    opponentActionSet = false;
    isPlayerTurn = true;
    
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

    // 创建额外的宝可梦物种
    Species venusaurSpecies{
        3,
        "Venusaur",
        Type::Grass,
        Type::Poison,
        {80, 82, 83, 100, 100, 80},
        {EggGroup::Monster, EggGroup::Grass},
        {},
        0.875f,
        -1,
        0,
        {AbilityType::Overgrow}
    };

    Species pikachuSpecies{
        25,
        "Pikachu",
        Type::Electric,
        Type::Count,
        {35, 55, 40, 50, 50, 90},
        {EggGroup::Field, EggGroup::Fairy},
        {},
        0.4f,
        -1,
        0,
        {AbilityType::None}
    };

    // 创建宝可梦
    Pokemon* charizard = new Pokemon(&charizardSpecies, Nature::Adamant, AbilityType::Blaze, 50, ivs, evs);
    Pokemon* blastoise = new Pokemon(&blastoiseSpecies, Nature::Modest, AbilityType::Torrent, 50, ivs, evs);
    Pokemon* venusaur = new Pokemon(&venusaurSpecies, Nature::Modest, AbilityType::Overgrow, 50, ivs, evs);
    Pokemon* pikachu = new Pokemon(&pikachuSpecies, Nature::Timid, AbilityType::None, 50, ivs, evs);

    // 添加技能
    charizard->addMove(Move{"Flamethrower", Type::Fire, Category::Special, 90, 100, 15, MoveEffect::Burn, 10});
    charizard->addMove(Move{"Dragon Claw", Type::Dragon, Category::Physical, 80, 100, 15});
    charizard->addMove(Move{"Fly", Type::Flying, Category::Physical, 90, 95, 15});
    charizard->addMove(Move{"Earthquake", Type::Ground, Category::Physical, 100, 100, 10});

    blastoise->addMove(Move{"Hydro Pump", Type::Water, Category::Special, 110, 80, 5, MoveEffect::Paralyze, 10});
    blastoise->addMove(Move{"Ice Beam", Type::Ice, Category::Special, 90, 100, 10});
    blastoise->addMove(Move{"Earthquake", Type::Ground, Category::Physical, 100, 100, 10});
    blastoise->addMove(Move{"Rapid Spin", Type::Normal, Category::Physical, 20, 100, 40});

    venusaur->addMove(Move{"Solar Beam", Type::Grass, Category::Special, 120, 100, 10});
    venusaur->addMove(Move{"Sludge Bomb", Type::Poison, Category::Special, 90, 100, 10, MoveEffect::Poison, 30});
    venusaur->addMove(Move{"Earthquake", Type::Ground, Category::Physical, 100, 100, 10});
    venusaur->addMove(Move{"Sleep Powder", Type::Grass, Category::Status, 0, 75, 15, MoveEffect::Sleep, 100});

    pikachu->addMove(Move{"Thunderbolt", Type::Electric, Category::Special, 90, 100, 15, MoveEffect::Paralyze, 10});
    pikachu->addMove(Move{"Quick Attack", Type::Normal, Category::Physical, 40, 100, 30});
    pikachu->addMove(Move{"Iron Tail", Type::Steel, Category::Physical, 100, 75, 15, MoveEffect::StatChange, 30});
    pikachu->addMove(Move{"Thunder Wave", Type::Electric, Category::Status, 0, 90, 20, MoveEffect::Paralyze, 100});

    // 创建队伍
    Side sideA("Player");
    Side sideB("Opponent");
    sideA.addPokemon(charizard);
    sideA.addPokemon(venusaur);
    sideB.addPokemon(blastoise);
    sideB.addPokemon(pikachu);

    // 创建战斗
    battle = new Battle(sideA, sideB);
    battleActive = true;
    initializeButtons();
    initializeSwitchButtons();
    
    // 设置测试天气和场地效果
    battle->getWeather().setWeather(WeatherType::Rain, 5);
    battle->getField().setField(FieldType::Grassy, 8);
    std::cout << "Weather set to: " << battle->getWeather().getName() << std::endl;
    std::cout << "Field set to: " << battle->getField().getName() << std::endl;
    
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
    
    // 清理动画资源
    opponentPokemonAnimated.CleanUp();
    
    // 释放技能按钮背景图片资源
    for (auto& image : moveButtonBackgrounds) {
        image.CleanUp();
    }
    moveButtonBackgrounds.clear();
    
    // 清空按钮
    moveButtons.clear();
    switchButtons.clear();
}

void BattleScene::Update(float deltaTime) {
    if (!battleActive || !battle) return;
    
    // 更新对手宝可梦动画
    opponentPokemonAnimated.Update(deltaTime);
    
    // 检查双方是否都已设置操作
    if (playerActionSet && opponentActionSet) {
        std::cout << "Both actions set, processing turn..." << std::endl;
        
        // 处理回合
        battle->processTurn();
        
        // 检查战斗是否结束
        Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
        Pokemon* opponentPokemonPtr = battle->getSideB().getActivePokemon();
        
        if (playerPokemonPtr->isFainted() || opponentPokemonPtr->isFainted()) {
            battleActive = false;
            std::cout << "Battle ended!" << std::endl;
            // 战斗结束后切换到TestScene
            SceneManager::ChangeScene(SceneManager::SceneID::Test);
        } else {
            // 重置操作状态，开始下一回合
            playerActionSet = false;
            opponentActionSet = false;
            isPlayerTurn = true; // 下一回合开始时，先让玩家行动
            updateMoveButtons(true); // 更新为玩家的技能按钮
            std::cout << "Turn ended, waiting for player's action..." << std::endl;
        }
    } else if (isPlayerTurn && !playerActionSet) {
        // 玩家回合，等待玩家操作
        // 确保显示的是玩家的技能按钮
        updateMoveButtons(true);
    } else if (!isPlayerTurn && !opponentActionSet) {
        // 对手回合，等待对手操作
        // 确保显示的是对手的技能按钮
        updateMoveButtons(false);
        std::cout << "Opponent's turn, waiting for opponent's action..." << std::endl;
    } else if (playerActionSet && !opponentActionSet) {
        // 玩家已设置操作，切换到对手回合
        isPlayerTurn = false;
        updateMoveButtons(false); // 更新为对手的技能按钮
        std::cout << "Player action completed, opponent's turn..." << std::endl;
    }
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
        // 渲染pichu.gif动画
        opponentPokemonAnimated.Render(renderer, &OpponentPokemonRect);
        // 渲染对手宝可梦信息
        std::string opponentName = opponentPokemonPtr->getName();
        std::string opponentHP = "HP: " + std::to_string(opponentPokemonPtr->getCurrentHP()) + "/" + std::to_string(opponentPokemonPtr->getMaxHP());
        
        // 这里应该使用Text类来渲染文本，现在使用占位符
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect textRect = {800, 30, 200, 20};
        SDL_RenderDrawRect(renderer, &textRect);
    }
    
    // 渲染技能按钮
    if (!isSwitching) {
        Pokemon* currentPokemon;
        if (isPlayerTurn) {
            currentPokemon = battle->getSideA().getActivePokemon();
        } else {
            currentPokemon = battle->getSideB().getActivePokemon();
        }
        
        if (currentPokemon) {
            std::vector<Move> moves = currentPokemon->getMoves();
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
    } else {
        // 渲染切换宝可梦的按钮
        const auto& team = isPlayerTurn ? battle->getSideA().getTeam() : battle->getSideB().getTeam();
        
        // 定义不同的按钮颜色
        SDL_Color buttonColors[] = {
            {100, 100, 255, 255},  // 蓝色
            {100, 255, 100, 255},  // 绿色
            {255, 100, 100, 255},  // 红色
            {255, 255, 100, 255},  // 黄色
            {255, 100, 255, 255},  // 紫色
            {100, 255, 255, 255}   // 青色
        };
        
        for (int i = 0; i < switchButtons.size(); i++) {
            if (switchButtons[i].isVisible()) {
                // 绘制按钮背景颜色
                const SDL_Rect* rect = switchButtons[i].getDrect();
                if (rect) {
                    SDL_Color color = buttonColors[i % 6];
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                    SDL_RenderFillRect(renderer, rect);
                }
                
                // 渲染按钮（包括文本）
                switchButtons[i].Render(renderer);
                
                // 高亮选中的按钮
                if (i == selectedSwitchIndex) {
                    const SDL_Rect* rect = switchButtons[i].getDrect();
                    if (rect) {
                        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                        SDL_Rect highlightRect = {rect->x - 5, rect->y - 5, rect->w + 10, rect->h + 10};
                        SDL_RenderDrawRect(renderer, &highlightRect);
                    }
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
                if (isSwitching) {
                    selectedSwitchIndex = (selectedSwitchIndex - 2 + 6) % 6;
                } else {
                    selectedMoveIndex = (selectedMoveIndex - 2 + 4) % 4;
                }
                break;
            case SDLK_DOWN:
                if (isSwitching) {
                    selectedSwitchIndex = (selectedSwitchIndex + 2) % 6;
                } else {
                    selectedMoveIndex = (selectedMoveIndex + 2) % 4;
                }
                break;
            case SDLK_LEFT:
                if (isSwitching) {
                    selectedSwitchIndex = (selectedSwitchIndex - 1 + 6) % 6;
                } else {
                    selectedMoveIndex = (selectedMoveIndex - 1 + 4) % 4;
                }
                break;
            case SDLK_RIGHT:
                if (isSwitching) {
                    selectedSwitchIndex = (selectedSwitchIndex + 1) % 6;
                } else {
                    selectedMoveIndex = (selectedMoveIndex + 1) % 4;
                }
                break;
            case SDLK_RETURN:
                if (isSwitching) {
                    handleSwitchSelection();
                } else if (isPlayerTurn) {
                    handleMoveSelection();
                } else {
                    // 处理对手的技能选择
                    Pokemon* opponent = battle->getSideB().getActivePokemon();
                    if (!opponent) return;
                    
                    std::vector<Move> moves = opponent->getMoves();
                    if (selectedMoveIndex >= 0 && selectedMoveIndex < moves.size()) {
                        Move selectedMove = moves[selectedMoveIndex];
                        
                        // 获取玩家宝可梦
                        Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
                        if (!playerPokemonPtr) return;
                        
                        // 创建攻击动作
                        BattleAction action = BattleAction::makeAttack(opponent, playerPokemonPtr, selectedMove);
                        
                        // 入队动作
                        battle->enqueueAction(action);
                        
                        // 标记对手操作已设置
                        opponentActionSet = true;
                        std::cout << "Opponent action set: " << selectedMove.getName() << std::endl;
                        
                        // 切换回玩家回合
                        isPlayerTurn = true;
                        updateMoveButtons(true);
                        std::cout << "Opponent action completed, back to player's turn..." << std::endl;
                    }
                }
                break;
            case SDLK_s:
                // 按S键进入切换宝可梦模式
                isSwitching = !isSwitching;
                if (isSwitching) {
                    updateSwitchButtons(isPlayerTurn);
                    std::cout << "Entering switch mode..." << std::endl;
                } else {
                    std::cout << "Exiting switch mode..." << std::endl;
                }
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
    
    // 处理切换宝可梦的按钮事件
    for (int i = 0; i < switchButtons.size(); i++) {
        switchButtons[i].HandleEvents(event);
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
        
        // 标记玩家操作已设置
        playerActionSet = true;
        std::cout << "Player action set: " << selectedMove.getName() << std::endl;
    }
}

void BattleScene::handleOpponentAction() {
    if (!battle || opponentActionSet) return;
    
    // 获取对手宝可梦
    Pokemon* opponent = battle->getSideB().getActivePokemon();
    if (!opponent) return;
    
    // 获取玩家宝可梦
    Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
    if (!playerPokemonPtr) return;
    
    // 获取对手的技能
    std::vector<Move> opponentMoves = opponent->getMoves();
    if (!opponentMoves.empty()) {
        // 简单AI：随机选择一个技能
        int randomMoveIndex = rand() % opponentMoves.size();
        Move selectedMove = opponentMoves[randomMoveIndex];
        
        // 创建攻击动作
        BattleAction opponentAction = BattleAction::makeAttack(opponent, playerPokemonPtr, selectedMove);
        
        // 入队动作
        battle->enqueueAction(opponentAction);
        
        // 标记对手操作已设置
        opponentActionSet = true;
        std::cout << "Opponent action set: " << selectedMove.getName() << std::endl;
    }
}

void BattleScene::updateMoveButtons(bool isPlayer) {
    if (!battle || moveButtons.empty() || moveButtonBackgrounds.empty()) return;
    
    Pokemon* pokemon;
    if (isPlayer) {
        pokemon = battle->getSideA().getActivePokemon();
    } else {
        pokemon = battle->getSideB().getActivePokemon();
    }
    
    if (!pokemon) return;
    
    auto moves = pokemon->getMoves();
    for (int i = 0; i < 4 && i < moves.size(); i++) {
        if (i < moveButtons.size()) {
            moveButtons[i].SetBackground(moveButtonBackgrounds[static_cast<int>(moves[i].getType())]);
            
            // 设置技能按钮文本，显示技能名、PP/MaxPP和Power
            std::string moveName = moves[i].getName();
            int currentPP = moves[i].getPP();
            int maxPP = moves[i].getMaxPP();
            int power = moves[i].getPower();
            
            std::string buttonText = moveName + "\nPP: " + std::to_string(currentPP) + "/" + std::to_string(maxPP) + "\nPower: " + std::to_string(power);
            moveButtons[i].SetText(buttonText);
        }
    }
}

void BattleScene::updateSwitchButtons(bool isPlayer) {
    if (!battle || switchButtons.empty()) return;
    
    const auto& team = isPlayer ? battle->getSideA().getTeam() : battle->getSideB().getTeam();
    
    // 定义不同的按钮颜色
    SDL_Color buttonColors[] = {
        {100, 100, 255, 255},  // 蓝色
        {100, 255, 100, 255},  // 绿色
        {255, 100, 100, 255},  // 红色
        {255, 255, 100, 255},  // 黄色
        {255, 100, 255, 255},  // 紫色
        {100, 255, 255, 255}   // 青色
    };
    
    // 启用或禁用按钮，根据队伍中的宝可梦
    for (int i = 0; i < switchButtons.size(); i++) {
        if (i < team.size() && team[i] && !team[i]->isFainted()) {
            // 宝可梦存在且未濒死，启用按钮
            switchButtons[i].setVisible(true);
            
            // 设置按钮文本为宝可梦名字
            std::string pokemonName = team[i]->getName();
            switchButtons[i].SetText(pokemonName);
            
            // 这里应该设置按钮的背景颜色，但由于Button类使用Image作为背景，我们需要修改Button类或创建临时Image
            // 为了简化，我们可以在Render方法中直接绘制颜色
        } else {
            // 宝可梦不存在或已濒死，禁用按钮
            switchButtons[i].setVisible(false);
        }
    }
}

void BattleScene::handleSwitchSelection() {
    if (!battle) return;
    
    Side* side;
    if (isPlayerTurn) {
        side = &battle->getSideA();
    } else {
        side = &battle->getSideB();
    }
    
    const auto& team = side->getTeam();
    if (selectedSwitchIndex >= 0 && selectedSwitchIndex < team.size() && team[selectedSwitchIndex] && !team[selectedSwitchIndex]->isFainted()) {
        // 创建切换动作
        BattleAction action = BattleAction::makeSwitch(side->getActivePokemon(), selectedSwitchIndex);
        
        // 入队动作
        battle->enqueueAction(action);
        
        // 标记操作已设置
        if (isPlayerTurn) {
            playerActionSet = true;
            std::cout << "Player switched to: " << team[selectedSwitchIndex]->getName() << std::endl;
            std::cout << "Player cannot use moves this turn after switching!" << std::endl;
        } else {
            opponentActionSet = true;
            std::cout << "Opponent switched to: " << team[selectedSwitchIndex]->getName() << std::endl;
            std::cout << "Opponent cannot use moves this turn after switching!" << std::endl;
        }
        
        // 退出切换模式
        isSwitching = false;
    }
}

void BattleScene::startBattle(Battle& battle) {
    this->battle = &battle;
    battleActive = true;
    selectedMoveIndex = 0;
    playerActionSet = false;
    opponentActionSet = false;
    isPlayerTurn = true;
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