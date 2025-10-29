#pragma once
#include "Scene.h"
#include "SpriteObject.h"
#include "WorldTransform.h"
// 前方宣言（必要最小限）
namespace Transition {
    enum class TransitionType;
}
class GameScene;

class ClearScene final : public Scene {
public:
    ClearScene() = default;
    ~ClearScene() override = default;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    //void Finalize() override {}

private:
    bool requested_ = false; // 二重遷移防止
    std::unique_ptr<SpriteObject> clearSprite_;
    int clearTexTextureHandle_= -1;
	WorldTransform clearSpriteWorld_;

    std::unique_ptr<SpriteObject> spaceSprite_;
    int spaceTexTextureHandle_ = -1;
    WorldTransform pressSpaceWorld_;

private:
    // フェード点滅用
    float time_ = 0.0f;
    float blinkHz_ = 2.0f;       // 点滅周波数 [Hz]
    float minAlpha_ = 0.25f;     // 最小アルファ
    float maxAlpha_ = 1.0f;      // 最大アルファ
    float spaceAlpha_ = 1.0f;    // 計算結果（Drawで使用）

};
