// Sprite_Editor
//by vertices
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string>
#define SDL_MAIN_HANDLED

#if defined(_WIN32)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_sdl2.h"

#include "SpriteEditor.h"

// ============================================================
// 平台判断
// ============================================================
#if defined(__ANDROID__) || defined(__IPHONEOS__) || defined(TARGET_OS_IPHONE)
#define PE_MOBILE 1
#else
#define PE_MOBILE 0
#endif

// ============================================================
// 字体加载
// ============================================================
static void setupFonts(float dpiScale, bool isMobile)
{
    ImGuiIO& io = ImGui::GetIO();

    // ★ 安卓直接定 36px，PC 定 18px
    float fontSize = isMobile ? 36.0f : 18.0f; 

    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;

    const char* path = nullptr;
    ImFont* font = nullptr;

#if PE_MOBILE
    const char* mobilePaths[] = {
        "/system/fonts/NotoSansCJK-Regular.ttc",
        "/system/fonts/DroidSansFallback.ttf",
        "Resources/fonts/NotoSansSC-Regular.otf",
        "Resources/fonts/msyh.ttc"
    };
    for (auto p : mobilePaths) {
        FILE* fp = fopen(p, "rb");
        if (fp) { fclose(fp); path = p; break; }
    }
#elif defined(_WIN32)
    path = "C:/Windows/Fonts/msyh.ttc";
#elif defined(__APPLE__)
    path = "/System/Library/Fonts/PingFang.ttc";
#else
    path = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc";
#endif

    if (path) {
        font = io.Fonts->AddFontFromFileTTF(path, fontSize, &cfg,
            io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    }

    if (!font) {
        io.Fonts->AddFontDefault();
    }
}

static void setupStyle(bool isMobile, float dpiScale)
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    style.GrabRounding   = 3.0f;

    if (isMobile) {
        // 安卓：只放大控件，不再乘 DPI
        style.ScrollbarSize = 30.0f;
        style.GrabMinSize   = 30.0f;
        style.FramePadding  = ImVec2(16, 12);
        style.ItemSpacing   = ImVec2(12, 14);
        style.WindowPadding = ImVec2(16, 16);
    } else {
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize   = 12.0f;
        style.FramePadding  = ImVec2(8, 5);
        style.ItemSpacing   = ImVec2(8, 6);
        style.WindowPadding = ImVec2(10, 10);
        
        // ★ 只有在 PC 端才乘 DPI
        style.ScaleAllSizes(dpiScale);
    }
	ImVec4* colors = style.Colors;
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.05f, 0.05f, 0.08f, 0.45f); // 减轻背景压暗
	colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.16f, 0.20f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.24f, 0.45f, 0.72f, 0.80f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.55f, 0.85f, 0.90f);
}
// ============================================================
// DPI 缩放
// ============================================================
static float calcDpiScale(SDL_Window* window, SDL_Renderer* renderer)
{
    float dpiScale = 1.0f;
#if PE_MOBILE
    float ddpi = 0.0f;
    if (SDL_GetDisplayDPI(0, &ddpi, nullptr, nullptr) == 0 && ddpi > 0.0f)
        dpiScale = ddpi / 160.0f;
#else
    if (window && renderer) {
        int ww = 0, wh = 0, dw = 0, dh = 0;
        SDL_GetWindowSize(window, &ww, &wh);
        SDL_GetRendererOutputSize(renderer, &dw, &dh);
        if (ww > 0 && dw > 0) dpiScale = (float)dw / (float)ww;
    }
#endif
    if (dpiScale <= 0.0f) dpiScale = 1.0f;
    if (dpiScale > 3.0f)  dpiScale = 3.0f;
    return dpiScale;
}

// ============================================================
// 样式


// ============================================================
// 主函数
// ============================================================
int main(int argc, char* argv[])
{
    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
 

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#if PE_MOBILE
    window_flags = (SDL_WindowFlags)(window_flags | SDL_WINDOW_FULLSCREEN);
#endif

    int winW = 1080, winH = 768;
#if PE_MOBILE
    winW = 0; winH = 0;
#endif

    SDL_Window* window = SDL_CreateWindow(
        "Particle Editor by vertices",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW, winH, window_flags);
    if (!window) return -1;

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) return -1;

 
    // ---------- ImGui ----------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#if !PE_MOBILE
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
#endif

    float dpiScale = calcDpiScale(window, renderer);
    setupStyle(PE_MOBILE, dpiScale);
    setupFonts(dpiScale, PE_MOBILE); // ★ 加载中文字体

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    
    int initW = (winW > 0) ? winW : 1080;
    int initH = (winH > 0) ? winH : 1920;
    ImGuiSDL::Initialize(renderer, initW, initH);

   SpriteEditor* editor = new SpriteEditor();
    editor->init(renderer);

    // ---------- 主循环 ----------
    bool done = false;
    Uint32 lastTicks = SDL_GetTicks();

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
                done = true;

            if (event.type == SDL_DROPFILE) {
                char* droppedPath = event.drop.file;
                if (droppedPath) {
                    std::string path(droppedPath);
                    std::string ext;
                    size_t dot = path.find_last_of('.');
                    if (dot != std::string::npos) {
                        ext = path.substr(dot);
                        for (auto& ch : ext) ch = (char)tolower(ch);
                    }
                    if (ext == ".plist") editor->loadPlist(path.c_str());
                    else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                        editor->loadImage(path.c_str());
                    SDL_free(droppedPath);
                }
            }
            // 快捷键撤销
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_z && (event.key.keysym.mod & KMOD_CTRL)) {
                // editor->Undo(); 
            }
#if PE_MOBILE
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_AC_BACK)
                done = true;
#endif
        }
 
        Uint32 now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        lastTicks = now;

        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 替换绘制逻辑
        editor->update(dt);
        editor->drawUI();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGuiSDL::Render(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // 清理
    delete editor;

    // ---------- 清理 ----------
   
    ImGuiSDL::Deinitialize();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}