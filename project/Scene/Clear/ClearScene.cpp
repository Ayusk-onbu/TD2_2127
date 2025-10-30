#include "ClearScene.h"
#include "CameraSystem.h"           // CameraSystem::GetActiveCamera
#include "Transition/TransitionHub.h"  // Transition::FadeToWith
#include "Game/GameScene.h"            // 戻り先
#include "InputManager.h"                     // Input::TriggerKey/TriggerMouse
#include <dinput.h>                    // DIK_*
#include "imguiManager.h"

void ClearScene::Initialize() {
    // クリア演出の初期化を入れたい場合はここに
     // 画像は適当な場所に用意（例: resources/ui/clear.png）
	clearSpriteWorld_.Initialize();
    clearSprite_ = std::make_unique<SpriteObject>();
    clearSprite_->Initialize(p_fngine_->GetD3D12System(), 1280.0f, 720.0f);   // D3D12System を渡す流儀なら
    // テクスチャ紐付け（直接メンバ／SetTexture のどちらか）
    clearTexTextureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/clear.png");                // これが通る型ならこれでOK


	pressSpaceWorld_.Initialize();
    pressSpaceWorld_.set_.Translation({ 384.0f,380.0f,0.0f });
	spaceSprite_ = std::make_unique<SpriteObject>();
	spaceSprite_->Initialize(p_fngine_->GetD3D12System(), 512.0f, 256.0f);
	spaceTexTextureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/spaceToTitle.png");
    // （SE再生・タイマー開始など）
}

void ClearScene::Update() {
    auto& key = InputManager::GetKey();
    // Enter / Space / 左クリック で GameScene にのみ戻る
    if (!requested_) {
        if (InputManager::GetJump()) {
            requested_ = true;
            Transition::FadeToWith(
                Transition::TransitionType::FadeSlideDown,      // 好みで FadeSlideUp 等に変更可
                []() { return new GameScene(); },      // ★戻り先固定
                0.8f, 0.8f
            );
        }
    }

    // 経過時間（固定60FPS前提の簡易カウント。Δtが取れるなら差し替えOK）
    time_ += 1.0f / 120.0f;

    // --- フェード点滅（アルファ 0〜1 のサイン波を min/max へマップ） ---
    const float s = 0.5f * (std::sin(2.0f * 3.14159265f * blinkHz_ * time_) + 1.0f); // 0..1
    spaceAlpha_ = minAlpha_ + (maxAlpha_ - minAlpha_) * s;
}

void ClearScene::Draw() {
	clearSpriteWorld_.LocalToWorld();	
    clearSprite_->SetWVPData(
        CameraSystem::GetInstance()->GetActiveCamera()->DrawUI(clearSpriteWorld_.mat_),
        clearSpriteWorld_.mat_,
        Matrix4x4::Make::Identity()
    );
	clearSprite_->Draw(p_fngine_->GetCommand(), p_fngine_->GetPSO(), p_fngine_->GetLight(), TextureManager::GetInstance()->GetTexture(clearTexTextureHandle_));

    pressSpaceWorld_.LocalToWorld();
    spaceSprite_->SetWVPData(
        CameraSystem::GetInstance()->GetActiveCamera()->DrawUI(pressSpaceWorld_.mat_),
        pressSpaceWorld_.mat_,
        Matrix4x4::Make::Identity()
    );

	spaceSprite_->SetColor({ 1.0f, 1.0f, 1.0f, spaceAlpha_ }); // アルファ値セット
    spaceSprite_->Draw(p_fngine_->GetCommand(), p_fngine_->GetPSO(), p_fngine_->GetLight(), TextureManager::GetInstance()->GetTexture(spaceTexTextureHandle_));
}
