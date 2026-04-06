#include"Scenes/BattleScene.h"
#include"SceneManager.h"
#include"Battle/Species.h"
#include"Battle/Types.h"
#include"Battle/Abilities.h"
#include"Battle/Weather.h"
#include"Battle/Field.h"
#include"Battle/BuildFromJson.h"
#include<iostream>

BattleScene::BattleScene() : battle(nullptr), selectedMoveIndex(0), selectedSwitchIndex(0), battleActive(false), playerActionSet(false), opponentActionSet(false), isPlayerTurn(true), isSwitching(false), isBagOpen(false) {
    std::cerr << "BattleScene constructor called" << std::endl;
}

BattleScene::~BattleScene() {
    std::cerr << "BattleScene destructor called" << std::endl;
    // 不再在析构函数中删除battle对象，因为它已经在Exit方法中被删除了
    // 这样可以避免双重删除导致的Segmentation fault
    std::cerr << "BattleScene destructor finished" << std::endl;
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
                    
                    // 切换回玩家回合
                    isPlayerTurn = true;
                    updateMoveButtons(true);
                }
            }
        }, buttonImage, "", font, textColor);
    }
    
    // 初始化背包按钮
    int bagButtonWidth = 100;
    int bagButtonHeight = 50;
    int bagButtonX = windowRect.w - bagButtonWidth - 20; // 右上角，右边距20像素
    int bagButtonY = 20; // 上边距20像素
    SDL_Rect bagButtonRect = {bagButtonX, bagButtonY, bagButtonWidth, bagButtonHeight};
    Image bagButtonImage;
    bagButton = Button(bagButtonRect, [this]() {
        isBagOpen = !isBagOpen; // 切换背包状态
    }, bagButtonImage, "Bag", font, textColor);
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
    
    // 加载雨水动画
    rainAnimation.LoadAnimation(renderer, "../assets/weather/rainy");
    rainAnimation.setPosition(0, 0);
    rainAnimation.setSize(1280, 720);
    rainAnimation.Play();
}

void BattleScene::Enter() {
    // 重置操作状态
    playerActionSet = false;
    opponentActionSet = false;
    isPlayerTurn = true;
    
    // 创建默认战斗实例
    // 创建宝可梦物种
    Species blastoiseSpecies{
        9,
        "Blastoise",
        160,
        855,
        Type::Water,
        Type::Count,
        {79, 83, 100, 85, 105, 78},
        {EggGroup::Monster, EggGroup::Water1},
        {},
        0.875f,
        -1,
        0,
        {AbilityType::Torrent},
        AbilityType::None
    };

    // 创建宝可梦
    std::array<int, static_cast<int>(StatIndex::Count)> ivs{};
    std::array<int, static_cast<int>(StatIndex::Count)> evs{};
    ivs.fill(31);
    evs.fill(31);

    // 从JSON文件加载宝可梦
    Pokemon charizard = BuildFromJson::loadPokemonFromFile("data/charizard.json");
    Pokemon pikachu = BuildFromJson::loadPokemonFromFile("data/pikachu.json");
    Pokemon blastoise = BuildFromJson::loadPokemonFromFile("data/blastoise.json");
    Pokemon venusaur = BuildFromJson::loadPokemonFromFile("data/venusaur.json");
    
    // 创建宝可梦指针
    Pokemon* charizardPtr = new Pokemon(charizard);
    Pokemon* pikachuPtr = new Pokemon(pikachu);
    Pokemon* blastoisePtr = new Pokemon(blastoise);
    Pokemon* venusaurPtr = new Pokemon(venusaur);

    // 技能已经从JSON文件中加载，不需要手动添加

    // 创建队伍
    Side sideA("Player");
    Side sideB("Opponent");
    sideA.addPokemon(charizardPtr);
    sideA.addPokemon(venusaurPtr);
    sideB.addPokemon(blastoisePtr);
    sideB.addPokemon(pikachuPtr);

    // 创建战斗
    battle = new Battle(sideA, sideB);
    battleActive = true;
    initializeButtons();
    initializeSwitchButtons();
    
    // 初始化血条对象
    initializeHealthBars();
    
    // 设置测试天气和场地效果
    battle->getWeather().setWeather(WeatherType::Rain, 5);
    battle->getField().setField(FieldType::Grassy, 8);
    
    // 更新技能按钮
    updateMoveButtons(true);
}

void BattleScene::Exit() {
    std::cerr << "BattleScene Exit called" << std::endl;
    battleActive = false;
    
    // 释放图片资源
    std::cerr << "BattleScene cleaning up images" << std::endl;
    background.CleanUp();
    playerPokemon.CleanUp();
    opponentPokemon.CleanUp();
    
    // 清理动画资源
    std::cerr << "BattleScene cleaning up animations" << std::endl;
    opponentPokemonAnimated.CleanUp();
    rainAnimation.CleanUp();
    
    // 清空按钮
    std::cerr << "BattleScene clearing buttons" << std::endl;
    moveButtons.clear();
    switchButtons.clear();
    
    // 释放技能按钮背景图片资源
    std::cerr << "BattleScene cleaning up move button backgrounds" << std::endl;
    std::cerr << "moveButtonBackgrounds size: " << moveButtonBackgrounds.size() << std::endl;
    for (size_t i = 0; i < moveButtonBackgrounds.size(); i++) {
        std::cerr << "Cleaning up image " << i << std::endl;
        moveButtonBackgrounds[i].CleanUp();
        std::cerr << "Image " << i << " cleaned up" << std::endl;
    }
    moveButtonBackgrounds.clear();
    std::cerr << "moveButtonBackgrounds cleared" << std::endl;
    
    // 释放资源
    std::cerr << "BattleScene cleaning up battle" << std::endl;
    if (battle) {
        std::cerr << "Deleting battle..." << std::endl;
        delete battle;
        std::cerr << "Battle deleted, setting to nullptr" << std::endl;
        battle = nullptr;
    }
    std::cerr << "BattleScene Exit finished" << std::endl;
}

void BattleScene::Update(float deltaTime) {
    if (!battleActive || !battle) return;
    
    // 取消Opponents的动画更新
    // opponentPokemonAnimated.Update(deltaTime);
    
    // 更新雨水动画
    if (battle->getWeather().type == WeatherType::Rain) {
        rainAnimation.Update(deltaTime);
    }
    
    // 血条更新
    Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
    Pokemon* opponentPokemonPtr = battle->getSideB().getActivePokemon();
    
    static Pokemon* lastPlayerPokemon = nullptr;
    static Pokemon* lastOpponentPokemon = nullptr;
    
    updateHealthBar(playerHealthBar, playerPokemonPtr, lastPlayerPokemon, deltaTime);
    updateHealthBar(opponentHealthBar, opponentPokemonPtr, lastOpponentPokemon, deltaTime);
    
    // 检查双方是否都已设置操作
        if (playerActionSet && opponentActionSet) {
            
            // 处理回合
        battle->processTurn();
        
        // 检查宝可梦是否切换，如果切换则立即更新血条
        static Pokemon* lastPlayerPokemon = nullptr;
        static Pokemon* lastOpponentPokemon = nullptr;
        
        Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
        Pokemon* opponentPokemonPtr = battle->getSideB().getActivePokemon();
        
        // 直接更新血条，不使用动画效果
        if (playerPokemonPtr) {
            if (lastPlayerPokemon != playerPokemonPtr) {
                playerHealthBar.setMaxHealth(playerPokemonPtr->getMaxHP());
                playerHealthBar.setCurrentHealthImmediately(playerPokemonPtr->getCurrentHP());
                lastPlayerPokemon = playerPokemonPtr;
            }
        }
        
        if (opponentPokemonPtr) {
            if (lastOpponentPokemon != opponentPokemonPtr) {
                opponentHealthBar.setMaxHealth(opponentPokemonPtr->getMaxHP());
                opponentHealthBar.setCurrentHealthImmediately(opponentPokemonPtr->getCurrentHP());
                lastOpponentPokemon = opponentPokemonPtr;
            }
        }
        
        // 检查战斗是否结束
        
        if (playerPokemonPtr->isFainted() || opponentPokemonPtr->isFainted()) {
            battleActive = false;
            // 战斗结束后切换到TestScene
            SceneManager::ChangeScene(SceneManager::SceneID::Test);
            return; // 立即返回，避免使用已删除的对象
        } else {
            // 重置操作状态，开始下一回合
            playerActionSet = false;
            opponentActionSet = false;
            isPlayerTurn = true; // 下一回合开始时，先让玩家行动
            updateMoveButtons(true); // 更新为玩家的技能按钮
        }
    } else if (isPlayerTurn && !playerActionSet) {
        // 玩家回合，等待玩家操作
        // 确保显示的是玩家的技能按钮
        updateMoveButtons(true);
    } else if (!isPlayerTurn && !opponentActionSet) {
        // 对手回合，等待对手操作
        // 确保显示的是对手的技能按钮
        updateMoveButtons(false);
    } else if (playerActionSet && !opponentActionSet) {
        // 玩家已设置操作，切换到对手回合
        isPlayerTurn = false;
        updateMoveButtons(false); // 更新为对手的技能按钮
    }
}

void BattleScene::Render(SDL_Renderer* renderer) {
    if (!battleActive || !battle) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
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
        renderPokemon(renderer, playerPokemonPtr, PlayerPokemonRect, playerHealthBar);
    }
    
    // 渲染对手宝可梦
    Pokemon* opponentPokemonPtr = battle->getSideB().getActivePokemon();
    if (opponentPokemonPtr) {
        renderPokemon(renderer, opponentPokemonPtr, OpponentPokemonRect, opponentHealthBar);
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
    
    // 检查是否需要虚化战斗场景
    bool needBlur = isSwitching || isBagOpen;
    
    // 如果需要虚化战斗场景
    if (needBlur) {
        // 绘制半透明白色矩形，实现虚化效果
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_Rect blurRect = {0, 0, windowRect.w, windowRect.h};
        SDL_RenderFillRect(renderer, &blurRect);
        
        // 如果是打开背包，渲染背包界面
        if (isBagOpen) {
            renderBag(renderer, isPlayerTurn);
        }
    }
    
    // 渲染背包按钮（在虚化效果之上）
    bagButton.Render(renderer);
    
    // 渲染雨水动画
    if (battle->getWeather().type == WeatherType::Rain) {
        rainAnimation.Render(renderer);
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
                        
                        // 切换回玩家回合
                        isPlayerTurn = true;
                        updateMoveButtons(true);
                    }
                }
                break;
            case SDLK_s:
                // 按S键进入切换宝可梦模式
                isSwitching = !isSwitching;
                if (isSwitching) {
                    updateSwitchButtons(isPlayerTurn);
                }
                break;
            case SDLK_ESCAPE:
                // 由于SceneID是私有成员，我们无法直接访问它
                // 这里可以添加退出战斗的逻辑
                break;
        }
    }
    
    // 处理背包按钮事件
    bagButton.HandleEvents(event);
    
    if (battle) {
        // 处理按钮事件
        for (int i = 0; i < moveButtons.size(); i++) {
            moveButtons[i].HandleEvents(event);
        }
        
        // 处理切换宝可梦的按钮事件
        for (int i = 0; i < switchButtons.size(); i++) {
            switchButtons[i].HandleEvents(event);
        }
        
        // 处理背包中宝可梦切换按钮的事件
        if (isBagOpen) {
            handlePokemonSwitchEvents(event, isPlayerTurn);
        }
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
        } else {
            opponentActionSet = true;
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

void BattleScene::renderPokemonButtons(SDL_Renderer* renderer, bool isPlayer) {
    if (!battle) return;
    
    const auto& team = isPlayer ? battle->getSideA().getTeam() : battle->getSideB().getTeam();
    
    // 宝可梦按钮的位置和大小
    int buttonWidth = windowRect.w / 8;
    int buttonHeight = windowRect.h / 12;
    int buttonSpacingX = windowRect.w / 40;
    int buttonSpacingY = windowRect.h / 60;
    int startX = windowRect.w / 4 + 20;
    int startY = windowRect.h / 4 + 60;
    
    // 定义不同的按钮颜色
    SDL_Color buttonColors[] = {
        {100, 100, 255, 255},  // 蓝色
        {100, 255, 100, 255},  // 绿色
        {255, 100, 100, 255},  // 红色
        {255, 255, 100, 255},  // 黄色
        {255, 100, 255, 255},  // 紫色
        {100, 255, 255, 255}   // 青色
    };
    
    // 加载字体
    TTF_Font* font = TTF_OpenFont("../assets/fonts/arial.ttf", 12);
    SDL_Color textColor = {0, 0, 0, 255};
    
    for (int i = 0; i < team.size(); i++) {
        if (team[i] && !team[i]->isFainted()) {
            int row = i / 2;
            int col = i % 2;
            SDL_Rect rect = {startX + col * (buttonWidth + buttonSpacingX), startY + row * (buttonHeight + buttonSpacingY), buttonWidth, buttonHeight};
            
            // 绘制按钮背景颜色
            SDL_Color color = buttonColors[i % 6];
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(renderer, &rect);
            
            // 绘制按钮边框
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderDrawRect(renderer, &rect);
            
            // 渲染宝可梦名字
            if (font) {
                std::string pokemonName = team[i]->getName();
                std::string hpText = "HP: " + std::to_string(team[i]->getCurrentHP()) + "/" + std::to_string(team[i]->getMaxHP());
                std::string buttonText = pokemonName + "\n" + hpText;
                
                SDL_Surface* surface = TTF_RenderText_Solid(font, buttonText.c_str(), textColor);
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                    if (texture) {
                        int textWidth, textHeight;
                        SDL_QueryTexture(texture, nullptr, nullptr, &textWidth, &textHeight);
                        SDL_Rect textRect = {
                            rect.x + (rect.w - textWidth) / 2,
                            rect.y + (rect.h - textHeight) / 2,
                            textWidth,
                            textHeight
                        };
                        SDL_RenderCopy(renderer, texture, nullptr, &textRect);
                        SDL_DestroyTexture(texture);
                    }
                    SDL_FreeSurface(surface);
                }
            }
        }
    }
    
    if (font) {
        TTF_CloseFont(font);
    }
}

void BattleScene::handlePokemonSwitchEvents(SDL_Event& event, bool isPlayer) {
    if (!battle) return;
    
    const auto& team = isPlayer ? battle->getSideA().getTeam() : battle->getSideB().getTeam();
    
    // 宝可梦按钮的位置和大小
    int buttonWidth = windowRect.w / 8;
    int buttonHeight = windowRect.h / 12;
    int buttonSpacingX = windowRect.w / 40;
    int buttonSpacingY = windowRect.h / 60;
    int startX = windowRect.w / 4 + 20;
    int startY = windowRect.h / 4 + 60;
    
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mouseX = event.button.x;
        int mouseY = event.button.y;
        
        for (int i = 0; i < team.size(); i++) {
            if (team[i] && !team[i]->isFainted()) {
                int row = i / 2;
                int col = i % 2;
                SDL_Rect rect = {startX + col * (buttonWidth + buttonSpacingX), startY + row * (buttonHeight + buttonSpacingY), buttonWidth, buttonHeight};
                
                if (mouseX >= rect.x && mouseX <= rect.x + rect.w && mouseY >= rect.y && mouseY <= rect.y + rect.h) {
                    // 创建切换动作
                    Side* side = isPlayer ? &battle->getSideA() : &battle->getSideB();
                    BattleAction action = BattleAction::makeSwitch(side->getActivePokemon(), i);
                    
                    // 入队动作
                    battle->enqueueAction(action);
                    
                    // 标记操作已设置
                    if (isPlayer) {
                        playerActionSet = true;
                    } else {
                        opponentActionSet = true;

                    }
                    
                    // 关闭背包
                    isBagOpen = false;
                    break;
                }
            }
        }
    }
}

void BattleScene::updateHealthBar(HealthBar& healthBar, Pokemon* pokemon, Pokemon*& lastPokemon, float deltaTime) {
    if (pokemon) {
        // 检查宝可梦是否更换
        if (lastPokemon != pokemon) {
            // 宝可梦更换，直接设置血条，跳过动画
            healthBar.setMaxHealth(pokemon->getMaxHP());
            healthBar.setCurrentHealthImmediately(pokemon->getCurrentHP());
            lastPokemon = pokemon;
        } else {
            // 宝可梦未更换，使用动画效果
            healthBar.setCurrentHealth(pokemon->getCurrentHP());
            healthBar.update(deltaTime);
        }
    }
}

void BattleScene::initializeHealthBars() {
    if (battle) {
        Pokemon* playerPokemonPtr = battle->getSideA().getActivePokemon();
        Pokemon* opponentPokemonPtr = battle->getSideB().getActivePokemon();
        
        // 玩家宝可梦血条 - 锁定在宝可梦头上
        if (playerPokemonPtr) {
            SDL_Rect playerHpBarRect = {PlayerPokemonRect.x, PlayerPokemonRect.y - 20, PlayerPokemonRect.w, 10};
            playerHealthBar = HealthBar(playerHpBarRect, playerPokemonPtr->getMaxHP());
            playerHealthBar.setCurrentHealth(playerPokemonPtr->getCurrentHP());
        }
        
        // 对手宝可梦血条 - 锁定在宝可梦头上
        if (opponentPokemonPtr) {
            SDL_Rect opponentHpBarRect = {OpponentPokemonRect.x, OpponentPokemonRect.y - 20, OpponentPokemonRect.w, 10};
            opponentHealthBar = HealthBar(opponentHpBarRect, opponentPokemonPtr->getMaxHP());
            opponentHealthBar.setCurrentHealth(opponentPokemonPtr->getCurrentHP());
        }
    }
}

void BattleScene::renderPokemon(SDL_Renderer* renderer, Pokemon* pokemon, const SDL_Rect& rect, HealthBar& healthBar) {
    if (!pokemon) return;
    
    // 渲染宝可梦图片
    if (playerPokemon.getTexture() && rect.x == PlayerPokemonRect.x && rect.y == PlayerPokemonRect.y && rect.w == PlayerPokemonRect.w && rect.h == PlayerPokemonRect.h) {
        playerPokemon.Render(renderer, const_cast<SDL_Rect*>(&rect));
    } else if (opponentPokemon.getTexture() && rect.x == OpponentPokemonRect.x && rect.y == OpponentPokemonRect.y && rect.w == OpponentPokemonRect.w && rect.h == OpponentPokemonRect.h) {
        opponentPokemon.Render(renderer, const_cast<SDL_Rect*>(&rect));
    } else {
        // 图片加载失败，使用默认颜色填充
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        SDL_RenderFillRect(renderer, const_cast<SDL_Rect*>(&rect));
    }
    
    // 更新血条位置，确保它始终在宝可梦头上
    healthBar.setPosition(rect.x, rect.y - 20);
    healthBar.setSize(rect.w, 10);
    
    // 渲染宝可梦名字（在血条上方）
    TTF_Font* font = TTF_OpenFont("../assets/fonts/arial.ttf", 14);
    if (font) {
        SDL_Color textColor = {0, 0, 0, 255}; // 黑色字体
        std::string pokemonName = pokemon->getName();
        
        SDL_Surface* surface = TTF_RenderText_Solid(font, pokemonName.c_str(), textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                int textWidth, textHeight;
                SDL_QueryTexture(texture, nullptr, nullptr, &textWidth, &textHeight);
                SDL_Rect textRect = {
                    rect.x + (rect.w - textWidth) / 2,
                    rect.y - 40,
                    textWidth,
                    textHeight
                };
                SDL_RenderCopy(renderer, texture, nullptr, &textRect);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
        TTF_CloseFont(font);
    }
    
    // 渲染血条
    healthBar.render(renderer);
}

void BattleScene::renderBag(SDL_Renderer* renderer, bool isPlayer) {
    // 渲染背包背景
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_Rect bagRect = {windowRect.w / 4, windowRect.h / 4, windowRect.w / 2, windowRect.h / 2};
    SDL_RenderFillRect(renderer, &bagRect);
    
    // 渲染背包标题
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect titleRect = {windowRect.w / 4 + 10, windowRect.h / 4 + 10, windowRect.w / 2 - 20, 40};
    SDL_RenderDrawRect(renderer, &titleRect);
    
    // 渲染背包标题文本
    TTF_Font* font = TTF_OpenFont("../assets/fonts/arial.ttf", 24);
    if (font) {
        SDL_Color textColor = {0, 0, 0, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(font, "Bag", textColor);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                int width, height;
                SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
                SDL_Rect textRect = {
                    windowRect.w / 4 + (windowRect.w / 2 - width) / 2,
                    windowRect.h / 4 + 15,
                    width,
                    height
                };
                SDL_RenderCopy(renderer, texture, nullptr, &textRect);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
        TTF_CloseFont(font);
    }
    
    // 渲染宝可梦按钮
    renderPokemonButtons(renderer, isPlayer);
}