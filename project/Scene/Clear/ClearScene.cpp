#include "ClearScene.h"

#include "Transition/TransitionHub.h"  // Transition::FadeToWith
#include "Game/GameScene.h"            // 戻り先
#include "InputManager.h"                     // Input::TriggerKey/TriggerMouse
#include <dinput.h>                    // DIK_*
#include "imguiManager.h"

void ClearScene::Initialize() {
    // クリア演出の初期化を入れたい場合はここに
    // （SE再生・タイマー開始など）
}

void ClearScene::Update() {
    auto& key = InputManager::GetKey();
    // Enter / Space / 左クリック で GameScene にのみ戻る
    if (!requested_) {
        if (key.PressedKey(DIK_RETURN)) {
            requested_ = true;
            Transition::FadeToWith(
                Transition::TransitionType::Fade,      // 好みで FadeSlideUp 等に変更可
                []() { return new GameScene(); },      // ★戻り先固定
                0.35f, 0.35f
            );
        }
    }
}

void ClearScene::Draw() {
    // 簡易な中央バナー（必要になったらSprite描画に置き換えOK）
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 center = vp ? vp->GetCenter() : ImVec2(640, 360);
    ImGui::SetNextWindowPos(ImVec2(center.x - 200.0f, center.y - 60.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::Begin("##clear_banner", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(2.2f);
    ImGui::Text("STAGE  CLEAR !");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Text("ENTER / SPACE / CLICK : Back to Game");
    ImGui::End();
}
