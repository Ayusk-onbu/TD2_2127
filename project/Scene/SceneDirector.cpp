#include "SceneDirector.h"
#include "Transition/TransitionHub.h"

SceneDirector::~SceneDirector() {
	delete currentScene_;
}

void SceneDirector::EnqueueImmediateSwitch(Scene* newScene) {
	// ここでは delete しない！ ただ積むだけ
	pendingNext_ = newScene;
	switchQueued_ = true;
}

void SceneDirector::ApplyPendingSwitch_() {
	if (!switchQueued_) return;
	switchQueued_ = false;
	DoImmediateSwitch_(pendingNext_);
	pendingNext_ = nullptr;
}

void SceneDirector::Initialize(Scene& firstScene) {
	currentScene_ = &firstScene;
	currentScene_->FngineSetUp(*p_fngine_);
	currentScene_->Initialize();
	currentScene_->GetSceneDirector(this);

	// フェード管理の初期化（最初のシーン開始時に一度だけ）
	Transition::Setup(*p_fngine_, *this, 1280, 720);
	Transition::SetDelta(1.0f / 60.0f);
}

void SceneDirector::Run() {

	// ★フレーム冒頭で、前フレーム末に積まれた切替を安全に適用
	ApplyPendingSwitch_();

	//while()
	currentScene_->Update();
	currentScene_->Draw();

	// フェード更新・描画
	Transition::TickAndDraw();
}

void SceneDirector::RequestChangeScene(Scene* newScene) {
	if (currentScene_) {
		delete currentScene_;
	}
	currentScene_ = newScene;
	currentScene_->FngineSetUp(*p_fngine_);
	currentScene_->Initialize();
	currentScene_->GetSceneDirector(this);
}

void SceneDirector::DoImmediateSwitch_(Scene* newScene) {
	if (currentScene_) { delete currentScene_; }
	currentScene_ = newScene;
	currentScene_->FngineSetUp(*p_fngine_);
	currentScene_->Initialize();
	currentScene_->GetSceneDirector(this);
}

void SceneDirector::ImmediateSwitchForTransition(Scene* newScene) {
	DoImmediateSwitch_(newScene);
}
