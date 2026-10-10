#pragma once
#include "audio/generic/AudioDecoder.h"
 
#include <vector>
 

#include "mpg123.h"


struct mpg123_handle_struct;



namespace orange {

	/**
	 * @brief The class for decoding compressed audio file to PCM buffer.
	 */
	class AudioDecoderMp3 : public AudioDecoder
	{
	public:
		virtual bool open(const char* path) override;
		virtual void close() override;
		virtual uint32_t read(uint32_t framesToRead, char* pcmBuf) override;
		virtual bool seek(uint32_t frameOffset) override;
		virtual uint32_t tell() const override;

	protected:

		AudioDecoderMp3();
		~AudioDecoderMp3();

		static bool lazyInit();
		static void destroy();

		struct mpg123_handle_struct* _mpg123handle;

		friend class AudioDecoderManager;

		//--------------添加回调成员-------------------

		struct MyMP3IO
		{
			const char* data;
			size_t size;
			size_t pos;
		};
		static mpg123_ssize_t myRead(void* handle, void* buf, size_t count);
		static off_t myLseek(void* handle, off_t offset, int whence);
		static void myClose(void* handle);

		std::vector<char> _mp3Data;        // ★ 加这个
		MyMP3IO _mp3IO;                    // ★ 加这个
	};


} // namespace orange {
