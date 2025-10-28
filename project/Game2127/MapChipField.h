#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "Fngine.h"

class MapChipField {
public:
	enum MapChipType {
		kBlank,
		kBlock,
		kTrap,
		kDestroiable,
		kMapChipTypeCount
	};

	struct MapChipData {
		std::vector<std::vector<MapChipType>> data;
	};
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};

	struct Rect {
		float left;
		float right;
		float bottom;
		float top;
	};

public:

	MapChipData mapChipData_;

	void ResetMapChipData();

	void LoadMapChipCsv(const std::string& filePath);

	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

	uint32_t GetNumBlockVertical() { return kNumBlockVertical;}

	uint32_t GetNumBlockHorizontal() { return kNumBlockHorizontal; }

	IndexSet GetMapChipIndexSetByPosition(const Vector3& position);

	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

private:
	// 1ブロックのサイズ
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;
	// ブロックの個数
	static inline const uint32_t kNumBlockVertical = 128;
	static inline const uint32_t kNumBlockHorizontal = 21;
};
