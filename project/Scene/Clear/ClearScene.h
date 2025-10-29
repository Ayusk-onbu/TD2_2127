#pragma once
#include "Scene.h"

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
};
