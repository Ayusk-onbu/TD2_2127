#include "GameScene.h"
#include "ImGuiManager.h"
#include "CameraSystem.h"
#include "Transition/TransitionHub.h"
#include "stageeditor/Stageeditor.h"
#include "InputManager.h"
GameScene::~GameScene() {
	
	Log::ViewFile("Path GameScene ~");
}

void GameScene::Initialize() {
	CameraSystem::GetInstance()->MakeCamera("NormalCamera", CameraType::Normal);
	CameraSystem::GetInstance()->MakeCamera("DebugCamera",CameraType::Debug);
	CameraSystem::GetInstance()->SetActiveCamera("DebugCamera");

}

void GameScene::Update(){
	
	CameraSystem::GetInstance()->Update();

	auto& key = InputManager::GetKey();
	if (key.PressedKey(DIK_F2)) {
		Fngine* eng = /* ここはあなたの環境で取得する */ p_fngine_; // 例
		Transition::FadeToSlideRight([eng] {
			auto* ed = new StageEditor();
			ed->BindEngine(eng);       // ★ 必須：StageEditorにエンジンを渡す
			return ed;
			}, 0.8f, 0.8f);
	}
}

void GameScene::Draw() {
	
}