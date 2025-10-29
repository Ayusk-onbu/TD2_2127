#pragma once
#include "Fngine.h"
#include "BulletManager.h"
#include "MapChipField.h"
#include <cassert>
#include <stdint.h>
#include "ModelObject.h"
#include "AABB.h"
#include "LineObject.h"
#include "SpriteObject.h"


class MapChipField;

class Player {
public:
	enum class PlayerState { kGround, kJump, kApexSpin, kFall };
	enum class PlayingState { kPlaying, kGameOver, kStateCount };

	struct CollisionMapInfo {
		bool ceilingCollision = false;
		bool groundCollision = false;
		bool wallCollision = false;
		Vector3 moveVector;
	};

	enum Corner { kRightBottom, kLeftBottom, kRightTop, kLeftTop, kNumCorner };

	void Initialize(ModelObject* model, ModelObject* arrowModel, const Vector3& position, BulletManager* bulletManager,Fngine* fngine);
	void Update();
	void Draw();

	const Vector3& GetVelocity() const { return velocity_; }
	//const WorldTransform& GetWorldTransform() const { return obj_->worldTransform_.GetWorldPos(); }
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	bool IsDead() const { return isDead_; }
	void OnCollision() { isDead_ = true; }
	AABB GetAABB();
	void OnEnemyStomp();
	void ResetAirShot() { canAirShot_ = true; }
	bool IsInAir() const { return state_ != PlayerState::kGround; }
	void ResetAirAction() {
		canAirShot_ = true;
		stompJumpAvailable_ = true;
	}
	void MapCollider(CollisionMapInfo& info);
	void CeilingCollision(CollisionMapInfo& info);
	Vector3 CornerPosition(const Vector3& center, Corner corner);
	void ResultReflectToMove(const CollisionMapInfo& info);
	void OnCeilingCollision(const CollisionMapInfo& info);
	void GroundCollision(CollisionMapInfo& info);
	void RightCollision(CollisionMapInfo& info);
	void LeftCollision(CollisionMapInfo& info);
	void OnWallCollision(const CollisionMapInfo& info);
	bool IsOnGround();

private:
	void StartApexSpin();

	ModelObject* obj_;

	ModelObject* arrowObj_;

	Vector3 velocity_ = {};
	BulletManager* bulletManager_ = nullptr;
	bool isDead_ = false;

	static inline const float kAcceleration = 0.03f;
	static inline const float kAttenuation = 0.02f;
	static inline const float kLimitRunSpeed = 0.5f;
	static inline const float kGravityAcceleration = 0.02f;
	static inline const float kLimitFallSpeed = 0.5f;
	static inline const float kJumpVelocity = 0.6f;
	static inline const float kAirShotRecoil = 0.7f;
	static inline const int kApexSpinDuration = 120;

	static inline const float kTimeTurn = 0.3f;
	static inline const float kBlank = 0.02f;
	static inline const float kAttenuationLanding = 0.05f;
	static inline const float kAttenuationWall = 0.2f;

	enum class LRDirection { kRight, kLeft };
	LRDirection lrdirection_ = LRDirection::kRight;

	float turnFIrstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	PlayerState state_ = PlayerState::kGround;
	PlayingState playingState_ = PlayingState::kPlaying;
	int apexSpinTimer_ = 0;
	bool canAirShot_ = true;// 弾が撃てるかどうか
	MapChipField* mapChipField_ = nullptr;

	static inline const float kWidth = 1.8f;
	static inline const float kHeight = 1.8f;

	bool stompJumpAvailable_ = false;// ジャンプできるかどうか？

	// Hamadaが追加した
	ModelObject gunArrowObj_;

	Vector3 savePoint_ = {};
	float aliveTimer_ = 0.0f;// 復活時間
	float kAliveTime_ = 1.05f;// 復活するための時間

	SpriteObject isGunSprite_;
	WorldTransform isGunSpriteWorldTransform_;

	Fngine* fngine_ = nullptr;
	Audio se_;
};