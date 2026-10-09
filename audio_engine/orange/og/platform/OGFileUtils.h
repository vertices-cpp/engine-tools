#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_
 
#include "base/OGData.h"
#include "platform/OGHeader.h"
 

#include <string> 
#include <map> 
#include <iostream>
#include <vector>
#if defined(_WIN32)
  #include <windows.h>
#endif
// #pragma warning(disable:4996)


 
OG_BEGIN
 
#define MAX_FILENAME 512
#define READ_SIZE 8192

#define dir_delimter '/'
#define OG_MAX_PATH  512



static std::string s_resourcePath = "";


// \\替换为/
static inline std::string convertPathFormatToUnixStyle(const std::string& path)
{
	std::string ret = path;
	int len = ret.length();
	for (int i = 0; i < len; ++i)
	{
		if (ret[i] == '\\')
		{
			ret[i] = '/';
		}
	}
	return ret;
}
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
#endif
 
}

//-------------------------------------AssetManager--------------------------
// #include <vector>
// class AssetManager {
// 	std::string _FilePath;
// 	std::vector<Data> file;
// public:
// 	vector<Data> get_file() { return file; }
// 	void addFile(const string &filePath, const Data &d) {
// 		_FilePath = filePath;
// 		file.push_back(move(d));
// 	}
// 
// };

//-------------------------------------FileUtils--------------------------




class ResizableBuffer {
public:
	virtual ~ResizableBuffer() {}
	virtual void resize(size_t size) = 0;
	virtual void* buffer() const = 0;
};

template<typename T>
class ResizableBufferAdapter { };


template<typename CharT, typename Traits, typename Allocator>
class ResizableBufferAdapter< std::basic_string<CharT, Traits, Allocator> > : public ResizableBuffer {
	typedef std::basic_string<CharT, Traits, Allocator> BufferType;
	BufferType* _buffer;
public:
	explicit ResizableBufferAdapter(BufferType* buffer) : _buffer(buffer) {}
	virtual void resize(size_t size) override {
		_buffer->resize((size + sizeof(CharT) - 1) / sizeof(CharT));
	}
	virtual void* buffer() const override {
		// can not invoke string::front() if it is empty

		if (_buffer->empty())
			return nullptr;
		else
			return &_buffer->front();
	}
};

template<typename T, typename Allocator>
class ResizableBufferAdapter< std::vector<T, Allocator> > : public ResizableBuffer {
	typedef std::vector<T, Allocator> BufferType;
	BufferType* _buffer;
public:
	explicit ResizableBufferAdapter(BufferType* buffer) : _buffer(buffer) {}
	virtual void resize(size_t size) override {
		_buffer->resize((size + sizeof(T) - 1) / sizeof(T));
	}
	virtual void* buffer() const override {
		// can not invoke vector::front() if it is empty

		if (_buffer->empty())
			return nullptr;
		else
			return &_buffer->front();
	}
};


template<>
class ResizableBufferAdapter<Data> : public ResizableBuffer {
	typedef Data BufferType;
	BufferType* _buffer;
public:
	explicit ResizableBufferAdapter(BufferType* buffer) : _buffer(buffer) {}
	virtual void resize(size_t size) override {
		size_t oldSize = static_cast<size_t>(_buffer->getSize());
		if (oldSize != size) {
			auto old = _buffer->getBytes();
			void* buffer = realloc(old, size);
			if (buffer)
				_buffer->fastSet((unsigned char*)buffer, size);
		}
	}
	virtual void* buffer() const override {
		return _buffer->getBytes();
	}
};

class FileUtils {
	enum class Status
	{
		OK = 0,
		NotExists = 1, // File not exists
		OpenFailed = 2, // Open file failed.
		ReadFailed = 3, // Read failed
		NotInitialized = 4, // FileUtils is not initializes
		TooLarge = 5, // The file is too large (great than 2^32-1)
		ObtainSizeFailed = 6, // Failed to obtain the file size.
		NotRegularFileType = 7 // File type is not S_IFREG
	};

	static FileUtils* s_sharedFileUtils;
 

	std::string _defaultResRootPath;
	mutable std::unordered_map<std::string, std::string> _fullPathCache;
	std::vector<std::string> _searchPathArray;

	std::vector<Data*> _FileCache;
	std::vector<Data*> & getData() {
		return _FileCache;
	}
public:
	static FileUtils*  getInstance()
	{
		if (s_sharedFileUtils == nullptr)
		{
			s_sharedFileUtils = new FileUtils();
			if (!s_sharedFileUtils->init())
			{
				delete s_sharedFileUtils;
				s_sharedFileUtils = nullptr;

			}
		}
		return s_sharedFileUtils;
	}
	~FileUtils() {
		clear();
	}
	void clear() {
		_fullPathCache.clear();
		
		if (_FileCache.size())
		{
			for (auto &i:_FileCache)
			{
				if (i!=nullptr)
				{
					delete i;
				}
			}
		}
		_FileCache.clear();
	}

	bool init() {
		
		 //s_resourcePath = SDL_GetBasePath();
		 _checkPath();
		_defaultResRootPath = convertPathFormatToUnixStyle(s_resourcePath) + "Resources/";
		_searchPathArray.push_back(_defaultResRootPath);
		return true;
	}
static std::vector<Data*>  &get_data() {
		auto f = FileUtils::getInstance();
		return f->getData();
	} 
	bool isAbsolutePath(const std::string & path) const;
	bool isFileExistInternal(const std::string & strFilePath)const;
	std::string getFullPathForFilenameWithinDirectory(const std::string & directory, const std::string & filename)const;
	std::string getPathForFilename(const std::string & filename, const std::string & resource_path)const;


	std::string fullPathForFilename(const std::string & filename)const;

	void addData(const std::string & zipFilePath, const std::string & fileName, unsigned char * buf, int len);

	 

	virtual std::string getSuitableFOpen(const std::string& filenameUtf8) const;
	Data getDataFromFile(const std::string & filename) const;
	FileUtils::Status getContents(const std::string & filename, ResizableBuffer * buffer) const;
	bool isFileExist(const std::string & filename) const;
	std::string getFileExtension(const std::string & filePath) const;
	template <
		typename T,
		typename Enable = typename std::enable_if<
		std::is_base_of< ResizableBuffer, ResizableBufferAdapter<T> >::value
		>::type
	>
		Status getContents(const std::string& filename, T* buffer) const {
		ResizableBufferAdapter<T> buf(buffer);
		return getContents(filename, &buf);
	}
};


OG_END

#endif