#ifndef _SPRITE_EDITOR_H_
#define _SPRITE_EDITOR_H_

#include <vector>
#include <string>
#include <map>
#include "Type.h"
#include "Texture2D.h"
#include "SimpleFileDialog.h"
#include "imgui.h"

struct SpriteFrame {
	std::string name;
	Rect rect;          // x, y, w, h
	Vec2 offset;        // ★ 新增：offsetX, offsetY
	bool selected = false;
	int markSelect = 0; // 0=无标记,1=区间起点,2=区间终点；仅动画预览用，不保存
};


class SpriteEditor {
public:
	SpriteEditor();
	~SpriteEditor();

	void init(SDL_Renderer* renderer);
	void update(float dt);
	void drawUI();

	void loadImage(const std::string& path);
	void loadPlist(const std::string& path);

private:
	void DrawMainMenuBar();
	void DrawLeftPanel();
	void DrawCenterView();
	void DrawRightPanel();
	void DrawPreviewWindow(); // ★ 新增：预览窗口

	void SaveSnapshot();
	void Undo();
	void Redo();
	 
	void PackAtlasToMemory();
	bool SaveAtlasToFile();


	void AutoExtractSprites();
	void AutoPackAtlas();

	SDL_Renderer* _renderer = nullptr;

	Texture2D* _texture = nullptr;
	int _imgW = 0, _imgH = 0;
	unsigned char* _rawPixels = nullptr;

	int _currentMode = 0;

	float _gridW = 64.0f;
	float _gridH = 64.0f;
	bool _snapToGrid = true;

	int _minArea = 50;

	std::vector<SpriteFrame> _frames;
	int _selectedFrameIndex = -1;

	ImVec2 _viewOffset = ImVec2(0, 0);
	float _viewScale = 1.0f;

	struct HistoryState {
		std::vector<SpriteFrame> frames;
	};
	std::vector<HistoryState> _history;
	int _historyIndex = -1;
	void PushState();
	void RestoreState(int index);

	std::string _currentImgPath;
	std::string _plistPath;

	Texture2D* _packedTexture = nullptr;

	// 文件加载模式
	SimpleFileDialog _fileDialog;
	int _loadMode = 0; // 0: 选图片, 1: 选Plist

	// 左右面板宽度
	float _leftPanelWidth = 380.0f;
	float _rightPanelWidth = 300.0f;

	// 预览窗口状态
	bool _showPreviewWindow = false;
	ImVec2 _previewOffset = ImVec2(0, 0);
	float _previewScale = 1.0f;
	std::string _savePngPath;
	std::string _savePlistPath;

	// ★ 动画预览状态
	bool _isPlaying = false;
	float _animFPS = 12.0f;
	float _animTimer = 0.0f;
	int _animCurrentFrame = 0;
	// 动画预览标签页 显示缩放（仅预览，不保存、不导出）
	float _animPreviewScale = 1.0f; 
	bool _animLoop = true;

	// Atlas Result 图集预览：平移偏移 + 缩放
	ImVec2 _atlasPan = ImVec2(0, 0);
	float _atlasZoom = 1.0f;
	int _packW = 0;
	int _packH = 0;

	bool _wantScrollToSelected = false;

	//导出图集文件名前缀(不带后缀)
	char _atlasExportName[128] = "atlas_output";
	//std::string _lastSaveResultTip;

	void RefreshAtlasPreviewIfOpen(); // ★ 新增这一行声明
};

#endif