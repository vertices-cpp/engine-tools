#ifndef _SIMPLE_FILE_DIALOG_H_
#define _SIMPLE_FILE_DIALOG_H_

#include <string>
#include <vector>
#include <set>
#include <map>       // ★ 新增：用于记忆路径
#include "imgui.h"

class SimpleFileDialog
{
public:
    void Open(const std::string& title,
              const std::string& exts = "",
              const std::string& initialDir = "");

    bool Display();
    void Close();
    bool IsOpen() const { return _isOpen; }

    const std::string& GetFilePath() const { return _result; }

    // 获取默认路径（优先 exe 旁边的 Resources，找不到回退 /sdcard/）
    static std::string GetDefaultDir();

private:
    void Refresh();
    void ParseFilter();
    bool IsExtAllowed(const std::string& name) const;
    
    // 限制最高只能退到 /sdcard/
    static std::string GetRootLimit();

    bool _isOpen = false;
    bool _needOpenPopup = false;
    std::string _title;
    std::string _filterExts;
    std::set<std::string> _filterExtSet;
    std::string _currentPath;
    std::string _result;

    std::vector<std::string> _dirs;
    std::vector<std::string> _files;

    char _pathBuf[1024] = { 0 };
    char _searchBuf[128]  = { 0 };

    // ★ 静态变量：记录每个窗口（标题）上次所在的目录
    static std::map<std::string, std::string> s_lastBrowsedDirs;
};

#endif // _SIMPLE_FILE_DIALOG_H_