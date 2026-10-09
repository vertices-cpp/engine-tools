#include "platform/OGFileUtils.h"


OG_BEGIN

FileUtils* FileUtils::s_sharedFileUtils = nullptr;



 
//-------------------------------------FileUtils--------------------------

//判断是否是绝对路径
bool FileUtils::isAbsolutePath(const std::string& strPath) const
{
	if (
		(strPath.length() > 2 &&
		((strPath[0] >= 'a' && strPath[0] <= 'z') ||
			(strPath[0] >= 'A' && strPath[0] <= 'Z'))
			&& strPath[1] == ':')

		||
		(strPath[0] == '/' && strPath[1] == '/'))
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