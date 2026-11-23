#include "SimpleFileDialog.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cctype>

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <limits.h>
#endif

// ★ 初始化静态变量
std::map<std::string, std::string> SimpleFileDialog::s_lastBrowsedDirs;

// ═══════════════════════════════════════════════════════════════
// 内部辅助函数
// ═══════════════════════════════════════════════════════════════
namespace {

bool IsDirectory(const std::string& path)
{
#if defined(_WIN32)
    struct _stat st;
    return _stat(path.c_str(), &st) == 0 && (st.st_mode & _S_IFDIR);
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

std::string ToLowerCopy(const std::string& s)
{
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return (char)std::tolower(c); });
    return r;
}

bool IsWithinRoot(const std::string& path, const std::string& root)
{
    if (root.empty()) return true;
    if (path.size() < root.size()) return false;
    return path.compare(0, root.size(), root) == 0;
}

#if defined(__ANDROID__)
// 解析 Android 包名根目录
std::string GetAndroidPackageRoot(const std::string& internalPath)
{
    size_t p1 = internalPath.find('/', 1);
    if (p1 == std::string::npos) return "";
    size_t p2 = internalPath.find('/', p1 + 1);
    if (p2 == std::string::npos) return "";
    size_t p3 = internalPath.find('/', p2 + 1);
    if (p3 == std::string::npos) return "";
    return internalPath.substr(0, p3 + 1);
}
#endif

} // namespace

// ═══════════════════════════════════════════════════════════════
// 默认路径与根限制
// ═══════════════════════════════════════════════════════════════
std::string SimpleFileDialog::GetDefaultDir()
{
	std::string dir;

#if defined(__ANDROID__)
	// 1. 尝试 /proc/self/exe（C4droid 直接运行 ELF 时有效）
	char buf[PATH_MAX] = { 0 };
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len > 0) {
		buf[len] = 0;
		std::string exe = buf;
		size_t slash = exe.find_last_of('/');
		std::string base = (slash != std::string::npos) ? exe.substr(slash + 1) : exe;

		// 排除 APK 运行时的 app_process
		if (base != "app_process" && base != "app_process64" && base != "app_process32") {
			std::string exeDir = (slash != std::string::npos) ? exe.substr(0, slash) : ".";

			// 路径映射：/storage/emulated/0/ -> /sdcard/
			const std::string kEmulated = "/storage/emulated/0/";
			if (exeDir.compare(0, kEmulated.size(), kEmulated) == 0) {
				exeDir = "/sdcard/" + exeDir.substr(kEmulated.size());
			}

			// 2. 优先使用 exe 旁边的 Resources
			if (IsDirectory(exeDir + "/Resources")) {
				dir = exeDir + "/Resources";
			}
			// 3. 如果没有 Resources，退而求其次打开 bin 目录本身
			else if (IsDirectory(exeDir)) {
				dir = exeDir + "/";
			}
		}
	}

	// 4. 如果什么都没检测到，默认打开 /sdcard/
	if (dir.empty()) {
		dir = "/sdcard/";
	}

#elif defined(_WIN32)
	// ================= Windows 平台 =================
	char buf[MAX_PATH] = { 0 };
	DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
	if (n > 0 && n < MAX_PATH) {
		dir = buf;
		size_t pos = dir.find_last_of("\\/");
		if (pos != std::string::npos) dir = dir.substr(0, pos);
	}
	if (dir.empty()) dir = ".";

	// 优先使用 exe 旁边的 Resources
	std::string candidate = dir + "/Resources";
	if (IsDirectory(candidate)) {
		dir = candidate;
	}
	else {
		dir = dir + "/"; // 如果没有，就打开 exe 所在目录本身
	}

#elif defined(__linux__)
	// ================= Linux 平台 =================
	char buf[PATH_MAX] = { 0 };
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len > 0) {
		buf[len] = 0;
		dir = buf;
		size_t pos = dir.find_last_of('/');
		if (pos != std::string::npos) dir = dir.substr(0, pos);
	}
	if (dir.empty()) dir = ".";

	// 优先使用 exe 旁边的 Resources
	std::string candidate = dir + "/Resources";
	if (IsDirectory(candidate)) {
		dir = candidate;
	}
	else {
		dir = dir + "/";
	}

#else
	// ================= 其他未知平台 =================
	dir = "./Resources";
#endif

	// 统一将反斜杠转成斜杠，方便路径处理
	std::replace(dir.begin(), dir.end(), '\\', '/');

	// 确保结尾有斜杠
	if (!dir.empty() && dir.back() != '/') {
		dir += '/';
	}

	return dir;
}

std::string SimpleFileDialog::GetRootLimit()
{
#if defined(__ANDROID__)
    std::string def = GetDefaultDir();
    if (def.compare(0, 8, "/sdcard/") == 0) {
        return "/sdcard/";
    }
    std::string pkgRoot = GetAndroidPackageRoot(def);
    if (!pkgRoot.empty()) {
        return pkgRoot;
    }
    return "";
#else
    return "";
#endif
}

// ═══════════════════════════════════════════════════════════════
// Open / Close
// ═══════════════════════════════════════════════════════════════
void SimpleFileDialog::Open(const std::string& title,
                            const std::string& exts,
                            const std::string& initialDir)
{
    _isOpen        = true;
    _needOpenPopup = true;
    _title         = title;
    _filterExts    = exts;
    ParseFilter();

    // ★ 路径优先级：1. 强制指定的 initialDir > 2. 上次记住的路径 > 3. 默认路径
    if (!initialDir.empty()) {
        _currentPath = initialDir;
    } 
    else if (s_lastBrowsedDirs.find(_title) != s_lastBrowsedDirs.end() && !s_lastBrowsedDirs[_title].empty()) {
        _currentPath = s_lastBrowsedDirs[_title];
    } 
    else {
        _currentPath = GetDefaultDir();
    }

    _result.clear();
    _searchBuf[0] = 0;

    Refresh();
}

void SimpleFileDialog::Close()
{
    _isOpen = false;
}

// ═══════════════════════════════════════════════════════════════
// 过滤
// ═══════════════════════════════════════════════════════════════
void SimpleFileDialog::ParseFilter()
{
    _filterExtSet.clear();
    if (_filterExts.empty()) return;

    size_t start = 0;
    while (start <= _filterExts.size()) {
        size_t end = _filterExts.find(',', start);
        if (end == std::string::npos) end = _filterExts.size();

        std::string item = _filterExts.substr(start, end - start);
        while (!item.empty() && std::isspace((unsigned char)item.front()))
            item.erase(item.begin());
        while (!item.empty() && std::isspace((unsigned char)item.back()))
            item.pop_back();

        if (!item.empty()) {
            if (item[0] != '.') item = "." + item;
            _filterExtSet.insert(ToLowerCopy(item));
        }

        if (end == _filterExts.size()) break;
        start = end + 1;
    }
}

bool SimpleFileDialog::IsExtAllowed(const std::string& name) const
{
    if (_filterExtSet.empty()) return true;
    size_t pos = name.find_last_of('.');
    if (pos == std::string::npos) return false;
    return _filterExtSet.count(ToLowerCopy(name.substr(pos))) > 0;
}
// ═══════════════════════════════════════════════════════════════
// Refresh
// ═══════════════════════════════════════════════════════════════
void SimpleFileDialog::Refresh()
{
	// 1. 规范化路径
	std::replace(_currentPath.begin(), _currentPath.end(), '\\', '/');
	if (!_currentPath.empty() && _currentPath.back() != '/')
		_currentPath += '/';

	// 2. 根限制：Android 上不允许超出 /sdcard/
	std::string root = GetRootLimit();
	if (!root.empty() && !IsWithinRoot(_currentPath, root)) {
		_currentPath = root;
	}

	// 3. 同步到输入框
	snprintf(_pathBuf, sizeof(_pathBuf), "%s", _currentPath.c_str());

	_dirs.clear();
	_files.clear();

#if defined(_WIN32)
	// ================= Windows 遍历逻辑 =================
	std::string searchPath = _currentPath + "*";
	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			std::string name = fd.cFileName;
			if (name == "." || name == "..") continue;

			// 判断是否为文件夹
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				_dirs.push_back(name);
			}
			// 判断文件后缀是否允许
			else if (IsExtAllowed(name)) {
				_files.push_back(name);
			}
		} while (FindNextFileA(hFind, &fd));
		FindClose(hFind);
	}
#else
	// ================= Linux / Android 遍历逻辑 =================
	DIR* d = opendir(_currentPath.c_str());
	if (d) {
		struct dirent* entry;
		while ((entry = readdir(d)) != nullptr) {
			std::string name = entry->d_name;
			if (name == "." || name == "..") continue;

			std::string full = _currentPath + name;
			struct stat st;
			if (stat(full.c_str(), &st) != 0) continue;

			if (S_ISDIR(st.st_mode)) {
				_dirs.push_back(name);
			}
			else if (IsExtAllowed(name)) {
				_files.push_back(name);
			}
		}
		closedir(d);
	}
#endif

	// 4. 排序
	std::sort(_dirs.begin(), _dirs.end());
	std::sort(_files.begin(), _files.end());
}

// ═══════════════════════════════════════════════════════════════
// Display (含大字体自适应和记忆目录)
// ═══════════════════════════════════════════════════════════════
bool SimpleFileDialog::Display()
{
    if (!_isOpen) return false;

    if (_needOpenPopup) {
        ImGui::OpenPopup(_title.c_str());
        _needOpenPopup = false;
    }

    // 自适应手机大屏幕
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSize(ImVec2(displaySize.x * 0.95f, displaySize.y * 0.90f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    bool p_open = true;
    if (!ImGui::BeginPopupModal(_title.c_str(), &p_open, ImGuiWindowFlags_NoSavedSettings)) {
        if (!p_open) _isOpen = false;
        return false;
    }

    bool ret = false;

    // ESC 关闭
#if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18700)
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) p_open = false;
#else
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) p_open = false;
#endif

    // ── 路径行 ──
    const float btnW = ImGui::CalcTextSize("Up (..)").x + ImGui::GetStyle().FramePadding.x * 4;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW * 2 - ImGui::GetStyle().ItemSpacing.x * 2);
    
    if (ImGui::InputText("##Path", _pathBuf, sizeof(_pathBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
        _currentPath = _pathBuf;
        s_lastBrowsedDirs[_title] = _currentPath; // ★ 手动输入路径也记住
        Refresh();
    }

    ImGui::SameLine();
    if (ImGui::Button("Up (..)", ImVec2(btnW, 0))) {
        std::string path = _currentPath;
        if (!path.empty() && path.back() == '/') path.pop_back();
        size_t pos = path.find_last_of('/');
        bool atRoot = false;
        if (pos == std::string::npos) atRoot = true;
        else if (pos == 1 && path.size() == 2 && path[1] == ':') atRoot = true;
        else if (pos == 0) atRoot = true;

        if (!atRoot) {
            std::string parent = path.substr(0, pos + 1);
            std::string root = GetRootLimit();
            if (root.empty() || IsWithinRoot(parent, root)) {
                _currentPath = parent;
                s_lastBrowsedDirs[_title] = _currentPath; // ★ 向上一级也记住
                Refresh();
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh", ImVec2(btnW, 0))) {
        Refresh();
    }

    // ── 文件名过滤 ──
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##Search", "Filter by name...", _searchBuf, sizeof(_searchBuf));
    ImGui::Separator();

    std::string search = ToLowerCopy(_searchBuf);

    // 列表高度自适应
    float footerHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y * 2;
    ImGui::BeginChild("FileList", ImVec2(0, -footerHeight), true);

    // ── 目录 ──
    for (size_t i = 0; i < _dirs.size(); ++i) {
        const std::string& dir = _dirs[i];
        if (!search.empty() && ToLowerCopy(dir).find(search) == std::string::npos)
            continue;

        std::string label = "[DIR] " + dir;
        if (ImGui::Selectable(label.c_str())) {
            std::string dirName = dir;
            _currentPath += dirName + "/";
            _searchBuf[0] = 0;
            s_lastBrowsedDirs[_title] = _currentPath; // ★ 进入子文件夹也记住
            Refresh();
            break; // 只导航，不关闭弹窗
        }
    }

    // ── 文件 ──
    for (size_t i = 0; i < _files.size(); ++i) {
        const std::string& file = _files[i];
        if (!search.empty() && ToLowerCopy(file).find(search) == std::string::npos)
            continue;

        if (ImGui::Selectable(file.c_str())) {
            _result = _currentPath + file;
            s_lastBrowsedDirs[_title] = _currentPath; // ★ 选中文件时记住目录
            ret = true;
            p_open = false;
            break;
        }
    }

    ImGui::EndChild();
    ImGui::Separator();

    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        p_open = false;
    }

    if (!p_open) {
        _isOpen = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return ret;
}