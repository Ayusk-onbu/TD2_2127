#pragma once
#include "Scene.h"
#include "SpriteObject.h"
#include "Vector3.h"
#include "Vector4.h"
#include <vector>
#include <string>
#include <optional>
#include "ModelObject.h"
class Fngine;

class StageEditor : public Scene {
public:
    StageEditor();
    ~StageEditor() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void Finalize();

    // ====== 保存 / 読込 ======
    // フォーマット:
    //   1行目: "COLS,ROWS"
    //   2行目以降: ROWS行、各行 COLS 個の整数（カンマ区切り）…タイル
    //   その後:   "#MODELS N"（任意）
    //             N 行: "tileId,cx,cy,rotY"
    bool SaveCSV(const std::string& path) const;
    bool LoadCSV(const std::string& path);

    void BindEngine(Fngine* eng) { eng_ = eng; }

    // 既存の ScreenToCell を「範囲外でも返す版」に差し替える宣言
    bool ScreenToCellUnbounded(int sx, int sy, int& outCx, int& outCy) const;

    // 範囲外セルを要求されたときにグリッドを自動拡張する
    void ExpandToIncludeCell(int cx, int cy);
    bool InBounds(int cx, int cy) const { return (0 <= cx && cx < gridCols_) && (0 <= cy && cy < gridRows_); }
   
private:
    // ===== グリッド表示 =====
    int   gridCols_ = 32;             // 列数
    int   gridRows_ = 18;             // 行数
    float cellSize_ = 40.0f;          // 1セルのピクセル
    float lineThickness_ = 1.0f;      // グリッド線の太さ(px)
    Vector4 gridColor_ = { 0.9f, 0.9f, 0.9f, 0.35f };

    SpriteObject gridLine_;
    int   whiteTexId_ = -1;

    // ===== タイルデータ =====
    // 1次元配列で保持（2D配列は使わない）
    std::vector<int> tiles_;
    inline int IndexOf(int x, int y) const { return y * gridCols_ + x; }

    // ===== モデル配置データ =====
public:
    struct PlacedModel {
        int   tileId;     // 置く種類を簡易にIDで
        int   cx, cy;     // セル座標
        float rotY;       // Y回転（度）
    };
private:
    std::vector<PlacedModel> placedModels_;
    float worldCellSize_ = 1.0f;   // セル→ワールド換算倍率（1.0 = セル1→1.0m）
    float baseY_ = 0.0f;           // 置く高さ（地面Y想定）

    // セル中心をワールド座標に変換（X-Z 平面、Yは baseY_）
    Vector3 CellCenterToWorld(int cx, int cy) const;
    // 置く/消す
    void PlaceModelAtCell(int cx, int cy, int tileId);
    void EraseModelAtCell(int cx, int cy);

    // ===== 編集状態 =====
    int   currentTileId_ = 1;      // 左クリックで置くタイルID
    int   eraseTileId_ = 0;      // 右クリックで消す（＝この値にする）
    bool  paintHold_ = false;  // ドラッグ塗りつぶし用（左）
    bool  eraseHold_ = false;  // ドラッグ消去用（右）

    // ===== 画面→セル変換 =====
    // UI座標（左上0,0 / +X:右 / +Y:下）を想定
    // 画面外は false を返す
    bool ScreenToCell(int sx, int sy, int& outCx, int& outCy) const;

    Fngine* eng_ = nullptr;

    // ===== モデルテンプレート =====
    std::vector<std::unique_ptr<ModelObject>> modelTemplates_;

    // ===== 配置済みモデルの描画用実体 =====
    std::vector<std::unique_ptr<ModelObject>> modelInstances_;

    // StageEditor.h の private に追加
    float defaultScale_ = 0.5f;  // モデルのスケール（等倍=1）

    // class StageEditor の private: に追加
    ImVec2 gridOffset_{ 0.0f, 0.0f };  // グリッドのスクロール（px）

};
