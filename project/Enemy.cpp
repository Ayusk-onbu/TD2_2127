#include "Enemy.h"
#include "Matrix4x4.h"
void Enemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model);
	assert(camera);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.set_.Translation(position);
}

void Enemy::Update() {

	// 行列更新
	worldTransform_.LocalToWorld();
}

void Enemy::Draw() { model_->Draw(worldTransform_, *camera_); }

Vector3 Enemy::GetWorldPosition() {
	Vector3 worldPos;
	worldPos = worldTransform_.GetWorldPos();
	return worldPos;
}