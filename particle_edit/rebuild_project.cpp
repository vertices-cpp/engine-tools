#include <iostream>
#include <filesystem>
#include <system_error>
#include <cstdlib>
#include <cctype>

namespace fs = std::filesystem;

bool iequals(const std::string& a, const std::string& b) {
	return std::equal(a.begin(), a.end(), b.begin(), b.end(),
		[](char a, char b) {
		return std::tolower(static_cast<unsigned char>(a)) ==
			std::tolower(static_cast<unsigned char>(b));
	});
}

// 清理 proj.win32 目录（保留 main.cpp）
void cleanup_win32_dir(const fs::path& target_dir) {
	if (!fs::exists(target_dir)) {
		std::cout << "[WARN] 目录不存在: " << target_dir << std::endl;
		return;
	}

	std::error_code ec;
	// 使用 directory_iterator 遍历 proj.win32 顶层（不直接用 recursive 以便于逐个安全处理）
	for (const auto& entry : fs::directory_iterator(target_dir, ec)) {
		if (ec) {
			std::cerr << "[ERROR] 读取目录出错: " << ec.message() << std::endl;
			continue;
		}

		// 获取当前文件/文件夹名称
		fs::path filename = entry.path().filename();

		// 跳过 main.cpp (忽略大小写匹配可保证 Windows 下更安全)
		if (entry.is_regular_file() && iequals(filename.string()  , "main.cpp")) {
			std::cout << "[SKIP] 跳过保留文件: " << entry.path() << std::endl;
			continue;
		}

		// 删除除 main.cpp 之外的所有文件和子目录（remove_all 会递归删除文件夹）
		std::uintmax_t removed_count = fs::remove_all(entry.path(), ec);
		if (ec) {
			std::cerr << "[ERROR] 删除失败 " << entry.path() << ": " << ec.message() << std::endl;
		}
		else {
			std::cout << "[CLEAN] 已删除 (" << removed_count << " 项): " << entry.path() << std::endl;
		}
	}
}

int main() {
	// 假设 proj.win32 在当前工作目录下，也可以改为绝对路径：D:/cplusplus/game/proj.win32
	fs::path win32_path = fs::current_path() / "example_sdl2_sdlrenderer2";

	std::cout << "=== 1. 开始清理目录: " << win32_path << " ===" << std::endl;
	cleanup_win32_dir(win32_path);

	std::cout << "\n=== 2. 执行 CMake 构建命令 ===" << std::endl;

	// 构造组合命令：cd 进入 proj.win32 目录，然后执行 CMake 生成命令
	// Windows cmd 中使用 && 连接两条命令
	std::string cmake_cmd = "cd /d \"" + win32_path.string() + "\" && cmake .. -G \"Visual Studio 15 2017\" -A win32";

	std::cout << "[EXEC] " << cmake_cmd << std::endl;

	int ret = std::system(cmake_cmd.c_str());

	if (ret == 0) {
		std::cout << "\n[SUCCESS] CMake 工程生成成功！" << std::endl;
	}
	else {
		std::cerr << "\n[FAILED] CMake 指令执行失败，退出码: " << ret << std::endl;
	}

	return 0;
}