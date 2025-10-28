#pragma once
#include "Fngine.h"
#include "Scene.h"
#include "WorldTransform.h"
#include "Player.h"
#include "EnemyManager.h"
#include "Particle.h"

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
	// タイトルシーンの処理
	void TitleUpdate();
	// ゲームシーンの処理
	void GameUpdate();
	void GenerateBlocks();
private:
	std::unique_ptr<Player>player_ = nullptr;

	std::vector<std::vector<ModelObject*>>blocks_;

	// 3Dモデルデータ
	std::unique_ptr<ModelObject> blockModel_ = nullptr;
	std::unique_ptr<ModelObject> playerModel_ = nullptr;
	std::unique_ptr<ModelObject> bulletModel_ = nullptr;
	std::unique_ptr<ModelObject> enemyModel_ = nullptr;
	std::unique_ptr<ModelObject> arrowModel_ = nullptr;

	// マップチップフィールド
	MapChipField* mapChipField_;

	BulletManager* bulletManager_ = nullptr;
	EnemyManager* enemyManager_ = nullptr;

	ModelParticleEmitter modelEmitter;

	//==== [  ] ====
	// Title Scene についての変数
	//==== ==== ====
	float titleTimer_ = 0.0f;// タイトルの経過時間
	float titleLoopTime_ = 4.0f;// 一タイトルのループ時間
	bool isTitleFirst_ = false;// タイトル初回フラグ
	float titleToGameFadeTimer_ = 1.0f;// タイトルからゲームへのフェード時間
	float titleToGameFadeDuration_ = 2.0f;// タイトルからゲームへのフェード時間の長さ

	// タイトルのカメラ制御
	float titleCameraRadius_ = 30.0f;// タイトルのカメラの半径	

	// タイトルのスプライト


	// ゲームスタートフラグ
	bool isGameStart_ = false;
};



