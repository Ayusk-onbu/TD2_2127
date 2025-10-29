#include "Particle.h"
#include "CameraSystem.h"
#include "TextureManager.h"
#include "ImGuiManager.h"
#include "Easing.h"

void SimpleParticleEmitter::Update() {
    frameCounter++;
    // パーティクル生成
    if (frameCounter % spawnInterval == 0) {
        for (int i = 0; i < spawnCountPerFrame; ++i) {
            Particle p;
            p.sprite = std::make_unique<SpriteObject>();
            p.position = emitterPos;
            p.velocity = direction * speed;
            p.life = particleLife;
           
			p.sprite->Initialize(fngine_->GetD3D12System(), 100.0f, 10.0f);
            p.worldTransform.Initialize();
			p.worldTransform.set_.Translation(p.position);
            particles.push_back(std::move(p));
        }
    }
    // パーティクル更新
    for (auto& p : particles) {
        p.position += p.velocity;
        p.life--;
        p.worldTransform.set_.Translation(p.position);
    }
    // 死んだパーティクルを削除
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0; }),
        particles.end());
	ImGui::Begin("Particle Emitter");
	ImGui::Text("Particle Count: %d", static_cast<int>(particles.size()));
	ImGui::End();
}

void SimpleParticleEmitter::Draw() {
    for (auto& p : particles) {
        p.worldTransform.LocalToWorld();
        p.sprite->SetWVPData(
            CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(p.worldTransform.mat_),
            p.worldTransform.mat_,
			Matrix4x4::Make::Identity());
        p.sprite->Draw(fngine_->GetCommand(),fngine_->GetPSO(),fngine_->GetLight(),TextureManager::GetInstance()->GetTexture(textureHandle));
    }
}



void ModelParticleEmitter::Update() {
    frameCounter++;

    // ランダム生成器の準備（Updateごとに初期化するのは非効率的ですが、ここでは簡易的に）
    // 通常はクラスのメンバとして保持します。
    static std::mt19937 gen(static_cast<unsigned int>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    // パーティクル生成
    if (frameCounter % spawnInterval == 0) {
        for (int i = 0; i < spawnCountPerFrame; ++i) {
            Particle p;
            p.model = std::make_unique<ModelObject>();
            p.life = particleLife;

            // ----------------------------------------------------------------------
            // ✅ 修正・追加部分: spawnArea 内でのランダムな初期位置の計算
            // ----------------------------------------------------------------------

            // X, Y, Z軸それぞれで、spawnAreaのminからmaxの範囲の乱数を生成
            std::uniform_real_distribution<float> distribX(spawnArea.min.x, spawnArea.max.x);
            std::uniform_real_distribution<float> distribY(spawnArea.min.y, spawnArea.max.y);
            std::uniform_real_distribution<float> distribZ(spawnArea.min.z, spawnArea.max.z);

            Vector3 randomOffset = {
                distribX(gen),
                distribY(gen),
                distribZ(gen)
            };

            // エミッター位置にオフセットを加えて初期位置とする
            p.position = emitterPos + randomOffset;

            // ----------------------------------------------------------------------

            p.velocity = direction * speed;

            p.model->Initialize(fngine_->GetD3D12System(), modelData);
            p.model->SetFngine(fngine_);
            p.model->worldTransform_.set_.Translation(p.position);
            p.model->worldTransform_.set_.Scale(startScale);
            p.model->SetColor({ 1.0f,1.0f,1.0f,startAlpha });
            p.model->textureHandle_ = textureHandle;
            particles.push_back(std::move(p));
        }
    }

    // パーティクル更新（省略、変更なし）
    for (auto& p : particles) {
        p.position += p.velocity;
        p.life--;
        // スケールとアルファの補間
        float t = 1.0f - static_cast<float>(p.life) / static_cast<float>(particleLife);
        Easing(p.model->worldTransform_.get_.Scale(), startScale, endScale, t, 1.0f, EASINGTYPE::None);
        Easing(p.model->worldTransform_.get_.Rotation(), startRotation, endRotation, t, 1.0f, EASINGTYPE::None);
        float alpha;
        alpha = Easing_Float(startAlpha, endAlpha, t, 1.0f, EASINGTYPE::None);
        p.model->SetColor({ 1.0f,1.0f,1.0f,alpha });

        p.model->worldTransform_.set_.Translation(p.position);
    }
    // 死んだパーティクルを削除（省略、変更なし）
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0; }),
        particles.end());
    ImGui::Begin("Particle Model Emitter");
    ImGui::Text("Particle Count: %d", static_cast<int>(particles.size()));
    ImGui::End();
}

void ModelParticleEmitter::Draw() {
    for (auto& p : particles) {
        p.model->LocalToWorld();
        p.model->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(p.model->worldTransform_.mat_));
        p.model->Draw();
    }
}