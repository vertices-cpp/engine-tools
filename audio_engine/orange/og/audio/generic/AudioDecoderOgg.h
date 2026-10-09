
#include "audio/generic/AudioDecoder.h"

#include "vorbis/vorbisfile.h"
#include "mpg123.h"
#include <vector>
 

namespace orange {

	/**
	 * @brief The class for decoding compressed audio file to PCM buffer.
	 */
	class AudioDecoderOgg : public AudioDecoder
	{
	public:
		virtual bool open(const char* path) override;

		virtual void close() override;
		virtual uint32_t read(uint32_t framesToRead, char* pcmBuf) override;
		virtual bool seek(uint32_t frameOffset) override;
		virtual uint32_t tell() const override;

	protected:
		AudioDecoderOgg();
		~AudioDecoderOgg();

		OggVorbis_File _vf;

		friend class AudioDecoderManager;

		//--------------添加回调成员-------------------

		struct MyOggIO
		{
			const char* data = nullptr;
			size_t size = 0;
			size_t pos = 0;
		};
		static size_t  oggRead(void* ptr, size_t size, size_t nmemb, void* datasource);
		static int oggSeek(void* datasource, ogg_int64_t offset, int whence);
		static int oggClose(void* datasource);
		static long oggTell(void* datasource);
		std::vector<char> _oggData;   // ★ 内存数据
		MyOggIO _oggIO;
	};

} // namespace orange {
