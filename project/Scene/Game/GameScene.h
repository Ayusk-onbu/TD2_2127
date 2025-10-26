#pragma once
#include "Fngine.h"
#include "Scene.h"
#include "WorldTransform.h"
#include "Player.h"
#include "EnemyManager.h"

class GameScene 
	:public Scene
{
public:
	GameScene()=default;
	~GameScene()override;
public:// Sceneにまつわる関数
	void Initialize()override;
	void Update()override;
	void Draw()override;
public:
	
public:
	
public:
	
private:
	void GenerateBlocks();
private:
	std::unique_ptr<Player>player_ = nullptr;

	std::vector<std::vector<ModelObject*>>blocks_;

	// 3Dモデルデータ
	ModelObject* blockModel_ = nullptr;
	ModelObject* playerModel_ = nullptr;
	ModelObject* bulletModel_ = nullptr;
	ModelObject* enemyModel_ = nullptr;
	ModelObject* arrowModel_ = nullptr;

	// マップチップフィールド
	MapChipField* mapChipField_;

	BulletManager* bulletManager_ = nullptr;
	EnemyManager* enemyManager_ = nullptr;
};



