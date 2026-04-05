#include"SceneManager.h"
#include"Scenes/BattleScene.h"
#include"Scenes/TestScene.h"
#include"Core.h"
#include<iostream>

Scene* SceneManager::currentScene = nullptr;
SceneManager::SceneID SceneManager::currentSceneID = SceneManager::SceneID::None;
bool SceneManager::isLoaded = false;

void SceneManager::Init() {
    SceneManager::ChangeScene(SceneManager::SceneID::Test);
}

void SceneManager::ChangeScene(SceneID newSceneID) {
    Clean();
    switch (newSceneID) {
    case SceneID::Battle:
        currentScene = new BattleScene();
        break;
    case SceneID::Test:
        currentScene = new TestScene();
        break;
    default:
        currentScene = nullptr;
        break;
    }
    if (currentScene != nullptr) {
        // 获取窗口大小并设置到场景
        int width, height;
        Core::getWindowSize(width, height);
        currentScene->SetWindowSize(width, height);
		//std::cout << "Setting window width: " << width << ", height: " << height << std::endl;
        // 先加载资源，再调用Enter
        LoadResources(Core::getRenderer());
        currentScene->Enter();
        currentSceneID = newSceneID;
    } else {
        currentSceneID = SceneID::None;
    }
}

void SceneManager::Update(const float deltaTime) {
	if (currentScene != nullptr && isLoaded) {
		currentScene->Update(deltaTime);
	}
}

void SceneManager::Render(SDL_Renderer* Renderer) {
	if (currentScene != nullptr) {
		if (!isLoaded) {
			LoadResources(Renderer);
		}
		currentScene->Render(Renderer);
	}
}

void SceneManager::Clean() {
	if (currentScene != nullptr) {
		currentScene->Exit();
		delete currentScene;
		currentScene = nullptr;
		isLoaded = false;
	}
}

void SceneManager::HandleEvents(SDL_Event& event) {
	if (currentScene != nullptr) {
		currentScene->HandleEvents(event);
	}
}

void SceneManager::LoadResources(SDL_Renderer* renderer) {
	if (currentScene != nullptr) {
		currentScene->LoadResources(renderer);
		isLoaded = true;
	}
}