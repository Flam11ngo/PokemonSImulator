#pragma once
#include"SDLH.h"
#include"Scene.h"
class Scene;
class SceneManager {
    friend class Scene;
public:
	enum class SceneID {
		None,
		Title,
		Game,
		Menu,
		Battle,
		Backpack,
		Test
	};

private:
	static Scene* currentScene;
	static SceneID currentSceneID;
	static bool isLoaded;
public:
	
	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;
	static void Init();
	static void ChangeScene(SceneID newSceneID);
	static void Update(const float deltaTime);
	static void Render(SDL_Renderer* Renderer);
	static void Clean();
	static void HandleEvents(SDL_Event& event);
	static void LoadResources(SDL_Renderer* renderer);
};