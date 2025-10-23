#pragma once
#include <functional>

class Fngine;
class SceneDirector;
class Scene;

namespace Transition {

	// 追加: トランジション種類
	enum class TransitionType {
		Fade,           // 既存
		FadeSlideLeft,  // 追加
		FadeSlideRight  // 追加
	};

	class Hub {
	public:
		static Hub& I();

		void Setup(Fngine* eng, SceneDirector* dir, int screenW, int screenH);
		void TickAndDraw();
		void SetDelta(float dt);
		void SetTransitionType(TransitionType type);
		void FadeTo(std::function<Scene* ()> factory, float outSec = 0.3f, float inSec = 0.3f);
		void FadeOverlay(float fromAlpha, float toAlpha, float duration);
		void Shutdown(); // 明示解放APIを追加
	private:
		struct Impl;               // ←実体は.cpp
		Impl* impl_;               // or std::unique_ptr<Impl>
		Hub();
		~Hub();
		Hub(const Hub&) = delete;
		Hub& operator=(const Hub&) = delete;
	};

	// “どこでも1行”用の薄いラッパ
	void Setup(Fngine& eng, SceneDirector& dir, int screenW, int screenH);
	void TickAndDraw();
	void SetDelta(float dt);
	void FadeTo(std::function<Scene* ()> factory, float outSec = 0.3f, float inSec = 0.3f);
	void FadeOverlay(float fromAlpha, float toAlpha, float duration);
	void Shutdown();
} // namespace Transition
