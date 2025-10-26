#include "Player.h"
#include <algorithm>

#include "ImGuiManager.h"
#include "InputManager.h"
#include "CameraSystem.h"
constexpr float kPi = 3.14159265f;

void Player::Initialize(ModelObject* model, ModelObject* arrowModel, const Vector3& position, BulletManager* bulletManager,Fngine*fngine) {
	assert(model);
	assert(arrowModel);
	assert(bulletManager);

	obj_ = model;
	arrowObj_ = arrowModel;
	bulletManager_ = bulletManager;

	obj_->Initialize(fngine->GetD3D12System(),"Player.obj","resources/Player");
	obj_->worldTransform_.set_.Translation(position);
	obj_->worldTransform_.get_.Rotation().y = Deg2Rad(90);
	obj_->SetFngine(fngine);
	obj_->textureHandle_ = TextureManager::GetInstance()->LoadTexture("resources/Player/player.png");

	arrowObj_->Initialize(fngine->GetD3D12System(), "Player.obj", "resources/Player");
	arrowObj_->worldTransform_.set_.Scale({2.0f,2.0f,2.0f});
	arrowObj_->SetFngine(fngine);
	arrowObj_->textureHandle_ = obj_->textureHandle_;
	playingState_ = PlayingState::kPlaying;
}

void Player::Update() {
	if (isDead_) {
		playingState_ = PlayingState::kGameOver;
		return;
	}

	switch (state_) {
	case PlayerState::kGround:
		velocity_.y = 0;
		if (InputManager::GetJump()) {
			velocity_.y = kJumpVelocity;
			stompJumpAvailable_ = false;
			canAirShot_ = true;
			state_ = PlayerState::kJump;
		}
		if (InputManager::GetRight()) {
			if (velocity_.x < 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			velocity_.x += kAcceleration;
			if (lrdirection_ != LRDirection::kRight) {
				lrdirection_ = LRDirection::kRight;
				turnFIrstRotationY_ = obj_->worldTransform_.get_.Rotation().y;
				turnTimer_ = kTimeTurn;
			}
		} else if (InputManager::GetLeft()) {
			if (velocity_.x > 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			velocity_.x -= kAcceleration;
			if (lrdirection_ != LRDirection::kLeft) {
				lrdirection_ = LRDirection::kLeft;
				turnFIrstRotationY_ = obj_->worldTransform_.get_.Rotation().y;
				turnTimer_ = kTimeTurn;
			}
		} else {
			velocity_.x *= (1.0f - kAttenuation);
		}
		break;

	case PlayerState::kJump:
		if (stompJumpAvailable_ && InputManager::GetJump()) {
			StartApexSpin();
			stompJumpAvailable_ = false;
			break;
		}
		velocity_.y -= kGravityAcceleration;
		if (velocity_.y <= 0.0f) {
			StartApexSpin();
		}
		break;

	case PlayerState::kApexSpin: {
		if (canAirShot_ && InputManager::GetJump()) {
			float angle = obj_->worldTransform_.get_.Rotation().z;
			Vector3 bulletVelocity = {cosf(angle) * 0.8f, sinf(angle) * 0.8f, 0.0f};
			bulletManager_->SpawnBullet(obj_->worldTransform_.get_.Translation(), bulletVelocity, this);
			velocity_.x = -cosf(angle) * kAirShotRecoil;
			velocity_.y = -sinf(angle) * kAirShotRecoil;
			canAirShot_ = false;
			stompJumpAvailable_ = false;
			obj_->worldTransform_.get_.Rotation().z = 0;
			state_ = PlayerState::kFall;
			break;
		}

		apexSpinTimer_--;
		{
			float progress = 1.0f - (static_cast<float>(apexSpinTimer_) / kApexSpinDuration);
			obj_->worldTransform_.get_.Rotation().z = -progress * 2.0f * kPi;
		}
		if (apexSpinTimer_ <= 0) {
			obj_->worldTransform_.get_.Rotation().z = 0;
			state_ = PlayerState::kFall;
		}

		arrowObj_->worldTransform_.get_.Rotation().y = obj_->worldTransform_.get_.Rotation().y;
		arrowObj_->worldTransform_.get_.Rotation().z = obj_->worldTransform_.get_.Rotation().z + kPi;
		float offsetDistance = 2.0f;
		Vector3 offsetDirection = {cosf(arrowObj_->worldTransform_.get_.Rotation().z), sinf(arrowObj_->worldTransform_.get_.Rotation().z), 0.0f};
		arrowObj_->worldTransform_.get_.Translation() = obj_->worldTransform_.get_.Translation() + offsetDirection * offsetDistance;
		arrowObj_->LocalToWorld();

		break;
	}

	case PlayerState::kFall:
		if (stompJumpAvailable_ && InputManager::GetJump()) {
			StartApexSpin();
			stompJumpAvailable_ = false; 
			break;                       
		}

		velocity_.y -= kGravityAcceleration;

		break;
	}

	velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
	velocity_.y = std::clamp(velocity_.y, -kLimitFallSpeed, kJumpVelocity);

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.moveVector = velocity_;

	MapCollider(collisionMapInfo);
	ResultReflectToMove(collisionMapInfo);

	OnCeilingCollision(collisionMapInfo);

	if (state_ == PlayerState::kFall && collisionMapInfo.groundCollision) {
		state_ = PlayerState::kGround;
		stompJumpAvailable_ = false;
	}

	if (state_ == PlayerState::kGround) {
		if (!IsOnGround()) {
			state_ = PlayerState::kFall;
		}
	}
	OnWallCollision(collisionMapInfo);

	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 60.0f;
		float destinationRotationYTable[] = {
		    kPi / 2.0f,
		    kPi * 3.0f / 2.0f,
		};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrdirection_)];
		float progress = 1.0f - (turnTimer_ / kTimeTurn);
		obj_->worldTransform_.get_.Rotation().y = turnFIrstRotationY_ + (destinationRotationY - turnFIrstRotationY_) * progress;
	}

	obj_->LocalToWorld();

	ImGuiManager::GetInstance()->DrawDrag("Player : Pos", obj_->worldTransform_.get_.Translation());
}

void Player::Draw() {
	obj_->LocalToWorld();
	obj_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(obj_->worldTransform_.mat_));
	obj_->Draw();
	if (state_ == PlayerState::kApexSpin) {
		arrowObj_->LocalToWorld();
		arrowObj_->SetWVPData(CameraSystem::GetInstance()->GetActiveCamera()->DrawCamera(arrowObj_->worldTransform_.mat_));
		arrowObj_->Draw();
	}
}

void Player::OnEnemyStomp() {
	velocity_.y = kJumpVelocity * 0.8f;
	ResetAirAction();
	state_ = PlayerState::kJump;
}

void Player::MapCollider(CollisionMapInfo& info) {
	CeilingCollision(info);
	GroundCollision(info);
	LeftCollision(info);
	RightCollision(info);
}

void Player::CeilingCollision(CollisionMapInfo& info) {
	if (info.moveVector.y <= 0.0f) {
		return;
	}
	Vector3 positionsNew[kNumCorner];
	for (int i = 0; i < kNumCorner; ++i) {
		positionsNew[i] = CornerPosition(obj_->worldTransform_.get_.Translation() + info.moveVector, static_cast<Corner>(i));
	}
	bool hit = false;
	float ceilingBlockBottomY = FLT_MAX;
	MapChipField::IndexSet indexSetLeft = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	MapChipField::MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex);
	MapChipField::MapChipType mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex + 1);
	if (mapChipType == MapChipField::MapChipType::kBlock && mapChipTypeNext != MapChipField::MapChipType::kBlock) {
		hit = true;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex);
		ceilingBlockBottomY = (std::min)(ceilingBlockBottomY, blockRect.bottom);
	}
	MapChipField::IndexSet indexSetRight = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex + 1);
	if (mapChipType == MapChipField::MapChipType::kBlock && mapChipTypeNext != MapChipField::MapChipType::kBlock) {
		hit = true;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetRight.xIndex, indexSetRight.yIndex);
		ceilingBlockBottomY = (std::min)(ceilingBlockBottomY, blockRect.bottom);
	}
	if (hit) {
		info.ceilingCollision = true;
		float playerTopY = obj_->worldTransform_.get_.Translation().y + (kHeight / 2.0f);
		float newMoveY = ceilingBlockBottomY - playerTopY;
		info.moveVector.y = (std::max)(0.0f, newMoveY);
	}
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
        {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f},
        {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f},
        {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}
    };
	return center + offsetTable[static_cast<uint32_t>(corner)];
}

void Player::ResultReflectToMove(const CollisionMapInfo& info) { obj_->worldTransform_.get_.Translation() += info.moveVector; }

void Player::OnCeilingCollision(const CollisionMapInfo& info) {
	if (info.ceilingCollision) {
		velocity_.y = 0.0f;
		if (state_ == PlayerState::kJump) {
			StartApexSpin();
		}
	}
}

void Player::GroundCollision(CollisionMapInfo& info) {
	if (info.moveVector.y >= 0.0f) {
		return;
	}
	MapChipField::IndexSet indexSetLeftNow = mapChipField_->GetMapChipIndexSetByPosition(CornerPosition(obj_->worldTransform_.get_.Translation(), kLeftBottom));
	MapChipField::IndexSet indexSetRightNow = mapChipField_->GetMapChipIndexSetByPosition(CornerPosition(obj_->worldTransform_.get_.Translation(), kRightBottom));
	Vector3 positionsNew[kNumCorner];
	for (int i = 0; i < kNumCorner; ++i) {
		positionsNew[i] = CornerPosition(obj_->worldTransform_.get_.Translation() + info.moveVector, static_cast<Corner>(i));
	}
	bool hit = false;
	float groundBlockTopY = -FLT_MAX;
	MapChipField::IndexSet indexSetLeftNew = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	MapChipField::MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetLeftNew.xIndex, indexSetLeftNew.yIndex);
	MapChipField::MapChipType mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetLeftNew.xIndex, indexSetLeftNew.yIndex - 1);
	if (mapChipType == MapChipField::MapChipType::kBlock && mapChipTypeNext != MapChipField::MapChipType::kBlock && indexSetLeftNow.yIndex != indexSetLeftNew.yIndex) {
		hit = true;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetLeftNew.xIndex, indexSetLeftNew.yIndex);
		groundBlockTopY = (std::max)(groundBlockTopY, blockRect.top);
	}
	MapChipField::IndexSet indexSetRightNew = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetRightNew.xIndex, indexSetRightNew.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetRightNew.xIndex, indexSetRightNew.yIndex - 1);
	if (mapChipType == MapChipField::MapChipType::kBlock && mapChipTypeNext != MapChipField::MapChipType::kBlock && indexSetRightNow.yIndex != indexSetRightNew.yIndex) {
		hit = true;
		MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSetRightNew.xIndex, indexSetRightNew.yIndex);
		groundBlockTopY = (std::max)(groundBlockTopY, blockRect.top);
	}
	if (hit) {
		info.groundCollision = true;
		float playerBottomY = obj_->worldTransform_.get_.Translation().y - (kHeight / 2.0f);
		float newMoveY = groundBlockTopY - playerBottomY;
		info.moveVector.y = (std::min)(0.0f, newMoveY);
	}
}

void Player::RightCollision(CollisionMapInfo& info) {
	if (info.moveVector.x <= 0.0f) {
		return;
	}
	const float kCollisionCheckMargin = 0.01f;
	bool hit = false;
	float blockLeftX = FLT_MAX;
	Corner corners[2] = {kRightTop, kRightBottom};
	for (int i = 0; i < 2; ++i) {
		Vector3 checkPos = CornerPosition(obj_->worldTransform_.get_.Translation(), corners[i]);
		checkPos.y += (corners[i] == kRightTop) ? -kCollisionCheckMargin : kCollisionCheckMargin;
		Vector3 checkPosNew = checkPos + info.moveVector;
		MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(checkPosNew);
		MapChipField::MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		MapChipField::MapChipType mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
		if (mapChipType == MapChipField::MapChipType::kBlock && mapChipTypeNext != MapChipField::MapChipType::kBlock) {
			hit = true;
			MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			blockLeftX = (std::min)(blockLeftX, blockRect.left);
		}
	}
	if (hit) {
		info.wallCollision = true;
		float playerRightX = obj_->worldTransform_.get_.Translation().x + (kWidth / 2.0f);
		info.moveVector.x = playerRightX > blockLeftX ? blockLeftX - playerRightX : 0.0f;
	}
}

void Player::LeftCollision(CollisionMapInfo& info) {
	if (info.moveVector.x >= 0.0f) {
		return;
	}
	const float kCollisionCheckMargin = 0.01f;
	bool hit = false;
	float blockRightX = -FLT_MAX;
	Corner corners[2] = {kLeftTop, kLeftBottom};
	for (int i = 0; i < 2; ++i) {
		Vector3 checkPos = CornerPosition(obj_->worldTransform_.get_.Translation(), corners[i]);
		checkPos.y += (corners[i] == kLeftTop) ? -kCollisionCheckMargin : kCollisionCheckMargin;
		Vector3 checkPosNew = checkPos + info.moveVector;
		MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(checkPosNew);
		MapChipField::MapChipType mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
		MapChipField::MapChipType mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
		if (mapChipType == MapChipField::MapChipType::kBlock && mapChipTypeNext != MapChipField::MapChipType::kBlock) {
			hit = true;
			MapChipField::Rect blockRect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			blockRightX = (std::max)(blockRightX, blockRect.right);
		}
	}
	if (hit) {
		info.wallCollision = true;
		float playerLeftX = obj_->worldTransform_.get_.Translation().x - (kWidth / 2.0f);
		info.moveVector.x = playerLeftX < blockRightX ? blockRightX - playerLeftX : 0.0f;
	}
}

void Player::OnWallCollision(const CollisionMapInfo& info) {
	if (info.wallCollision) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

bool Player::IsOnGround() {
	const float kGroundCheckEpsilon = 0.1f;
	Vector3 checkPosOffset = {0.0f, -kGroundCheckEpsilon, 0.0f};
	Vector3 posLeftBottom = CornerPosition(obj_->worldTransform_.get_.Translation() + checkPosOffset, kLeftBottom);
	Vector3 posRightBottom = CornerPosition(obj_->worldTransform_.get_.Translation() + checkPosOffset, kRightBottom);
	MapChipField::IndexSet indexSetLeft = mapChipField_->GetMapChipIndexSetByPosition(posLeftBottom);
	MapChipField::MapChipType typeLeft = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex);
	MapChipField::IndexSet indexSetRight = mapChipField_->GetMapChipIndexSetByPosition(posRightBottom);
	MapChipField::MapChipType typeRight = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex);
	if (typeLeft == MapChipField::MapChipType::kBlock || typeRight == MapChipField::MapChipType::kBlock) {
		return true;
	}
	return false;
}

void Player::StartApexSpin() {
	velocity_ = {};
	apexSpinTimer_ = kApexSpinDuration;
	state_ = PlayerState::kApexSpin;
}

AABB Player::GetAABB() {
	Vector3 size = {kWidth, kHeight, 1.0f};
	Vector3 worldPos = obj_->worldTransform_.get_.Translation();
	return {
	    {worldPos.x - size.x / 2.0f, worldPos.y - size.y / 2.0f, worldPos.z - size.z / 2.0f},
        {worldPos.x + size.x / 2.0f, worldPos.y + size.y / 2.0f, worldPos.z + size.z / 2.0f}
    };
}