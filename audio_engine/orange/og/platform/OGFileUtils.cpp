#include "platform/OGFileUtils.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__ANDROID__) ||defined(__linux__)
#include <unistd.h>
#include <sys/stat.h>
#endif 

OG_BEGIN

FileUtils* FileUtils::s_sharedFileUtils = nullptr;



 
//-------------------------------------FileUtils--------------------------

//获取绝对路径
static void _checkPath()
{
#if defined(_WIN32)
	if (s_resourcePath.empty())
	{
		WCHAR utf16Path[OG_MAX_PATH] = { 0 };
		GetModuleFileNameW(NULL, utf16Path, OG_MAX_PATH - 1);
		WCHAR *pUtf16ExePath = &(utf16Path[0]);

		// We need only directory part without exe
		WCHAR *pUtf16DirEnd = wcsrchr(pUtf16ExePath, L'\\');

		char utf8ExeDir[OG_MAX_PATH] = { 0 };
		int nNum = WideCharToMultiByte(CP_UTF8, 0, pUtf16ExePath, pUtf16DirEnd - pUtf16ExePath + 1, utf8ExeDir, sizeof(utf8ExeDir), nullptr, nullptr);

		s_resourcePath = convertPathFormatToUnixStyle(utf8ExeDir);
	}
#elif defined(__ANDROID__)
	char fullpath[256] = { 0 };
	ssize_t length = readlink("/proc/self/exe", fullpath, sizeof(fullpath) - 1);

	if (length <= 0) {
		return;
	}

	fullpath[length] = '\0';
	std::string appPath = fullpath;
	s_resourcePath = appPath.substr(0, appPath.find_last_of('/') + 1);

#endif

}

bool FileUtils::init()
{
	_checkPath();
	_defaultResRootPath = convertPathFormatToUnixStyle(s_resourcePath) + "Resources/";
	_searchPathArray.push_back(_defaultResRootPath);
	return true;
}

//判断是否是绝对路径
bool FileUtils::isAbsolutePath(const std::string& strPath) const
{
	if (strPath.empty()) return false;

#if defined(_WIN32)
	// 盘符 C:\ 或 C:/
	if (strPath.size() >= 2 &&
		((strPath[0] >= 'a' && strPath[0] <= 'z') ||
		(strPath[0] >= 'A' && strPath[0] <= 'Z')) &&
		strPath[1] == ':')
	{
		return true;
	}
	// Windows UNC \\server
	if (strPath.size() >= 2 && strPath[0] == '\\' && strPath[1] == '\\')
	{
		return true;
	}
#endif

	// 类 Unix / Android：以 / 开头就是绝对路径
	if (strPath[0] == '/')
	{
		return true;
	}

	return false;
}

//转UTF8
std::wstring StringUtf8ToWideChar(const std::string& strUtf8)
{
	std::wstring ret;
// 	if (!strUtf8.empty())
// 	{
// 		int nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, nullptr, 0);
// 		if (nNum)
// 		{
// 			WCHAR* wideCharString = new WCHAR[nNum + 1];
// 			wideCharString[0] = 0;
// 
// 			nNum = MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, wideCharString, nNum + 1);
// 
// 			ret = wideCharString;
// 			delete[] wideCharString;
// 		}
// 		else
// 		{
// 			//CCLOG("Wrong convert to WideChar code:0x%x", GetLastError());
// 		}
// 	}
	return ret;
}
//检测文件状态
bool FileUtils::isFileExistInternal(const std::string& strFilePath)const
{

	if (strFilePath.empty())
	{
		return false;
	}

	std::string strPath = strFilePath;
	if (!isAbsolutePath(strPath))
	{ // Not absolute path, add the default root path at the beginning.
		strPath.insert(0, _defaultResRootPath);
	}
	// 尝试以二进制只读方式打开
	FILE *fp = fopen(strPath.c_str(), "rb");
	if (fp)
	{
		fclose(fp);
		return true;
	} 
// 	DWORD attr = GetFileAttributesW(StringUtf8ToWideChar(strPath).c_str());
// 	if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
// 		return false;   //  not a file

	return false;
}
std::string FileUtils::getFullPathForFilenameWithinDirectory(const std::string& directory, const std::string& filename)const

{
	// get directory+filename, safely adding '/' as necessary
	std::string ret = directory;
	if (directory.size() && directory[directory.size() - 1] != '/') {
		ret += '/';
	}
	ret += filename;
	// if the file doesn't exist, return an empty string
	if (!isFileExistInternal(ret)) {
		ret = "";
	}
	return ret;
}

std::string FileUtils::getPathForFilename(const std::string& filename, const std::string & resource_path)const
{
	std::string file = filename;
	std::string file_path = "";
	size_t pos = filename.find_last_of('/');
	if (pos != std::string::npos)
	{
		file_path = filename.substr(0, pos + 1);
		file = filename.substr(pos + 1);
	}

	// searchPath + file_path + resourceDirectory
	std::string path = resource_path;
	path += file_path;


	path = getFullPathForFilenameWithinDirectory(path, file);

	return path;
}

std::string FileUtils::fullPathForFilename(const std::string &filename)const
{


	if (filename.empty())
	{
		return "";
	}

	if (isAbsolutePath(filename))
	{
		return filename;
	}

	// Already Cached ?
	auto cacheIter = _fullPathCache.find(filename);
	if (cacheIter != _fullPathCache.end())
	{
		return cacheIter->second;
	}
	for (const auto& searchIt : _searchPathArray)
	{
		std::string fullpath = this->getPathForFilename(filename, searchIt);
		if (fullpath.size())
		{
			_fullPathCache.emplace(std::make_pair(filename, fullpath));
			return fullpath;
		}
	}

	return "";
}


std::string FileUtils::getSuitableFOpen(const std::string & filenameUtf8) const
{
	//CCASSERT(false, "getSuitableFOpen should be override by platform FileUtils");
	return filenameUtf8;
}

Data FileUtils::getDataFromFile(const std::string& filename) const
{
	Data d;
	getContents(filename, &d);
	return d;
}
FileUtils::Status FileUtils::getContents(const std::string& filename, ResizableBuffer* buffer) const
{
	if (filename.empty())
		return Status::NotExists;

	auto fs = FileUtils::getInstance();

	std::string fullPath = fs->fullPathForFilename(filename);
	if (fullPath.empty())
		return Status::NotExists;

	std::string suitableFullPath = fs->getSuitableFOpen(fullPath);

	struct stat statBuf;
	if (stat(suitableFullPath.c_str(), &statBuf) == -1) {
		return Status::ReadFailed;
	}

	if (!(statBuf.st_mode & S_IFREG)) {
		return Status::NotRegularFileType;
	}

	FILE *fp = fopen(suitableFullPath.c_str(), "rb");
	if (!fp)
		return Status::OpenFailed;

	size_t size = statBuf.st_size;

	buffer->resize(size);
	size_t readsize = fread(buffer->buffer(), 1, size, fp);
	fclose(fp);

	if (readsize < size) {
		buffer->resize(readsize);
		return Status::ReadFailed;
	}

	return Status::OK;
}

bool FileUtils::isFileExist(const std::string& filename) const
{
	if (isAbsolutePath(filename))
	{
		return isFileExistInternal(filename);
	}
	else
	{
		std::string fullpath = fullPathForFilename(filename);
		if (fullpath.empty())
			return false;
		else
			return true;
	}
}
std::string FileUtils::getFileExtension(const std::string& filePath) const
{
	std::string fileExtension;
	size_t pos = filePath.find_last_of('.');
	if (pos != std::string::npos)
	{
		fileExtension = filePath.substr(pos, filePath.length());

		std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
	}

	return fileExtension;
}


OG_END