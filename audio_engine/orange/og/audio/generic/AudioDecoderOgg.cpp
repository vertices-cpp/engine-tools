

#include "audio/generic/AudioDecoderOgg.h"
#include "audio/generic/AudioMacros.h"

#include "platform/OGFileUtils.h"
 

#define LOG_TAG "AudioDecoderOgg"

namespace orange {

	AudioDecoderOgg::AudioDecoderOgg()
	{
	}

	AudioDecoderOgg::~AudioDecoderOgg()
	{
		close();
	}

	size_t AudioDecoderOgg::oggRead(void* ptr, size_t size, size_t nmemb, void* datasource)
	{
		MyOggIO* io = (MyOggIO*)datasource;
		size_t bytes = size * nmemb;
		size_t remain = io->size - io->pos;
		if (bytes > remain) bytes = remain;
		memcpy(ptr, io->data + io->pos, bytes);
		io->pos += bytes;
		return bytes / size;
	}

	int AudioDecoderOgg::oggSeek(void* datasource, ogg_int64_t offset, int whence)
	{
		MyOggIO* io = (MyOggIO*)datasource;
		if (whence == SEEK_SET)      io->pos = (size_t)offset;
		else if (whence == SEEK_CUR) io->pos += (size_t)offset;
		else if (whence == SEEK_END) io->pos = io->size + (size_t)offset;
		if (io->pos > io->size) io->pos = io->size;
		return 0;
	}

	int AudioDecoderOgg::oggClose(void* datasource) { return 0; }

	long AudioDecoderOgg::oggTell(void* datasource)
	{
		MyOggIO* io = (MyOggIO*)datasource;
		return (long)io->pos;
	}
	bool AudioDecoderOgg::open(const char* path)
	{
		// ① FileUtils 读数据（ZIP 里有从 ZIP 读，磁盘有从磁盘读）
		Data data = FileUtils::getInstance()->getDataFromFile(path);
		if (data.getSize() == 0)
			return false;

		// ② 存到成员（生命周期覆盖 ogg 使用期）
		_oggData.assign(data.getBytes(), data.getBytes() + data.getSize());

		// ③ 构造内存流
		_oggIO.data = _oggData.data();
		_oggIO.size = _oggData.size();
		_oggIO.pos = 0;

		// ④ 注册回调
		ov_callbacks callbacks;
		callbacks.read_func = oggRead;
		callbacks.seek_func = oggSeek;
		callbacks.close_func = oggClose;
		callbacks.tell_func = oggTell;

		// ⑤ ov_open_callbacks
		if (0 != ov_open_callbacks(&_oggIO, &_vf, nullptr, 0, callbacks))
			return false;

		// ⑥ header
		vorbis_info* vi = ov_info(&_vf, -1);
		_sampleRate = static_cast<uint32_t>(vi->rate);
		_channelCount = vi->channels;
		_bytesPerFrame = vi->channels * sizeof(short);
		_totalFrames = static_cast<uint32_t>(ov_pcm_total(&_vf, -1));
		_isOpened = true;
		return true;
	}
	void AudioDecoderOgg::close()
	{
		if (isOpened())
		{
			ov_clear(&_vf);   // ov_clear 会调 callbacks.close_func
			_isOpened = false;
		}
		// _oggData / _oggIO 可选清理
		_oggData.clear();
		_oggIO.data = nullptr;
		_oggIO.size = 0;
		_oggIO.pos = 0;
	}
	//     bool AudioDecoderOgg::open(const char* path)
	//     {
	//         std::string fullPath = FileUtils::getInstance()->fullPathForFilename(path);
	//         if (0 == ov_fopen(FileUtils::getInstance()->getSuitableFOpen(fullPath).c_str(), &_vf))
	//         {
	//             // header
	//             vorbis_info* vi = ov_info(&_vf, -1);
	//             _sampleRate = static_cast<uint32_t>(vi->rate);
	//             _channelCount = vi->channels;
	//             _bytesPerFrame = vi->channels * sizeof(short);
	//             _totalFrames = static_cast<uint32_t>(ov_pcm_total(&_vf, -1));
	//             _isOpened = true;
	//             return true;
	//         }
	//         return false;
	//     }

	//     void AudioDecoderOgg::close()
	//     {
	//         if (isOpened())
	//         {
	//             ov_clear(&_vf);
	//             _isOpened = false;
	//         }
	//     }

	uint32_t AudioDecoderOgg::read(uint32_t framesToRead, char* pcmBuf)
	{
		int currentSection = 0;
		int bytesToRead = framesToRead * _bytesPerFrame;
		long bytesRead = ov_read(&_vf, pcmBuf, bytesToRead, 0, 2, 1, &currentSection);
		return static_cast<uint32_t>(bytesRead / _bytesPerFrame);
	}

	bool AudioDecoderOgg::seek(uint32_t frameOffset)
	{
		return 0 == ov_pcm_seek(&_vf, frameOffset);
	}

	uint32_t AudioDecoderOgg::tell() const
	{
		return static_cast<uint32_t>(ov_pcm_tell(const_cast<OggVorbis_File*>(&_vf)));
	}

} // namespace orange {
