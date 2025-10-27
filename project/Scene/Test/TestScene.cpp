#include "TestScene.h"
#include "SceneDirector.h"
#include "ImGuiManager.h"
#include "CameraSystem.h"
#include "Transition/TransitionHub.h"
#include "GameScene.h"
// ================================
// Override Functions
// ================================

void TestScene::Initialize() {
	// Initialization code for the game scene
	CameraSystem::GetInstance()->MakeCamera("DebugCamera", CameraType::Debug);
	CameraSystem::GetInstance()->SetActiveCamera("DebugCamera");
	player_.Initialize(p_fngine_);
}

void TestScene::Update() {

	if (InputManager::GetKey().PressKey(DIK_SPACE)) {
		////hasRequestedNextScene_ = true;	
		//Transition::FadeTo([]() { return new GameScene(); }, 1.2f, 1.0f);

		// 種類を引数で渡すだけ
		Transition::FadeToSlideUp([] { return new GameScene(); }, 2.0f, 1.0f);


	}

	CameraSystem::GetInstance()->Update();
	player_.Update();

	ImGui::Begin("BlendMode");
	if (ImGui::Button("Alpha")) {
		p_fngine_->GetPSO().SetBlendState(BLENDMODE::AlphaBlend);
	}
	if (ImGui::Button("Add")) {
		p_fngine_->GetPSO().SetBlendState(BLENDMODE::Additive);
	}
	if (ImGui::Button("Sub")) {
		p_fngine_->GetPSO().SetBlendState(BLENDMODE::Subtractive);
	}
	if (ImGui::Button("Mul")) {
		p_fngine_->GetPSO().SetBlendState(BLENDMODE::Multiplicative);
	}
	if (ImGui::Button("Screen")) {
		p_fngine_->GetPSO().SetBlendState(BLENDMODE::ScreenBlend);
	}
	ImGui::End();

	/*if (hasRequestedNextScene_) {
		p_sceneDirector_->RequestChangeScene(new GameScene());
	}*/
}

void TestScene::Draw() {
	player_.Draw();
}