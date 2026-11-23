
#include "audio/generic/AudioDecoder.h"
#include "audio/generic/AudioMacros.h"
#include <string>
//#include "platform/OGFileUtils.h"

//#define LOG_TAG "AudioDecoder"

namespace orange {

AudioDecoder::AudioDecoder()
    : _isOpened(false)
    , _totalFrames(0)
    , _bytesPerFrame(0)
    , _sampleRate(0)
    , _channelCount(0)
    {

    }

    AudioDecoder::~AudioDecoder()
    {
    }


    bool AudioDecoder::isOpened() const
    {
        return _isOpened;
    }

    uint32_t AudioDecoder::readFixedFrames(uint32_t framesToRead, char* pcmBuf)
    {
        uint32_t framesRead = 0;
        uint32_t framesReadOnce = 0;
        do
        {
            framesReadOnce = read(framesToRead - framesRead, pcmBuf + framesRead * _bytesPerFrame);
            framesRead += framesReadOnce;
        } while (framesReadOnce != 0 && framesRead < framesToRead);

        if (framesRead < framesToRead)
        {
            memset(pcmBuf + framesRead * _bytesPerFrame, 0x00, (framesToRead - framesRead) * _bytesPerFrame);
        }

        return framesRead;
    }

    uint32_t AudioDecoder::getTotalFrames() const
    {
        return _totalFrames;
    }

    uint32_t AudioDecoder::getBytesPerFrame() const
    {
        return _bytesPerFrame;
    }

    uint32_t AudioDecoder::getSampleRate() const
    {
        return _sampleRate;
    }

    uint32_t AudioDecoder::getChannelCount() const
    {
        return _channelCount;
    }

} // namespace cocos2d {
