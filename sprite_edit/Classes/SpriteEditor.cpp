#include "SpriteEditor.h"
#include <algorithm>
#include <queue>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>


// ★ 不要在这里定义 STB_IMAGE_IMPLEMENTATION，它在 Texture2D.cpp 已经定义过了
#include "stb_image.h" 

// ★ stb_image_write 没有冲突，可以在这里定义一次用于导出图片
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "tinyxml2.h"

using namespace tinyxml2;

template<typename T>
static inline T Clamp(T v, T minv, T maxv)
{
	if (v < minv) return minv;
	if (v > maxv) return maxv;
	return v;
}

static std::string StripPngSuffix(const std::string& name) {
	const std::string suffix = ".png";
	if (name.size() >= suffix.size() &&
		name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
		return name.substr(0, name.size() - suffix.size());
	}
	return name;
}

SpriteEditor::SpriteEditor() {
	_texture = new Texture2D();
	_packedTexture = new Texture2D();
	strcpy(_atlasExportName, "atlas_output"); //加上这一行
}

SpriteEditor::~SpriteEditor() {
	if (_texture) delete _texture;
	if (_packedTexture) delete _packedTexture;
	if (_rawPixels) stbi_image_free(_rawPixels);
}

void SpriteEditor::init(SDL_Renderer* renderer) {
	_renderer = renderer;
	PushState();
}


// SpriteEditor.cpp 中实现改为：
void SpriteEditor::loadImage(const std::string& path, bool clearExistingFrames) {
	_currentImgPath = path;
	int comp;
	if (_rawPixels) {
		stbi_image_free(_rawPixels);
		_rawPixels = nullptr;
	}

	_rawPixels = stbi_load(path.c_str(), &_imgW, &_imgH, &comp, 4);
	if (!_rawPixels) {
		printf("Failed to load image: %s\nReason: %s\n", path.c_str(), stbi_failure_reason());
		_imgW = _imgH = 0;
		if (_texture) _texture->free();
		if (clearExistingFrames) {
			_frames.clear();
			_selectedFrameIndex = -1;
			_animCurrentFrame = -1;
		}
		RefreshAtlasPreviewIfOpen();
		return;
	}

	_texture->loadRawPixels(_renderer, _rawPixels, _imgW, _imgH);

	if (clearExistingFrames) {
		_frames.clear();
		_selectedFrameIndex = -1;
		_animCurrentFrame = -1;
	}
	RefreshAtlasPreviewIfOpen(); // ★ 加载图片后若预览窗口开着，同步刷新/清空旧图集预览
	PushState();
	printf("Loaded image: %d x %d\n", _imgW, _imgH);
}

void SpriteEditor::loadPlist(const std::string& path) {
	_plistPath = path;
	XMLDocument doc;
	if (doc.LoadFile(path.c_str()) != XML_SUCCESS) return;

	XMLElement* root = doc.FirstChildElement("plist");
	if (!root) return;
	XMLElement* dict = root->FirstChildElement("dict");
	if (!dict) return;

	XMLElement* framesNode = dict->FirstChildElement("key");
	while (framesNode && std::string(framesNode->GetText()) != "frames") {
		framesNode = framesNode->NextSiblingElement("key");
	}
	if (!framesNode) return;

	XMLElement* framesDict = framesNode->NextSiblingElement("dict");
	if (!framesDict) return;

	std::vector<SpriteFrame> parsedFrames; // 先暂存到局部 vector，避免被 loadImage 清空

	for (XMLElement* e = framesDict->FirstChildElement(); e != nullptr; e = e->NextSiblingElement()) {
		if (std::string(e->Value()) == "key") {
			std::string frameName = e->GetText();
			XMLElement* subDict = e->NextSiblingElement("dict");
			if (!subDict) continue;

			XMLElement* texRectNode = subDict->FirstChildElement("key");
			while (texRectNode && std::string(texRectNode->GetText()) != "frame") {
				texRectNode = texRectNode->NextSiblingElement("key");
			}
			if (texRectNode) {
				XMLElement* strNode = texRectNode->NextSiblingElement("string");
				if (strNode) {
					std::string rectStr = strNode->GetText();
					int x, y, w, h;
					if (sscanf(rectStr.c_str(), "{{%d,%d},{%d,%d}}", &x, &y, &w, &h) == 4) {
						SpriteFrame sf;
						sf.name = frameName;
						sf.rect = Rect((float)x, (float)y, (float)w, (float)h);

						XMLElement* offsetNode = subDict->FirstChildElement("key");
						while (offsetNode && std::string(offsetNode->GetText()) != "offset") {
							offsetNode = offsetNode->NextSiblingElement("key");
						}
						if (offsetNode) {
							XMLElement* offStrNode = offsetNode->NextSiblingElement("string");
							if (offStrNode) {
								std::string offStr = offStrNode->GetText();
								int ox, oy;
								if (sscanf(offStr.c_str(), "{%d,%d}", &ox, &oy) == 2) {
									sf.offset = Vec2((float)ox, (float)oy);
								}
							}
						}

						parsedFrames.push_back(sf);
					}
				}
			}
		}
	}

	// ★ 就在这里：循环结束后、PushState() 之前，先加载对应图片，再回填 _frames
	std::string texFileName;
	XMLElement* metaKey = dict->FirstChildElement("key");
	while (metaKey && std::string(metaKey->GetText()) != "metadata") {
		metaKey = metaKey->NextSiblingElement("key");
	}
	if (metaKey) {
		XMLElement* metaDict = metaKey->NextSiblingElement("dict");
		if (metaDict) {
			for (XMLElement* me = metaDict->FirstChildElement(); me != nullptr; me = me->NextSiblingElement()) {
				if (std::string(me->Value()) == "key") {
					std::string k = me->GetText() ? me->GetText() : "";
					XMLElement* valNode = me->NextSiblingElement("string");
					if (valNode && valNode->GetText()) {
						if (k == "realTextureFileName" || (k == "textureFileName" && texFileName.empty())) {
							texFileName = valNode->GetText();
						}
					}
				}
			}
		}
	}

	std::string baseDir = ".";
	size_t slashPos = path.find_last_of("/\\");
	if (slashPos != std::string::npos) {
		baseDir = path.substr(0, slashPos);
	}

	std::string imgPath;
	if (!texFileName.empty()) {
		bool hasDir = (texFileName.find('/') != std::string::npos) || (texFileName.find('\\') != std::string::npos);
		imgPath = hasDir ? texFileName : (baseDir + "/" + texFileName);
	}
	else {
		size_t dotPos = path.find_last_of('.');
		std::string stem = (dotPos != std::string::npos) ? path.substr(0, dotPos) : path;
		imgPath = stem + ".png";
	}

	loadImage(imgPath); // 内部会清空 _frames 并上传贴图
	_frames = std::move(parsedFrames);
	_selectedFrameIndex = _frames.empty() ? -1 : 0;
	_animCurrentFrame = _selectedFrameIndex;

	RefreshAtlasPreviewIfOpen(); // ★ 加载新 plist 后，若预览窗口开着则立刻重打包
	PushState(); 

	printf("Loaded Plist, frames: %d\n", (int)_frames.size());
}

// ======================= UI =======================

void SpriteEditor::drawUI() {


	DrawMainMenuBar();

	if (_fileDialog.Display()) {
		std::string path = _fileDialog.GetFilePath();
		if (_loadMode == 0) {
			loadImage(path);
		}
		else if (_loadMode == 1) {
			loadPlist(path);
		}
	}

	// ★ 绘制独立的预览窗口
	if (_showPreviewWindow) {
		DrawPreviewWindow();
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0, 20));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - 20));
	ImGui::Begin("Sprite Editor Main", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus);



	float totalAvailWidth = ImGui::GetContentRegionAvail().x;
	// ★ 加宽拖拽条：从 12f 增加到 32f，方便手机点击
	float splitterWidth = 32.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	// ================= 左侧面板 =================
	ImGui::BeginChild("Left", ImVec2(_leftPanelWidth, 0), true);
	DrawLeftPanel();
	ImGui::EndChild();

	ImGui::SameLine();

	// ================= 左分隔条 =================
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

	// 画一个带纹理的拖拽条，看起来更直观
	ImVec2 splitterPos = ImGui::GetCursorScreenPos();
	ImGui::Button("##LeftSplitter", ImVec2(splitterWidth, ImGui::GetContentRegionAvail().y));
	ImGui::GetWindowDrawList()->AddLine(
		ImVec2(splitterPos.x + splitterWidth / 2, splitterPos.y + 20),
		ImVec2(splitterPos.x + splitterWidth / 2, splitterPos.y + ImGui::GetContentRegionAvail().y - 20),
		IM_COL32(100, 100, 100, 200), 2.0f
	);

	ImGui::PopStyleColor(3);

	if (ImGui::IsItemActive()) {
		_leftPanelWidth += io.MouseDelta.x;
		if (_leftPanelWidth < 150.0f) _leftPanelWidth = 150.0f;
		if (_leftPanelWidth > totalAvailWidth * 0.6f) _leftPanelWidth = totalAvailWidth * 0.6f;
	}
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

	ImGui::SameLine();

	// ================= 中间画布 =================
	float centerWidth = totalAvailWidth - _leftPanelWidth - _rightPanelWidth - splitterWidth * 2;
	if (centerWidth < 100.0f) centerWidth = 100.0f;

	ImGui::BeginChild("Center", ImVec2(centerWidth, 0), true);
	DrawCenterView();
	ImGui::EndChild();

	ImGui::SameLine();

	// ================= 右分隔条 =================
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

	ImVec2 splitterPosR = ImGui::GetCursorScreenPos();
	ImGui::Button("##RightSplitter", ImVec2(splitterWidth, ImGui::GetContentRegionAvail().y));
	ImGui::GetWindowDrawList()->AddLine(
		ImVec2(splitterPosR.x + splitterWidth / 2, splitterPosR.y + 20),
		ImVec2(splitterPosR.x + splitterWidth / 2, splitterPosR.y + ImGui::GetContentRegionAvail().y - 20),
		IM_COL32(100, 100, 100, 200), 2.0f
	);

	ImGui::PopStyleColor(3);

	if (ImGui::IsItemActive()) {
		_rightPanelWidth -= io.MouseDelta.x;
		if (_rightPanelWidth < 150.0f) _rightPanelWidth = 150.0f;
		if (_rightPanelWidth > totalAvailWidth * 0.6f) _rightPanelWidth = totalAvailWidth * 0.6f;
	}
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

	ImGui::SameLine();

	// ================= 右侧面板 =================
	ImGui::BeginChild("Right", ImVec2(_rightPanelWidth, 0), true);
	DrawRightPanel();
	ImGui::EndChild();

	ImGui::PopStyleVar();

	ImGui::End();
}

void SpriteEditor::DrawMainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			 
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z", false, _historyIndex > 0)) Undo();
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, _historyIndex < (int)_history.size() - 1)) Redo();
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}


void SpriteEditor::DrawCenterView() {
	if (!_texture || !_texture->getTexture()) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Please load an image first.");
		return;
	}
	// ================== ★ 顶部缩放工具栏 ==================
	ImGui::SetNextItemWidth(200.0f);
	ImGui::SliderFloat("Zoom", &_viewScale, 0.1f, 10.0f, "%.2fx");
	ImGui::SameLine();
	if (ImGui::Button("Reset")) {
		_viewScale = 1.0f;
		_viewOffset = ImVec2(10, 10);
	}
	ImGui::Separator();
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImGui::GetContentRegionAvail();
	ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
	bool is_hovered = ImGui::IsItemHovered();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);
	draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(40, 40, 40, 255));
	ImGuiIO& io = ImGui::GetIO();
	// ================== 交互逻辑 ==================
	// 1. 滚轮缩放
	if (is_hovered && io.MouseWheel != 0.0f) {
		float zoom_factor = 1.0f + (io.MouseWheel * 0.1f);
		float new_scale = _viewScale * zoom_factor;
		if (new_scale < 0.1f) new_scale = 0.1f;
		if (new_scale > 10.0f) new_scale = 10.0f;
		float mouse_x = io.MousePos.x - canvas_pos.x;
		float mouse_y = io.MousePos.y - canvas_pos.y;
		float img_x = (mouse_x - _viewOffset.x) / _viewScale;
		float img_y = (mouse_y - _viewOffset.y) / _viewScale;
		_viewScale = new_scale;
		_viewOffset.x = mouse_x - img_x * _viewScale;
		_viewOffset.y = mouse_y - img_y * _viewScale;
	}
	// 2. 拖拽平移
	bool is_dragging_left = ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (io.MouseDragMaxDistanceSqr[0] > 4.0f);
	if (is_hovered && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
		ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
		is_dragging_left)) {
		_viewOffset.x += io.MouseDelta.x;
		_viewOffset.y += io.MouseDelta.y;
	}

	// ==========【重点】统一放到鼠标抬起处理：拾取帧 + 网格新增帧 ==========
	if (is_hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		//只有点击，不是拖拽（拖动距离很小才算点击）
		if (io.MouseDragMaxDistanceSqr[0] < 4.0f)
		{
			// ========== 修复：必须减去 canvas_pos.x / canvas_pos.y ==========
			float localMouseX = io.MousePos.x - canvas_pos.x;
			float localMouseY = io.MousePos.y - canvas_pos.y;

			float mx = (localMouseX - _viewOffset.x) / _viewScale;
			float my = (localMouseY - _viewOffset.y) / _viewScale;

			int hitIndex = -1;
			//遍历所有帧，找鼠标落在哪个rect内部
			for (int i = 0; i < (int)_frames.size(); i++)
			{
				auto& fr = _frames[i];
				if (mx >= fr.rect.x && my >= fr.rect.y
					&& mx < fr.rect.x + fr.rect.w
					&& my < fr.rect.y + fr.rect.h)
				{
					hitIndex = i;
					break;
				}
			}

			if (hitIndex != -1)
			{
				//✅命中已有切片：只做选中，任何模式都生效！不新增帧
				_selectedFrameIndex = hitIndex;
				_animCurrentFrame = hitIndex;
				_wantScrollToSelected = true; //👉 请求下一帧自动滚动列表
			}
			else
			{
				//❌没有命中任何帧，只有Mode0网格模式才允许新建格子
				if (_currentMode == 0)
				{
					float localX = mx;
					float localY = my;
					if (localX >= 0 && localX < _imgW && localY >= 0 && localY < _imgH)
					{
						if (_snapToGrid) {
							localX = floor(localX / _gridW) * _gridW;
							localY = floor(localY / _gridH) * _gridH;
						}
						SpriteFrame newFrame;
						// ★ 修复：改成 frame_ 格式，并补齐 4 位数字
						char nameBuf[64];
						snprintf(nameBuf, sizeof(nameBuf), "frame_%04d.png", (int)_frames.size());
						newFrame.name = nameBuf;
						newFrame.rect = Rect(localX, localY, _snapToGrid ? _gridW : 64, _snapToGrid ? _gridH : 64);
						newFrame.markSelect = 0;
						_frames.push_back(newFrame);
						_selectedFrameIndex = _frames.size() - 1;
						RenumberFrames(); // ★ 新增：点击新建后重排
						PushState();
					}
				}
			}
		}
	}


	// ================== 渲染图像 ==================
	ImVec2 img_pos = ImVec2(canvas_pos.x + _viewOffset.x, canvas_pos.y + _viewOffset.y);
	ImVec2 img_size = ImVec2(_imgW * _viewScale, _imgH * _viewScale);
	draw_list->AddImage((ImTextureID)(intptr_t)_texture->getTexture(), img_pos, ImVec2(img_pos.x + img_size.x, img_pos.y + img_size.y));
	// 画网格
	if (_currentMode == 0) {
		for (float x = 0; x < _imgW; x += _gridW) {
			draw_list->AddLine(ImVec2(img_pos.x + x * _viewScale, img_pos.y), ImVec2(img_pos.x + x * _viewScale, img_pos.y + img_size.y), IM_COL32(255, 255, 255, 50));
		}
		for (float y = 0; y < _imgH; y += _gridH) {
			draw_list->AddLine(ImVec2(img_pos.x, img_pos.y + y * _viewScale), ImVec2(img_pos.x + img_size.x, img_pos.y + y * _viewScale), IM_COL32(255, 255, 255, 50));
		}
	}
	// 画全部帧框
	for (int i = 0; i < _frames.size(); ++i) {
		auto& f = _frames[i];
		ImVec2 rmin = ImVec2(img_pos.x + f.rect.x * _viewScale, img_pos.y + f.rect.y * _viewScale);
		ImVec2 rmax = ImVec2(rmin.x + f.rect.w * _viewScale, rmin.y + f.rect.h * _viewScale);
		ImU32 color = (i == _selectedFrameIndex) ? IM_COL32(0, 255, 255, 255) : IM_COL32(255, 200, 0, 200);
		draw_list->AddRect(rmin, rmax, color, 0.0f, 0, 2.0f);
	}
	draw_list->PopClipRect();
}



void SpriteEditor::update(float dt) {
	if (_isPlaying && _frames.size() > 0)
	{
		int playBeginIdx = 0;
		int playEndIdx = (int)_frames.size() - 1;
		bool hasStart = false, hasEnd = false;
		//扫描找标记
		for (int i = 0; i < (int)_frames.size(); i++)
		{
			if (_frames[i].markSelect == 1)
			{
				playBeginIdx = i;
				hasStart = true;
			}
			if (_frames[i].markSelect == 2)
			{
				playEndIdx = i;
				hasEnd = true;
			}
		}
		if (hasStart && !hasEnd) playEndIdx = (int)_frames.size() - 1;
		if (!hasStart && hasEnd) playBeginIdx = 0;

		//确保 begin <= end
		if (playBeginIdx > playEndIdx)
		{
			std::swap(playBeginIdx, playEndIdx);
		}
		int rangeCount = playEndIdx - playBeginIdx + 1;
		if (rangeCount <= 0) return;

		_animTimer += dt;
		float frameDt = 1.0f / _animFPS;
		if (_animTimer >= frameDt)
		{
			_animTimer -= frameDt;
			_animCurrentFrame += 1;

			if (_animCurrentFrame > playEndIdx)
			{
				if (_animLoop)
				{
					_animCurrentFrame = playBeginIdx;
				}
				else
				{
					_animCurrentFrame = playEndIdx;
					_isPlaying = false;
				}
			}
			// 保护：如果当前帧跑到区间外面
			if (_animCurrentFrame < playBeginIdx || _animCurrentFrame> playEndIdx)
			{
				_animCurrentFrame = playBeginIdx;
			}
		}
	}
}

void SpriteEditor::RefreshAtlasPreviewIfOpen()
{
	if (_showPreviewWindow)
	{
		PackAtlasToMemory();
	}
}
// ======================= 左侧面板：修复按钮高度 =======================

void SpriteEditor::DrawLeftPanel() {
	float availWidth = ImGui::GetContentRegionAvail().x;
	float btnHeight = ImGui::GetFontSize() * 1.5f;

	ImGui::Text("File Management");

	if (ImGui::Button("Load Image", ImVec2(availWidth, btnHeight))) {
		_loadMode = 0;
		_fileDialog.Open("Select Image", ".png,.jpg,.jpeg,.bmp");
	}
	if (ImGui::Button("Load Plist", ImVec2(availWidth, btnHeight))) {
		_loadMode = 1;
		_fileDialog.Open("Select Plist", ".plist");
	}

	ImGui::Separator();
	ImGui::Text("Export Atlas File Name(no ext):");
	ImGui::InputText("##exportname", _atlasExportName, IM_ARRAYSIZE(_atlasExportName));
	ImGui::SameLine();
	ImGui::TextDisabled("(*.png / *.plist)");

	if (ImGui::Button("Save Atlas To File", ImVec2(availWidth, btnHeight))) {
		SaveAtlasToFile();
	}

	ImGui::Separator();

	// 路径显示
	if (!_currentImgPath.empty()) {
		ImGui::TextColored(ImVec4(0, 1, 0, 1), "Input Path:");
		ImGui::TextWrapped("%s", _currentImgPath.c_str());
	}
	else {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "Input: None");
	}

	ImGui::TextColored(ImVec4(0, 1, 0, 1), "Output PNG:");
	ImGui::TextWrapped("%s", _savePngPath.c_str());
	ImGui::TextColored(ImVec4(0, 1, 0, 1), "Output Plist:");
	ImGui::TextWrapped("%s", _savePlistPath.c_str());

	ImGui::Separator();
	ImGui::Text("Image Info: %d x %d", _imgW, _imgH);

	ImGui::Text("Mode Selection");
	ImGui::RadioButton("Grid Cut (Mode 1)", &_currentMode, 0);
	ImGui::RadioButton("Auto Pack (Mode 2)", &_currentMode, 1);

	if (_currentMode == 0) {
		ImGui::PushItemWidth(availWidth * 0.4f);
		ImGui::DragFloat("Grid W", &_gridW, 1.0f, 1.0f, 1000.0f);
		ImGui::DragFloat("Grid H", &_gridH, 1.0f, 1.0f, 1000.0f);
		ImGui::PopItemWidth();

		ImGui::Checkbox("Snap to Grid", &_snapToGrid);

		ImGui::Checkbox("Repack (Compact) on Export", &_repackOnExport);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			RefreshAtlasPreviewIfOpen(); // ★ 勾选/取消勾选时，立刻刷新预览
		}

		if (ImGui::Button("Generate Grid Frames", ImVec2(availWidth, btnHeight)))
		{
			if (_imgW > 0 && _imgH > 0 && _gridW > 0.0f && _gridH > 0.0f)
			{
				_frames.clear();
				_selectedFrameIndex = -1;

				for (float y = 0.0f; y < (float)_imgH; y += _gridH)
				{
					float curH = std::min(_gridH, (float)_imgH - y);
					if (curH <= 0.0f) continue;

					for (float x = 0.0f; x < (float)_imgW; x += _gridW)
					{
						float curW = std::min(_gridW, (float)_imgW - x);
						if (curW <= 0.0f) continue;

						SpriteFrame newFrame;
						char nameBuf[64];
						snprintf(nameBuf, sizeof(nameBuf), "frame_%04d.png", (int)_frames.size());
						newFrame.name = nameBuf;
						newFrame.rect = Rect(x, y, curW, curH);
						newFrame.markSelect = 0;
						_frames.push_back(newFrame);
					}
				}
				_selectedFrameIndex = _frames.empty() ? -1 : 0;
				_animCurrentFrame = _selectedFrameIndex;
				PushState(); 
				RefreshAtlasPreviewIfOpen();
			}
		}

		if (ImGui::Button("Clear Frames", ImVec2(availWidth, btnHeight))) {
			_frames.clear();
			_selectedFrameIndex = -1;
			PushState();
			RefreshAtlasPreviewIfOpen();
		}
	}
	else {
		ImGui::PushItemWidth(availWidth * 0.4f);
		ImGui::InputInt("Min Area", &_minArea);
		ImGui::PopItemWidth();

		ImGui::Checkbox("Repack (Compact) on Export", &_repackOnExport);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			RefreshAtlasPreviewIfOpen(); // ★ 勾选/取消勾选时，立刻刷新预览
		}

		if (ImGui::Button("Extract Regions", ImVec2(availWidth, btnHeight))) {
			AutoExtractSprites();
		}
		if (ImGui::Button("Clear Frames", ImVec2(availWidth, btnHeight))) {
			_frames.clear();
			_selectedFrameIndex = -1;
			PushState();
			RefreshAtlasPreviewIfOpen();
		}
	}

	// ★ 放到这里刚刚好！属于图集操作逻辑
	ImGui::Separator();
	ImGui::Text("Export Options");
	
	ImGui::TextDisabled("Uncheck to keep original image layout.");
}

// ======================= 右侧面板：增加 Offset 编辑 =======================

void SpriteEditor::DrawRightPanel() {
	ImGui::Text("Sprite Frames (%d)", (int)_frames.size());
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), "☑Mark:Start  ▢Mark:End (max two mark)");
	ImGui::Separator();

	ImGui::BeginChild("FrameList", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.4f), true);

	int markDeleteIdx = -1; // 标记要删除的下标

	for (int i = 0; i < (int)_frames.size(); ++i)
	{
		auto& f = _frames[i];
		ImGui::PushID(i);

		//==== 标记勾选框 =====
		int mark = f.markSelect;
		bool bMark = (mark != 0);
		if (ImGui::Checkbox(("##mark" + std::to_string(i)).c_str(), &bMark))
		{
			if (bMark)
			{
				int cntStart = 0, cntEnd = 0;
				for (auto &fr : _frames)
				{
					if (fr.markSelect == 1) cntStart++;
					if (fr.markSelect == 2) cntEnd++;
				}
				if (cntStart == 0)
				{
					f.markSelect = 1;
				}
				else if (cntEnd == 0)
				{
					f.markSelect = 2;
				}
				else
				{
					for (auto &fr : _frames)
					{
						if (fr.markSelect == 2) fr.markSelect = 0;
					}
					f.markSelect = 2;
				}
			}
			else
			{
				f.markSelect = 0;
			}
		}
		ImGui::SameLine();

		// 条目选中
		if (ImGui::Selectable(f.name.c_str(), i == _selectedFrameIndex))
		{
			_selectedFrameIndex = i;
			_animCurrentFrame = i;
			_wantScrollToSelected = true;
		}

		// ========= 新增：需要滚动，且是当前选中行 =========
		if (i == _selectedFrameIndex && _wantScrollToSelected)
		{
			// 0.5f：尽量滚动到窗口垂直中间；0.0f=置顶，1.0f=置底
			ImGui::SetScrollHereY(0.5f);
			_wantScrollToSelected = false; //消耗掉标记，只滚一次
		}

		// ====== 拖拽源 ======
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			ImGui::SetDragDropPayload("FRAME_ITEM", &i, sizeof(int));
			ImGui::TextUnformatted(f.name.c_str());
			ImGui::EndDragDropSource();
		}

		// ====== 拖拽目标：放到当前条目上方 ======
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FRAME_ITEM"))
			{
				int srcIdx = *(const int*)payload->Data;
				int dstIdx = i;
				if (srcIdx != dstIdx)
				{
					SpriteFrame temp = _frames[srcIdx];
					_frames.erase(_frames.begin() + srcIdx);
					_frames.insert(_frames.begin() + dstIdx, temp);

					if (_selectedFrameIndex == srcIdx)
						_selectedFrameIndex = dstIdx;
					else if (srcIdx < _selectedFrameIndex && dstIdx >= _selectedFrameIndex)
						_selectedFrameIndex -= 1;
					else if (srcIdx > _selectedFrameIndex && dstIdx <= _selectedFrameIndex)
						_selectedFrameIndex += 1;

					RenumberFrames(); // ★ 新增：拖拽后重新编排序号
					PushState();
					RefreshAtlasPreviewIfOpen();
				}
			}
			ImGui::EndDragDropTarget();
		}
		 
		// ====== 右键上下文菜单：Rename 在 Delete 上面 ======
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Rename..."))
			{
				_renameTargetIdx = i;
				_renameBatchMode = false;
				_renameStartNum = 0;
				std::string initName = StripPngSuffix(f.name);
				if (initName.empty()) initName = "frame_";
				snprintf(_renameInputBuf, sizeof(_renameInputBuf), "%s", initName.c_str());
				_openRenamePopup = true;
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Delete"))
			{
				markDeleteIdx = i; //只记录标记，不erase、不break
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID(); //每轮循环一定执行PopID，ID栈平衡！
	}


	// ========== 循环结束之后，再执行真正删除 =========
	if (markDeleteIdx != -1)
	{
		int delIdx = markDeleteIdx;
		if (delIdx >= 0 && delIdx < (int)_frames.size())
		{
			// 先保存快照
			PushState();

			_frames.erase(_frames.begin() + delIdx);

			// 同步修正选中、动画索引
			if (_animCurrentFrame > delIdx)
			{
				_animCurrentFrame--;
			}
			else if (_animCurrentFrame == delIdx)
			{
				_animCurrentFrame = _frames.empty() ? -1 : 0;
			}

			if (_selectedFrameIndex == delIdx)
			{
				_selectedFrameIndex = _frames.empty() ? -1 : ((int)_frames.size() - 1);
			}
			else if (_selectedFrameIndex > delIdx)
			{
				_selectedFrameIndex--;
			}
			RenumberFrames(); // ★ 新增：删除后重新编排序号
		}
	}

	ImGui::EndChild();

	if (_openRenamePopup) {
		ImGui::OpenPopup("Rename Frame(s)");
		_openRenamePopup = false;
	}

	if (ImGui::BeginPopupModal("Rename Frame(s)", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::InputText("Name / Prefix", _renameInputBuf, IM_ARRAYSIZE(_renameInputBuf));
		ImGui::Checkbox("Batch Format (auto index)", &_renameBatchMode);

		if (_renameBatchMode) {
			ImGui::InputInt("Start Index", &_renameStartNum);
			if (_renameStartNum < 0) _renameStartNum = 0;
			ImGui::Checkbox("Use Start/End Marks (else from current to end)", &_renameUseMarkedRange);
		}

		ImGui::Separator();
		if (ImGui::Button("OK", ImVec2(100, 0))) {
			ApplyRenameFromPopup();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100, 0))) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	 
	if (ImGui::Button("Add Frame"))
	{
		float newX = 0.0f;
		float newY = 0.0f;
		float newW = (_gridW > 0.0f) ? _gridW : 16.0f;
		float newH = (_gridH > 0.0f) ? _gridH : 16.0f;

		// 如果当前有选中的帧，默认在它右侧/下方错开一点，方便直接拖拽或改坐标
		if (_selectedFrameIndex >= 0 && _selectedFrameIndex < (int)_frames.size())
		{
			const auto& cur = _frames[_selectedFrameIndex].rect;
			newX = cur.x + cur.w;
			newY = cur.y;
			newW = (cur.w > 0.0f) ? cur.w : newW;
			newH = (cur.h > 0.0f) ? cur.h : newH;
		}

		SpriteFrame newFrame;
		char nameBuf[64];
		snprintf(nameBuf, sizeof(nameBuf), "frame_%04d.png", (int)_frames.size());
		newFrame.name = nameBuf;
		newFrame.rect = Rect(newX, newY, newW, newH);
		newFrame.offset = Vec2(0.0f, 0.0f);
		newFrame.markSelect = 0;

		_frames.push_back(newFrame);
		_selectedFrameIndex = (int)_frames.size() - 1;
		_animCurrentFrame = _selectedFrameIndex;
		_wantScrollToSelected = true;

		RenumberFrames(); // ★ 新增：新增后重新编排序号
		PushState();
		RefreshAtlasPreviewIfOpen();
	}
	ImGui::SameLine();
	//=====快捷按钮：清除全部标记=====
	if (ImGui::Button("Clear All Mark"))
	{
		for (auto &fr : _frames) fr.markSelect = 0;
	}

	if (_selectedFrameIndex >= 0 && _selectedFrameIndex < _frames.size()) {
		ImGui::Separator();
		ImGui::Text("Edit Frame:");
		auto& r = _frames[_selectedFrameIndex].rect;
		auto& o = _frames[_selectedFrameIndex].offset;

		bool changed = false;
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);

		changed |= ImGui::DragFloat("X", &r.x, 1.0f);
		changed |= ImGui::DragFloat("Y", &r.y, 1.0f);
		changed |= ImGui::DragFloat("W", &r.w, 1.0f);
		changed |= ImGui::DragFloat("H", &r.h, 1.0f);

		ImGui::Separator();
		changed |= ImGui::DragFloat("Offset X", &o.x, 1.0f);
		changed |= ImGui::DragFloat("Offset Y", &o.y, 1.0f);

		ImGui::PopItemWidth();

		if (ImGui::IsItemDeactivatedAfterEdit() && changed) {
			PushState();
		}
	}

	ImGui::Separator();
	if (ImGui::Button(_showPreviewWindow ? "Close Preview" : "Open Preview", ImVec2(-1, ImGui::GetFontSize() * 1.5f))) {
		if (_showPreviewWindow)
		{
			// 当前是打开状态 → 关闭
			_showPreviewWindow = false;
		}
		else
		{
			// 当前是关闭状态 → 打开，打包
			PackAtlasToMemory();
			_animPreviewScale = 1.0f;
			_showPreviewWindow = true;
		}
	}


}



// ======================= 独立预览窗口（包含动画） =======================

void SpriteEditor::DrawPreviewWindow() {
	//ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Preview Window", &_showPreviewWindow)) {

		if (ImGui::BeginTabBar("PreviewTabs")) {

			// Tab 1: 图集重组结果
			if (ImGui::BeginTabItem("Atlas Result"))
			{
				if (_packedTexture && _packedTexture->getTexture() && _packW > 0 && _packH > 0)
				{
					ImVec2 canvasPos = ImGui::GetCursorScreenPos();
					ImVec2 canvasSize = ImGui::GetContentRegionAvail();
					ImGui::InvisibleButton("atlas_canvas", canvasSize);
					bool hover = ImGui::IsItemHovered();
					ImGuiIO& io = ImGui::GetIO();

					// 左键拖拽平移
					if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f) && hover)
					{
						_atlasPan.x += io.MouseDelta.x;
						_atlasPan.y += io.MouseDelta.y;
					}

					// 滚轮缩放（鼠标位置为中心缩放）
					if (hover && io.MouseWheel != 0)
					{
						ImVec2 mouseInCanvas = ImVec2(io.MousePos.x - canvasPos.x, io.MousePos.y - canvasPos.y);
						float zoomOld = _atlasZoom;
						float zoomStep = 0.1f;
						_atlasZoom += io.MouseWheel * zoomStep;
						if (_atlasZoom < 0.1f) _atlasZoom = 0.1f;
						if (_atlasZoom > 5.0f) _atlasZoom = 5.0f;

						_atlasPan.x -= mouseInCanvas.x * (_atlasZoom / zoomOld - 1.0f);
						_atlasPan.y -= mouseInCanvas.y * (_atlasZoom / zoomOld - 1.0f);
					}

					ImGui::Text("Atlas Preview | Zoom: %.2fx", _atlasZoom);
					ImGui::SameLine();
					if (ImGui::Button("Reset View"))
					{
						_atlasPan = ImVec2(0, 0);
						_atlasZoom = 1.0f;
					}

					ImDrawList* dl = ImGui::GetWindowDrawList();
					dl->PushClipRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), true);
					dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(30, 30, 30, 255));

					float texW = (float)_packW;
					float texH = (float)_packH;
					float dispW = texW * _atlasZoom;
					float dispH = texH * _atlasZoom;

					ImVec2 drawPos = ImVec2(
						canvasPos.x + (canvasSize.x - dispW) * 0.5f + _atlasPan.x,
						canvasPos.y + (canvasSize.y - dispH) * 0.5f + _atlasPan.y
					);

					ImVec2 uv0(0, 0);
					ImVec2 uv1(1, 1);
					dl->AddImage(
						(ImTextureID)(intptr_t)_packedTexture->getTexture(),
						drawPos,
						ImVec2(drawPos.x + dispW, drawPos.y + dispH),
						uv0, uv1
					);

					dl->PopClipRect();
				}
				else
				{
					ImGui::TextDisabled("Build atlas first.");
				}
				ImGui::EndTabItem();
			}


			// Tab 2: 动画播放预览
			// Tab 2: 动画播放预览
			if (ImGui::BeginTabItem("Animation")) {
				if (_texture && _texture->getTexture() && !_frames.empty()) {
					int frameTotal = (int)_frames.size();

					// ★ 防止 _animCurrentFrame 为 -1 或越界导致 vector subscript out of range
					if (_animCurrentFrame < 0 || _animCurrentFrame >= frameTotal) {
						_animCurrentFrame = 0;
					}

					//运行时扫描标记，得到播放区间（UI显示提示用）
					int playBeginIdx = 0, playEndIdx = frameTotal - 1;
					bool hasStart = false, hasEnd = false;
					for (int i = 0; i < frameTotal; i++)
					{
						if (_frames[i].markSelect == 1) { playBeginIdx = i; hasStart = true; }
						if (_frames[i].markSelect == 2) { playEndIdx = i; hasEnd = true; }
					}
					if (hasStart && !hasEnd) playEndIdx = frameTotal - 1;
					if (!hasStart && hasEnd) playBeginIdx = 0;
					if (playBeginIdx > playEndIdx) std::swap(playBeginIdx, playEndIdx);

					// =========播放控制栏 =========
					if (ImGui::Button(_isPlaying ? "Pause" : "Play")) {
						_isPlaying = !_isPlaying;
						if (_isPlaying)
						{
							//点击播放，跳到区间起点
							_animCurrentFrame = playBeginIdx;
						}
					}
					ImGui::SameLine();
					ImGui::SetNextItemWidth(100.0f);
					ImGui::SliderFloat("FPS", &_animFPS, 1.0f, 60.0f, "%.0f");

					ImGui::SameLine();
					ImGui::SetNextItemWidth(100.0f);
					ImGui::SliderFloat("Zoom", &_animPreviewScale, 0.1f, 5.0f, "%.2fx");

					ImGui::SameLine();
					if (ImGui::Button("ResetZoom"))
					{
						_animPreviewScale = 1.0f;
					}
					ImGui::SameLine();
					ImGui::Checkbox("Loop", &_animLoop);

					//显示当前识别的区间提示
					if (hasStart || hasEnd)
					{
						ImGui::TextColored(ImVec4(0, 1, 1, 1), "Play range: %d ~ %d", playBeginIdx, playEndIdx);
					}
					else
					{
						ImGui::TextDisabled("Play range: All frames (no mark)");
					}
					ImGui::Text("Frame:%d/%d", _animCurrentFrame + 1, frameTotal);

					ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
					ImVec2 canvas_size = ImGui::GetContentRegionAvail();
					ImGui::InvisibleButton("anim_canvas", canvas_size);
					bool hoverAnimCanvas = ImGui::IsItemHovered();
					ImGuiIO& io = ImGui::GetIO();
					//鼠标滚轮缩放动画预览画布
					if (hoverAnimCanvas && io.MouseWheel != 0)
					{
						float zoomFac = 1.0f + io.MouseWheel * 0.1f;
						_animPreviewScale *= zoomFac;
						_animPreviewScale = Clamp(_animPreviewScale, 0.1f, 5.0f);
					}

					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					draw_list->PushClipRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), true);

					//井字格参考背景
					draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(40, 40, 40, 255));
					ImVec2 center = ImVec2(canvas_pos.x + canvas_size.x / 2, canvas_pos.y + canvas_size.y / 2);
					draw_list->AddLine(ImVec2(center.x, canvas_pos.y), ImVec2(center.x, canvas_pos.y + canvas_size.y), IM_COL32(255, 255, 255, 30), 2.0f);
					draw_list->AddLine(ImVec2(canvas_pos.x, center.y), ImVec2(canvas_pos.x + canvas_size.x, center.y), IM_COL32(255, 255, 255, 30), 2.0f);

					//渲染当前帧
					Rect& r = _frames[_animCurrentFrame].rect;
					Vec2& off = _frames[_animCurrentFrame].offset;

					ImVec2 uv0((float)r.x / _imgW, (float)r.y / _imgH);
					ImVec2 uv1((float)(r.x + r.w) / _imgW, (float)(r.y + r.h) / _imgH);

					float dispW = r.w * _animPreviewScale;
					float dispH = r.h * _animPreviewScale;
					ImVec2 draw_pos = ImVec2(
						center.x - dispW / 2.0f + off.x * _animPreviewScale,
						center.y - dispH / 2.0f + off.y * _animPreviewScale
					);

					draw_list->AddImage(
						(ImTextureID)(intptr_t)_texture->getTexture(),
						draw_pos,
						ImVec2(draw_pos.x + dispW, draw_pos.y + dispH),
						uv0, uv1
					);
					draw_list->PopClipRect();
				}
				else {
					ImGui::Text("Load an image and add frames first.");
				}
				ImGui::EndTabItem();
			}


			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

// ======================= 算法 =======================

void SpriteEditor::AutoExtractSprites() {
	if (!_rawPixels) return;
	_frames.clear();

	std::vector<bool> visited(_imgW * _imgH, false);
	int dx[] = { -1, 1, 0, 0 };
	int dy[] = { 0, 0, -1, 1 };

	for (int y = 0; y < _imgH; ++y) {
		for (int x = 0; x < _imgW; ++x) {
			int idx = y * _imgW + x;
			if (!visited[idx] && _rawPixels[idx * 4 + 3] > 10) {
				std::queue<std::pair<int, int>> q;
				q.push({ x, y });
				visited[idx] = true;

				int minX = x, maxX = x, minY = y, maxY = y;
				int area = 0;

				while (!q.empty()) {
					auto cx = q.front().first, cy  = q.front().second; q.pop();
					area++;
					minX = std::min(minX, cx); maxX = std::max(maxX, cx);
					minY = std::min(minY, cy); maxY = std::max(maxY, cy);

					for (int i = 0; i < 4; ++i) {
						int nx = cx + dx[i], ny = cy + dy[i];
						if (nx >= 0 && nx < _imgW && ny >= 0 && ny < _imgH) {
							int nidx = ny * _imgW + nx;
							if (!visited[nidx] && _rawPixels[nidx * 4 + 3] > 10) {
								visited[nidx] = true;
								q.push({ nx, ny });
							}
						}
					}
				}

				if (area >= _minArea) {
					SpriteFrame sf;
					// ★ 修复：改成 frame_ 格式，并补齐 4 位数字
					char nameBuf[64];
					snprintf(nameBuf, sizeof(nameBuf), "frame_%04d.png", (int)_frames.size());
					sf.name = nameBuf;
					sf.rect = Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
					_frames.push_back(sf);
				}
			}
		}
	}
	RenumberFrames(); // ★ 新增
	PushState();
	RefreshAtlasPreviewIfOpen();
}

struct RectNode {
	int id, w, h;
	bool operator<(const RectNode& o) const { return h > o.h; }
};

void SpriteEditor::AutoPackAtlas() {
	if (_frames.empty() || !_rawPixels) return;

	std::vector<RectNode> nodes;
	for (int i = 0; i < _frames.size(); ++i) {
		nodes.push_back({ i, (int)_frames[i].rect.w, (int)_frames[i].rect.h });
	}
	//std::sort(nodes.begin(), nodes.end());

	// 动态计算最大宽度：取 512 和 所有图块宽度之和 的较小值
	int maxW = 512;
	int sumW = 0;
	for (auto& f : _frames) sumW += (int)f.rect.w;
	if (sumW < maxW) maxW = sumW;
	if (maxW < 64) maxW = 64; // 兜底，别小于64

	int curX = 0, curY = 0, rowH = 0;
	std::vector<Rect> packedRects(nodes.size());

	for (auto& n : nodes) {
		if (curX + n.w > maxW) {
			curX = 0;
			curY += rowH;
			rowH = 0;
		}
		packedRects[n.id] = Rect(curX, curY, n.w, n.h);
		curX += n.w;
		rowH = std::max(rowH, n.h);
	}
	int totalH = curY + rowH;

	std::vector<unsigned char> canvas(maxW * totalH * 4, 0);
	for (int i = 0; i < _frames.size(); ++i) {
		Rect src = _frames[i].rect;
		Rect dst = packedRects[i];
		for (int y = 0; y < src.h; ++y) {
			for (int x = 0; x < src.w; ++x) {
				int srcIdx = ((int)src.y + y) * _imgW + ((int)src.x + x);
				int dstIdx = ((int)dst.y + y) * maxW + ((int)dst.x + x);
				for (int c = 0; c < 4; ++c) canvas[dstIdx * 4 + c] = _rawPixels[srcIdx * 4 + c];
			}
		}
	}

	// ... 前面生成 canvas 画布的代码不变 ...

	_packedTexture->loadRawPixels(_renderer, canvas.data(), maxW, totalH);

	// 决定路径 (优先存到源图片的目录)
	std::string baseDir = ".";
	if (!_currentImgPath.empty()) {
		size_t pos = _currentImgPath.find_last_of("/\\");
		if (pos != std::string::npos) baseDir = _currentImgPath.substr(0, pos);
	}

	_savePngPath = baseDir + "/atlas_output.png";
	_savePlistPath = baseDir + "/atlas_output.plist";

	stbi_write_png(_savePngPath.c_str(), maxW, totalH, 4, canvas.data(), maxW * 4);

	std::ofstream out(_savePlistPath);


	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
	out << "<plist version=\"1.0\">\n<dict>\n    <key>frames</key>\n    <dict>\n";
	for (int i = 0; i < _frames.size(); ++i) {
		out << "        <key>" << _frames[i].name << "</key>\n";
		out << "        <dict>\n";
		out << "            <key>textureRect</key>\n";
		out << "            <string>{{" << packedRects[i].x << "," << packedRects[i].y << "},{"
			<< packedRects[i].w << "," << packedRects[i].h << "}}</string>\n";
		out << "        </dict>\n";
	}
	out << "    </dict>\n</dict>\n</plist>\n";

	// ... 写入 plist 的逻辑保持不变 ...
	out.close();
	 
	printf("Exported to %s and %s\n", _savePngPath.c_str(), _savePlistPath.c_str());


}

// ======================= 撤销重做 =======================

void SpriteEditor::PushState() {
	HistoryState state;
	state.frames = _frames;
	if (_historyIndex < (int)_history.size() - 1) {
		_history.erase(_history.begin() + _historyIndex + 1, _history.end());
	}
	_history.push_back(state);
	_historyIndex++;
}

void SpriteEditor::Undo() {
	if (_historyIndex > 0) {
		_historyIndex--;
		RestoreState(_historyIndex);
	}
}

void SpriteEditor::Redo() {
	if (_historyIndex < (int)_history.size() - 1) {
		_historyIndex++;
		RestoreState(_historyIndex);
	}
}

void SpriteEditor::RestoreState(int index) {
	if (index >= 0 && index < _history.size()) {
		_frames = _history[index].frames;
		_selectedFrameIndex = -1;
		_animCurrentFrame = -1;
		_wantScrollToSelected = false;
		RenumberFrames(); // ★ 新增：撤销/重做后重排
	}
}
void SpriteEditor::PackAtlasToMemory()
{
	if (_frames.empty() || !_rawPixels || _imgW <= 0 || _imgH <= 0) {
		if (_packedTexture) _packedTexture->free();
		_packW = 0;
		_packH = 0;
		return;
	}

	// ★ 新增逻辑：如果未勾选重排，预览直接使用原图
	if (!_repackOnExport) {
		_packedTexture->loadRawPixels(_renderer, _rawPixels, _imgW, _imgH);
		_packW = _imgW;
		_packH = _imgH;
		return;
	}

	// ==== 下面是勾选重排（Mode 1）的打包逻辑 ====
	std::vector<RectNode> nodes;
	for (int i = 0; i < (int)_frames.size(); ++i) {
		nodes.push_back({ i, (int)_frames[i].rect.w, (int)_frames[i].rect.h });
	}

	// 动态计算最大宽度
	int maxW = 512;
	int sumW = 0;
	for (auto& f : _frames) sumW += (int)f.rect.w;
	if (sumW < maxW) maxW = sumW;
	if (maxW < 64) maxW = 64;

	int curX = 0, curY = 0, rowH = 0;
	std::vector<Rect> packedRects(nodes.size());
	for (auto& n : nodes) {
		if (curX + n.w > maxW) {
			curX = 0;
			curY += rowH;
			rowH = 0;
		}
		packedRects[n.id] = Rect(curX, curY, n.w, n.h);
		curX += n.w;
		rowH = std::max(rowH, n.h);
	}
	int totalH = curY + rowH;
	if (totalH <= 0) {
		if (_packedTexture) _packedTexture->free();
		_packW = 0;
		_packH = 0;
		return;
	}

	std::vector<unsigned char> canvas((size_t)maxW * (size_t)totalH * 4, 0);
	for (int i = 0; i < (int)_frames.size(); ++i) {
		Rect src = _frames[i].rect;
		Rect dst = packedRects[i];
		int copyW = (int)src.w;
		int copyH = (int)src.h;
		for (int y = 0; y < copyH; ++y) {
			int srcY = (int)src.y + y;
			int dstY = (int)dst.y + y;
			if (srcY < 0 || srcY >= _imgH || dstY < 0 || dstY >= totalH) continue;

			for (int x = 0; x < copyW; ++x) {
				int srcX = (int)src.x + x;
				int dstX = (int)dst.x + x;
				if (srcX < 0 || srcX >= _imgW || dstX < 0 || dstX >= maxW) continue;

				size_t srcIdx = ((size_t)srcY * (size_t)_imgW + (size_t)srcX) * 4;
				size_t dstIdx = ((size_t)dstY * (size_t)maxW + (size_t)dstX) * 4;
				for (int c = 0; c < 4; ++c) {
					canvas[dstIdx + c] = _rawPixels[srcIdx + c];
				}
			}
		}
	}
	_packedTexture->loadRawPixels(_renderer, canvas.data(), maxW, totalH);
	_packW = maxW;
	_packH = totalH;
}
//【保存已经打包好的数据到磁盘，返回true成功】
bool SpriteEditor::SaveAtlasToFile()
{
	if (_atlasExportName[0] == '\0') strcpy(_atlasExportName, "atlas_output");
	if (_frames.empty() || !_rawPixels) {
		printf("Error: Nothing to export.\n");
		return false;
	}

	std::string baseDir = ".";
	if (!_currentImgPath.empty()) {
		size_t pos = _currentImgPath.find_last_of("/\\");
		if (pos != std::string::npos) baseDir = _currentImgPath.substr(0, pos);
	}

	std::string pngName = std::string(_atlasExportName) + ".png";
	std::string plistName = std::string(_atlasExportName) + ".plist";
	_savePngPath = baseDir + "/" + pngName;
	_savePlistPath = baseDir + "/" + plistName;

	std::vector<Rect> packedRects(_frames.size());

	if (_repackOnExport) {
		// ==== 模式1：重新紧凑拼装 ====
		std::vector<RectNode> nodes;
		for (int i = 0; i < _frames.size(); ++i) nodes.push_back({ i, (int)_frames[i].rect.w, (int)_frames[i].rect.h });

		// ★ 修复：动态计算 maxW，与内存预览逻辑保持一致
		int maxW = 512;
		int sumW = 0;
		for (auto& f : _frames) sumW += (int)f.rect.w;
		if (sumW < maxW) maxW = sumW;
		if (maxW < 64) maxW = 64;

		int curX = 0, curY = 0, rowH = 0;
		for (auto& n : nodes) {
			if (curX + n.w > maxW) { curX = 0; curY += rowH; rowH = 0; }
			packedRects[n.id] = Rect(curX, curY, n.w, n.h);
			curX += n.w;
			rowH = std::max(rowH, n.h);
		}
		_packW = maxW;
		_packH = curY + rowH;

		std::vector<unsigned char> canvas((size_t)_packW * _packH * 4, 0);
		for (int i = 0; i < _frames.size(); ++i) {
			Rect src = _frames[i].rect;
			Rect dst = packedRects[i];
			for (int y = 0; y < src.h; ++y) {
				for (int x = 0; x < src.w; ++x) {
					size_t srcIdx = ((size_t)src.y + y) * _imgW + ((size_t)src.x + x);
					size_t dstIdx = ((size_t)dst.y + y) * _packW + ((size_t)dst.x + x);
					for (int c = 0; c < 4; ++c) canvas[dstIdx * 4 + c] = _rawPixels[srcIdx * 4 + c];
				}
			}
		}
		stbi_write_png(_savePngPath.c_str(), _packW, _packH, 4, canvas.data(), _packW * 4);
		_packedTexture->loadRawPixels(_renderer, canvas.data(), _packW, _packH);

	}
	else {
		// ==== 模式2：不重新拼装 ====
		for (int i = 0; i < _frames.size(); ++i) {
			packedRects[i] = _frames[i].rect;
		}
		_packW = _imgW;
		_packH = _imgH;

		stbi_write_png(_savePngPath.c_str(), _imgW, _imgH, 4, _rawPixels, _imgW * 4);
		if (_packedTexture) _packedTexture->loadRawPixels(_renderer, _rawPixels, _imgW, _imgH);
	}

	EnsureUniqueFrameNamesForExport();
	// ==== 写 plist ====
	std::ofstream out(_savePlistPath);
	out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	out << "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
	out << "<plist version=\"1.0\">\n";
	out << "<dict>\n";
	out << "    <key>frames</key>\n";
	out << "    <dict>\n";

	for (int i = 0; i < _frames.size(); ++i) {
		Rect srcFrame = _frames[i].rect;
		Rect dst = packedRects[i];
		Vec2 off = _frames[i].offset;

		out << "        <key>" << _frames[i].name << "</key>\n";
		out << "        <dict>\n";
		out << "            <key>frame</key>\n";
		out << "            <string>{{" << (int)dst.x << "," << (int)dst.y << "},{" << (int)dst.w << "," << (int)dst.h << "}}</string>\n";
		out << "            <key>offset</key>\n";
		out << "            <string>{" << (int)off.x << "," << (int)off.y << "}</string>\n";
		out << "            <key>rotated</key>\n";
		out << "            <false/>\n";
		out << "            <key>sourceColorRect</key>\n";
		out << "            <string>{{0,0},{" << (int)srcFrame.w << "," << (int)srcFrame.h << "}}</string>\n";
		out << "            <key>sourceSize</key>\n";
		out << "            <string>{" << (int)srcFrame.w << "," << (int)srcFrame.h << "}</string>\n";
		out << "        </dict>\n";
	}
	out << "    </dict>\n";
	out << "    <key>metadata</key>\n";
	out << "    <dict>\n";
	out << "        <key>format</key>\n";
	out << "        <integer>2</integer>\n";
	out << "        <key>realTextureFileName</key>\n";
	out << "        <string>" << pngName << "</string>\n";
	out << "        <key>size</key>\n";
	out << "        <string>{" << _packW << "," << _packH << "}</string>\n";
	out << "        <key>smartupdate</key>\n";
	out << "        <string>$TexturePacker:SmartUpdate:00000000000000000000000000000000$</string>\n";
	out << "        <key>textureFileName</key>\n";
	out << "        <string>" << pngName << "</string>\n";
	out << "    </dict>\n";
	out << "</dict>\n";
	out << "</plist>\n";
	out.close();

	printf("Saved: Png=%s  Plist=%s\n", _savePngPath.c_str(), _savePlistPath.c_str());
	return true;
}

void SpriteEditor::RenumberFrames() {
	for (int i = 0; i < (int)_frames.size(); ++i) {
		char nameBuf[64];
		snprintf(nameBuf, sizeof(nameBuf), "frame_%04d.png", i);
		_frames[i].name = nameBuf;
	}
}
 


// 1. 剥离末尾 ".png" 和末尾 "_数字"，还原纯前缀（如 "aa_0005.png" -> "aa_"）
static std::string ExtractCleanPrefix(const std::string& rawInput) {
	const std::string suffix = ".png";
	std::string s = rawInput;
	if (s.size() >= suffix.size() &&
		s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0) {
		s = s.substr(0, s.size() - suffix.size());
	}
	if (s.empty()) return "frame_";

	size_t underscorePos = s.find_last_of('_');
	if (underscorePos != std::string::npos && underscorePos + 1 < s.size()) {
		bool allDigits = true;
		for (size_t i = underscorePos + 1; i < s.size(); ++i) {
			if (!isdigit((unsigned char)s[i])) {
				allDigits = false;
				break;
			}
		}
		if (allDigits) {
			s = s.substr(0, underscorePos + 1);
		}
	}

	if (!s.empty() && s.back() != '_') {
		s += "_";
	}
	return s;
}

static std::string FormatIndexedName(const std::string& prefix, int index) {
	char buf[192];
	snprintf(buf, sizeof(buf), "%s%04d.png", prefix.c_str(), std::max(0, index));
	return std::string(buf);
}

void SpriteEditor::MakeUniqueFrameName(std::string& candidate, int selfIdx) const {
	std::string prefix = ExtractCleanPrefix(candidate);

	auto hasNameConflict = [&](const std::string& testName) -> bool {
		for (int i = 0; i < (int)_frames.size(); ++i) {
			if (i == selfIdx) continue;
			if (_frames[i].name == testName) return true;
		}
		return false;
	};

	int seq = 0;
	while (true) {
		std::string trial = FormatIndexedName(prefix, seq++);
		if (!hasNameConflict(trial)) {
			candidate = trial;
			return;
		}
	}
}

void SpriteEditor::ApplyRenameFromPopup() {
	if (_frames.empty() || _renameTargetIdx < 0 || _renameTargetIdx >= (int)_frames.size()) return;

	if (!_renameBatchMode) {
		std::string nameStr = _renameInputBuf;
		if (nameStr.empty()) nameStr = "frame_";
		MakeUniqueFrameName(nameStr, _renameTargetIdx);
		_frames[_renameTargetIdx].name = nameStr;
	}
	else {
		int total = (int)_frames.size();
		int curIdx = Clamp(_renameTargetIdx, 0, total - 1);
		int beginIdx = curIdx;
		int endIdx = total - 1;

		if (_renameUseMarkedRange) {
			int startMarkIdx = -1;
			int endMarkIdx = -1;
			for (int i = 0; i < total; ++i) {
				if (_frames[i].markSelect == 1) startMarkIdx = i;
				if (_frames[i].markSelect == 2) endMarkIdx = i;
			}

			if (startMarkIdx != -1 && endMarkIdx != -1) {
				beginIdx = std::min(startMarkIdx, endMarkIdx);
				endIdx = std::max(startMarkIdx, endMarkIdx);
			}
			else if (startMarkIdx != -1) {
				beginIdx = startMarkIdx;
				endIdx = std::max(startMarkIdx, curIdx);
			}
			else if (endMarkIdx != -1) {
				beginIdx = std::min(curIdx, endMarkIdx);
				endIdx = endMarkIdx;
			}
		}

		std::string prefix = ExtractCleanPrefix(_renameInputBuf);

			int maxUsedNum = -1;
		for (int i = 0; i < total; ++i) {
			if (i >= beginIdx && i <= endIdx) continue;
			const std::string& n = _frames[i].name;
			if (n.compare(0, prefix.size(), prefix) == 0) {
				std::string rest = n.substr(prefix.size());
				const std::string pngExt = ".png";
				if (rest.size() >= pngExt.size() &&
					rest.compare(rest.size() - pngExt.size(), pngExt.size(), pngExt) == 0) {
					rest = rest.substr(0, rest.size() - pngExt.size());
				}
				if (!rest.empty()) {
					bool allDigits = true;
					for (char ch : rest) {
						if (!isdigit((unsigned char)ch)) { allDigits = false; break; }
					}
					if (allDigits) {
						maxUsedNum = std::max(maxUsedNum, std::atoi(rest.c_str()));
					}
				}
			}
		}

		int num = std::max(0, _renameStartNum);
		if (maxUsedNum >= 0 && _renameStartNum == 0) {
			num = maxUsedNum + 1;
		}

		auto isOccupiedOutside = [&](const std::string& testName, int idxInRange) -> bool {
			for (int i = 0; i < total; ++i) {
				if (i == idxInRange) continue;
				if (i < beginIdx || i > endIdx) {
					if (_frames[i].name == testName) return true;
				}
			}
			return false;
		};

		for (int i = beginIdx; i <= endIdx; ++i) {
			std::string candidate;
			while (true) {
				candidate = FormatIndexedName(prefix, num++);
					if (!isOccupiedOutside(candidate, i)) break;
			}
			_frames[i].name = candidate;
		}
	}

	PushState();
	RefreshAtlasPreviewIfOpen();
}

void SpriteEditor::EnsureUniqueFrameNamesForExport() {
	for (int i = 0; i < (int)_frames.size(); ++i) {
		if (_frames[i].name.empty()) {
			char buf[64];
			snprintf(buf, sizeof(buf), "frame_%04d.png", i);
			_frames[i].name = buf;
		}
		MakeUniqueFrameName(_frames[i].name, i);
	}
}
 