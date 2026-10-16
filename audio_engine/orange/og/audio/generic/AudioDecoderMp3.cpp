

#include "audio/generic/AudioDecoderMp3.h"
#include "audio/generic/AudioMacros.h"
#include "platform/OGFileUtils.h"

//#include "base/OGConsole.h"
#include "mpg123.h"

#define LOG_TAG "AudioDecoderMp3"

namespace orange {

	static bool __mp3Inited = false;

	bool AudioDecoderMp3::lazyInit()
	{
		bool ret = true;
		if (!__mp3Inited)
		{
			int error = mpg123_init();
			if (error == MPG123_OK)
			{
				__mp3Inited = true;
			}
			else
			{
				ALOGE("Basic setup goes wrong: %s", mpg123_plain_strerror(error));
				ret = false;
			}
		}
		return ret;
	}

	void AudioDecoderMp3::destroy()
	{
		if (__mp3Inited)
		{
			mpg123_exit();
			__mp3Inited = false;
		}
	}

	AudioDecoderMp3::AudioDecoderMp3()
		: _mpg123handle(nullptr)
	{
		lazyInit();
	}

	AudioDecoderMp3::~AudioDecoderMp3()
	{
		close();
	}

	//--------------添加回调-------------------

	mpg123_ssize_t AudioDecoderMp3::myRead(void* handle, void* buf, size_t count)
	{
		MyMP3IO* io = (AudioDecoderMp3::MyMP3IO*)handle;
		size_t remain = io->size - io->pos;
		if (count > remain) count = remain;
		memcpy(buf, io->data + io->pos, count);
		io->pos += count;
		return (mpg123_ssize_t)count;
	}

	off_t AudioDecoderMp3::myLseek(void* handle, off_t offset, int whence)
	{
		MyMP3IO* io = (MyMP3IO*)handle;
		if (whence == SEEK_SET)      io->pos = (size_t)offset;
		else if (whence == SEEK_CUR) io->pos += (size_t)offset;
		else if (whence == SEEK_END) io->pos = io->size + (size_t)offset;
		if (io->pos > io->size) io->pos = io->size;
		return (off_t)io->pos;
	}
	void AudioDecoderMp3::myClose(void* handle) { /* 内存流，什么都不做 */ }

	bool AudioDecoderMp3::open(const char* path)
	{
		long rate = 0;
		int error = MPG123_OK;
		int mp3Encoding = 0;
		int channel = 0;
		bool opened = false;

		_mpg123handle = mpg123_new(nullptr, &error);
		if (nullptr == _mpg123handle) return false;

		do
		{
			// ① FileUtils 读数据（ZIP 里有从 ZIP 读，磁盘有从磁盘读）
			Data data = FileUtils::getInstance()->getDataFromFile(path);
			if (data.getSize() == 0) break;

			// ② 存到成员（生命周期要覆盖 mpg123 使用期）
			_mp3Data.assign(data.getBytes(), data.getBytes() + data.getSize());

			// ③ 构造内存流
			_mp3IO.data = _mp3Data.data();
			_mp3IO.size = _mp3Data.size();
			_mp3IO.pos = 0;

			// ④ 注册回调 + open_handle
			mpg123_replace_reader_handle(_mpg123handle, myRead, myLseek, myClose);
			if (mpg123_open_handle(_mpg123handle, &_mp3IO) != MPG123_OK)
			{
				ALOGE("mpg123_open_handle failed");
				break;
			}

			// ⑤ 后面 getformat / format / scan / length 都一样
			if (mpg123_getformat(_mpg123handle, &rate, &channel, &mp3Encoding) != MPG123_OK)
				break;

			_channelCount = channel;
			_sampleRate = rate;

			if (mp3Encoding == MPG123_ENC_SIGNED_16)
				_bytesPerFrame = 2 * _channelCount;
			else if (mp3Encoding == MPG123_ENC_FLOAT_32)
				_bytesPerFrame = 4 * _channelCount;
			else
				break;

			mpg123_format_none(_mpg123handle);
			mpg123_format(_mpg123handle, rate, channel, mp3Encoding);
			mpg123_scan(_mpg123handle);
			_totalFrames = mpg123_length(_mpg123handle);

			opened = true;
		} while (false);

		if (!opened)
		{
			if (_mpg123handle)
			{
				mpg123_close(_mpg123handle);
				mpg123_delete(_mpg123handle);
				_mpg123handle = nullptr;
			}
			return false;
		}

		_isOpened = true;
		return true;
	}
	//---------------------------------

//     bool AudioDecoderMp3::open(const char* path)
//     {
//         std::string fullPath = FileUtils::getInstance()->fullPathForFilename(path);
// 
//         long rate = 0;
//         int error = MPG123_OK;
//         int mp3Encoding = 0;
//         int channel = 0;
//         do
//         {
//             _mpg123handle = mpg123_new(nullptr, &error);
//             if (nullptr == _mpg123handle)
//             {
//                 ALOGE("Basic setup goes wrong: %s", mpg123_plain_strerror(error));
//                 break;
//             }
// 
//             if (mpg123_open(_mpg123handle, FileUtils::getInstance()->getSuitableFOpen(fullPath).c_str()) != MPG123_OK
//                 || mpg123_getformat(_mpg123handle, &rate, &channel, &mp3Encoding) != MPG123_OK)
//             {
//                 ALOGE("Trouble with mpg123: %s\n", mpg123_strerror(_mpg123handle) );
//                 break;
//             }
// 
//             _channelCount = channel;
//             _sampleRate = rate;
// 
//             if (mp3Encoding == MPG123_ENC_SIGNED_16)
//             {
//                 _bytesPerFrame = 2 * _channelCount;
//             }
//             else if (mp3Encoding == MPG123_ENC_FLOAT_32)
//             {
//                 _bytesPerFrame = 4 * _channelCount;
//             }
//             else
//             {
//                 ALOGE("Bad encoding: 0x%x!\n", mp3Encoding);
//                 break;
//             }
// 
//             /* Ensure that this output format will not change (it could, when we allow it). */
//             mpg123_format_none(_mpg123handle);
//             mpg123_format(_mpg123handle, rate, channel, mp3Encoding);
//             /* Ensure that we can get accurate length by call mpg123_length */
//             mpg123_scan(_mpg123handle);
// 
//             _totalFrames = mpg123_length(_mpg123handle);
// 
//             _isOpened = true;
//             return true;
//         } while (false);
// 
//         if (_mpg123handle != nullptr)
//         {
//             mpg123_close(_mpg123handle);
//             mpg123_delete(_mpg123handle);
//             _mpg123handle = nullptr;
//         }
//         return false;
//     }

	void AudioDecoderMp3::close()
	{
		if (isOpened())
		{
			if (_mpg123handle != nullptr)
			{
				mpg123_close(_mpg123handle);
				mpg123_delete(_mpg123handle);
				_mpg123handle = nullptr;
			}
			_isOpened = false;
		}
	}

	uint32_t AudioDecoderMp3::read(uint32_t framesToRead, char* pcmBuf)
	{
		int bytesToRead = framesToRead * _bytesPerFrame;
		::size_t bytesRead = 0;
		int err = mpg123_read(_mpg123handle, (unsigned char*)pcmBuf, bytesToRead, &bytesRead);
		if (err == MPG123_ERR)
		{
			ALOGE("Trouble with mpg123: %s\n", mpg123_strerror(_mpg123handle));
			return 0;
		}

		return static_cast<uint32_t>(bytesRead / _bytesPerFrame);
	}

	bool AudioDecoderMp3::seek(uint32_t frameOffset)
	{
		off_t offset = mpg123_seek(_mpg123handle, frameOffset, SEEK_SET);
		//ALOGD("mpg123_seek return: %d", (int)offset);
		if (offset >= 0 && offset == frameOffset)
		{
			return true;
		}
		return false;
	}

	uint32_t AudioDecoderMp3::tell() const
	{
		return static_cast<uint32_t>(mpg123_tell(_mpg123handle));
	}

}// namespace orange {
