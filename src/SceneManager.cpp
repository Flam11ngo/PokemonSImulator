#include"SceneManager.h"
#include"Scenes/BattleScene.h"

Scene* SceneManager::currentScene = nullptr;
SceneManager::SceneID SceneManager::currentSceneID = SceneManager::SceneID::None;
bool SceneManager::isLoaded = false;

void SceneManager::Init() {
    SceneManager::ChangeScene(SceneManager::SceneID::Battle);
}

void SceneManager::ChangeScene(SceneID newSceneID) {
    Clean();
    switch (newSceneID) {
    case SceneID::Battle:
        currentScene = new BattleScene();
        break;
    default:
        currentScene = nullptr;
        break;
    }
    if (currentScene != nullptr) {
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