#pragma once
#include "Scene.h"
#include "GameScene.h"
#include "TestScene.h"
#include <memory>

class SceneDirector
{
public:
	~SceneDirector();
public:
	void Initialize(Scene& firstScene);
	void Run();
	void RequestChangeScene(Scene* newScene);

	// フェードHubからは “即時” ではなく “遅延実行のエンキュー” を呼ぶ
	void EnqueueImmediateSwitch(Scene* newScene);

	//フェード管理側（TransitionHub）から呼ぶ“即時切替”用
	void ImmediateSwitchForTransition(Scene* newScene);
public:
	void SetUpFngine(Fngine& fngine) { p_fngine_ = &fngine; }
	void ApplyPendingSwitch_();               // ★次フレーム冒頭で実行

private:

	// ★追加：共通の実体（即時切替ロジック）
	void DoImmediateSwitch_(Scene* newScene);

	Fngine* p_fngine_;
	Scene* currentScene_;

	// ★追加：遅延用
	Scene* pendingNext_ = nullptr;
	bool    switchQueued_ = false;
};

