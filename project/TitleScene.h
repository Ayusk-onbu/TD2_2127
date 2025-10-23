#pragma once



class TitleScene {
public:
	void Initialize();
	Scene Update();
	void Draw();

private:
	uint32_t titleTexture_ = 0;
	uint32_t promptTexture_ = 0;
};