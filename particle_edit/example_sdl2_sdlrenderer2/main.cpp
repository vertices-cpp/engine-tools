// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <stdio.h>
#include <SDL.h>
#include "ParticleManager.h"

#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

// ============================================================
// 平台判断
// ============================================================
#if defined(__ANDROID__) || defined(__IPHONEOS__) || defined(TARGET_OS_IPHONE)
#define PE_MOBILE 1
#else
#define PE_MOBILE 0
#endif

// ============================================================
// 字体路径
// ============================================================
static const char* getFontPath()
{
#if PE_MOBILE
    return "fonts/NotoSansSC-Regular.otf";
#elif defined(_WIN32)
    return "C:/Windows/Fonts/msyh.ttc";
#elif defined(__APPLE__)
    return "/System/Library/Fonts/PingFang.ttc";
#else
    return "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
#endif
}

// 读文件到内存
static void* readFileToMemory(const char* path, int* outLen)
{
    if (!outLen) return nullptr;
    *outLen = 0;

    SDL_RWops* rw = SDL_RWFromFile(path, "rb");
    if (!rw) return nullptr;

    Sint64 size = SDL_RWsize(rw);
    if (size <= 0) { SDL_RWclose(rw); return nullptr; }

    void* buf = malloc((size_t)size);
    if (!buf) { SDL_RWclose(rw); return nullptr; }

    size_t read = SDL_RWread(rw, buf, 1, (size_t)size);
    SDL_RWclose(rw);
    if (read != (size_t)size) { free(buf); return nullptr; }

    *outLen = (int)size;
    return buf;
}

// 加载字体
static void setupFonts(float dpiScale, bool isMobile)
{
    ImGuiIO& io = ImGui::GetIO();

    float fontSize = (isMobile ? 16.0f : 18.0f) * dpiScale;

    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;
    cfg.FontDataOwnedByAtlas = true;

    const char* path = getFontPath();
    ImFont* font = nullptr;

#if PE_MOBILE
    // 手机：从内存加载
    int len = 0;
    void* data = readFileToMemory(path, &len);
    if (data && len > 0) {
        font = io.Fonts->AddFontFromMemoryTTF(
            data, len, fontSize, &cfg,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        if (!font) free(data);
    }
#else
    // 桌面：直接用文件路径
    font = io.Fonts->AddFontFromFileTTF(
        path, fontSize, &cfg,
        io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
#endif

    if (!font) {
        SDL_Log("字体加载失败: %s，使用默认字体", path);
        io.Fonts->AddFontDefault();
    }
    else {
        SDL_Log("字体加载成功: %s (%.1fpx)", path, fontSize);
    }
}

static float calcDpiScale(SDL_Window* window)
{
    float dpiScale = 1.0f;

#if PE_MOBILE
    // 手机：用显示 DPI 计算
    float ddpi = 0.0f;
    if (SDL_GetDisplayDPI(0, &ddpi, nullptr, nullptr) == 0 && ddpi > 0.0f)
        dpiScale = ddpi / 160.0f;   // Android 基准 160dpi
#else
    // 桌面：优先用窗口逻辑尺寸 / 像素尺寸的比值（兼容所有 SDL2 版本）
    if (window)
    {
        int ww = 0, wh = 0;      // 逻辑尺寸
        int dw = 0, dh = 0;      // 物理像素尺寸
        SDL_GetWindowSize(window, &ww, &wh);

        // 2.26+ 才有 SDL_GetWindowSizeInPixels，这里用 SDL_GetRendererOutputSize 代替
        SDL_Renderer* ren = SDL_GetRenderer(window);
        if (ren) {
            SDL_GetRendererOutputSize(ren, &dw, &dh);
        }
        else {
            dw = ww; dh = wh;
        }

        if (ww > 0 && dw > 0)
            dpiScale = (float)dw / (float)ww;
    }
#endif

    if (dpiScale <= 0.0f) dpiScale = 1.0f;
    if (dpiScale > 3.0f)  dpiScale = 3.0f;
    return dpiScale;
}

// 应用样式
static void setupStyle(bool isMobile, float dpiScale)
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    // 圆角
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;

    // 间距
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;

    // 边框
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    // 手机：加大控件
    if (isMobile) {
        style.ScrollbarSize = 24.0f;
        style.GrabMinSize = 24.0f;
        style.FramePadding = ImVec2(14, 10);
        style.ItemSpacing = ImVec2(10, 12);
        style.WindowPadding = ImVec2(14, 14);
    }

    // 颜色微调
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    c[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
    c[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.26f, 0.32f, 1.00f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
    c[ImGuiCol_Header] = ImVec4(0.24f, 0.36f, 0.55f, 0.85f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.45f, 0.68f, 0.90f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.34f, 0.52f, 0.78f, 1.00f);
    c[ImGuiCol_Button] = ImVec4(0.22f, 0.26f, 0.34f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.40f, 0.58f, 1.00f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.36f, 0.50f, 0.72f, 1.00f);
    c[ImGuiCol_CheckMark] = ImVec4(0.60f, 0.78f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.42f, 0.60f, 0.85f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.52f, 0.72f, 1.00f, 1.00f);
    c[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    c[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.18f, 0.24f, 1.00f);

    // 缩放
    float uiScale = isMobile ? (dpiScale * 1.3f) : dpiScale;
    style.ScaleAllSizes(uiScale);
}

// ============================================================
int main(int, char**)
{
    // ---------- 初始化 SDL ----------
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

#if PE_MOBILE
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

    // ---------- 窗口 / 渲染器 ----------
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#if PE_MOBILE
    window_flags = (SDL_WindowFlags)(window_flags | SDL_WINDOW_FULLSCREEN);
#endif

    int winW = 1080, winH = 768;
#if PE_MOBILE
    winW = 0; winH = 0;   // 手机全屏
#endif

    SDL_Window* window = SDL_CreateWindow(
        "Particle Editor QQ:1598058687 桔皮沙拉",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW, winH, window_flags);
    if (!window) { SDL_Log("CreateWindow failed: %s", SDL_GetError()); return -1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Error creating SDL_Renderer!");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // ---------- ImGui ----------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#if !PE_MOBILE
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
#endif

    float dpiScale = calcDpiScale(window);
    setupStyle(PE_MOBILE, dpiScale);

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    setupFonts(dpiScale, PE_MOBILE);

    // ---------- ParticleManager ----------
    auto pm = ParticleManager::create();
    pm->init(renderer,400,400);

    // ---------- 主循环 ----------
    bool done = false;
    Uint32 lastTicks = SDL_GetTicks();

	while (!done)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT
				&& event.window.event == SDL_WINDOWEVENT_CLOSE
				&& event.window.windowID == SDL_GetWindowID(window))
				done = true;
			if (event.type == SDL_DROPFILE)
			{
				char* droppedPath = event.drop.file;
				if (droppedPath)
				{
					std::string path(droppedPath);

					// 按扩展名分发
					std::string ext;
					size_t dot = path.find_last_of('.');
					if (dot != std::string::npos) {
						ext = path.substr(dot);
						for (auto& c : ext) c = (char)tolower(c);
					}

					if (ext == ".plist") {
						pm->loadDragPlist(path.c_str());   // 你现成的
						
					}
					else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
						 
						pm->loadDragTexture(path.c_str());     // 你现成的
					}

					SDL_free(droppedPath);
				}
				
			}
// 			if (event.type == SDL_MOUSEBUTTONDOWN)
// 			{
// 				if (event.button.button == SDL_BUTTON_RIGHT)
// 				{
// 					pm->getPexample()->resetSystem();
// 				}
// 			}
#if PE_MOBILE
            // Android 返回键
            if (event.type == SDL_KEYDOWN
                && event.key.keysym.scancode == SDL_SCANCODE_AC_BACK)
                done = true;
#endif
        }

        // dt
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;   // 防止切后台回来爆炸
        lastTicks = now;

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ---------- 更新 & 绘制 ----------
        pm->update(dt);
        pm->drawUI();
       // SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
        // ---------- 渲染 ----------
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // ---------- 清理 ----------
    delete pm;

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
