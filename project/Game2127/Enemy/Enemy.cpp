#include "Enemy.h"
#include "CameraSystem.h"
#include "ImGuiManager.h"

void Enemy::Initialize(ModelObject* model, const Vector3& position) {
	assert(model);

	model_ = model;
	model_->worldTransform_.set_.Translation(position);
}

void Enemy::Update() {
	Vector3 pos = model_->worldTransform_.get_.Translation();
	
	model_->worldTransform_.set_.Translation(pos);
	ImGuiManager::GetInstance()->DrawDrag("EnemyPos", model_->worldTransform_.get_.Translation());
}

void Enemy::Draw() { 
	model_->LocalToWorld();
	model_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(model_->worldTransform_.mat_));
	model_->Draw();
}

AABB Enemy::GetAABB() {
	AABB aabb;
	Vector3 worldPos = model_->worldTransform_.GetWorldPos();
	aabb.min = worldPos - kEnemySize * 0.5f;
	aabb.max = worldPos + kEnemySize * 0.5f;
	return aabb;
}