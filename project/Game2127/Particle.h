#pragma once
#include <vector>
#include <memory>
#include "SpriteObject.h"
#include "ModelObject.h"
#include "WorldTransform.h"
#include "AABB.h"
#include <random>
#include <chrono>
#include "Fngine.h"

class SimpleParticleEmitter {
public:
    struct Particle {
        Vector3 position;
        Vector3 velocity;
        int life; // 残り寿命（フレーム数）
        WorldTransform worldTransform;
        std::unique_ptr<SpriteObject> sprite;
    };

    // エミッターの設定
    Vector3 emitterPos = { 0, 0, 0 };
    Vector3 direction = { 0, 1, 0 }; // 流れる方向
    float speed = 1.0f;
    int spawnCountPerFrame = 1; // 1フレームで何個沸くか
    int spawnInterval = 5;      // 何フレームおきに沸くか
    int particleLife = 60;      // パーティクルの寿命（フレーム数）
    int frameCounter = 0;
    Fngine* fngine_ = nullptr;

    std::vector<Particle> particles;

    // パーティクル画像
    int textureHandle = -1;

    void SetEmitter(const Vector3& pos) { emitterPos = pos; }
    void SetDirection(const Vector3& dir) { direction = dir; }
    void SetSpeed(float s) { speed = s; }
    void SetSpawnCount(int count) { spawnCountPerFrame = count; }
    void SetSpawnInterval(int interval) { spawnInterval = interval; }
    void SetParticleLife(int life) { particleLife = life; }
    void SetTexture(int handle) { textureHandle = handle; }
    void SetFngine(Fngine* fngine) { fngine_ = fngine; }

    void Update();

    void Draw();
};

class ModelParticleEmitter {
public:
    // ランダム生成器の準備（Updateごとに初期化するのは非効率的ですが、ここでは簡易的に）
    // 通常はクラスのメンバとして保持します。
    struct Particle {
        Vector3 position;
        Vector3 velocity;
        int life; // 残り寿命（フレーム数）
        std::unique_ptr<ModelObject> model;
    };

    // エミッターの設定
    Vector3 emitterPos = { 0, 0, 0 };
    Vector3 direction = { 0, 1, 0 }; // 流れる方向
    float speed = 1.0f;
    int spawnCountPerFrame = 1; // 1フレームで何個沸くか
    int spawnInterval = 5;      // 何フレームおきに沸くか
    int particleLife = 60;      // パーティクルの寿命（フレーム数）
    int frameCounter = 0;

    float startAlpha = 1.0f;
    float endAlpha = 0.0f;

    Vector3 startScale = {1.0f,1.0f,1.0f};
    Vector3 endScale = { 0.1f,0.1f,0.1f };

	Vector3 startRotation = { 0.0f,0.0f,0.0f };
    Vector3 endRotation = { 0.0f,0.0f,0.0f };

    // 出現範囲
	AABB spawnArea = { {-1.0f,0.0f,-1.0f}, {1.0f,0.0f,1.0f} };

    Fngine* fngine_ = nullptr;

    std::vector<Particle> particles;

    // パーティクル画像
    int textureHandle = -1;
    ModelData modelData;

    void SetEmitter(const Vector3& pos) { emitterPos = pos; }
    void SetDirection(const Vector3& dir) { direction = dir; }
    void SetSpeed(float s) { speed = s; }
    void SetSpawnCount(int count) { spawnCountPerFrame = count; }
    void SetSpawnInterval(int interval) { spawnInterval = interval; }
    void SetParticleLife(int life) { particleLife = life; }
    void SetTexture(int handle) { textureHandle = handle; }
    void SetFngine(Fngine* fngine) { fngine_ = fngine; }
	void SetModelData(const ModelData& data) { modelData = data; }
	void SetStartAlpha(float alpha) { startAlpha = alpha; }
	void SetEndAlpha(float alpha) { endAlpha = alpha; }
	void SetStartScale(const Vector3& scale) { startScale = scale; }
	void SetEndScale(const Vector3& scale) { endScale = scale; }
	void SetSpawnArea(const AABB& area) { spawnArea = area; }
	void SetStartRotation(const Vector3& rotation) { startRotation = rotation; }
	void SetEndRotation(const Vector3& rotation) { endRotation = rotation; }

    void Update();

    void Draw();
};
