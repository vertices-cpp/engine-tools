

//#define LOG_TAG "AudioDecoderManager"

#include "audio/generic/AudioDecoderManager.h"
#include "audio/generic/AudioDecoderOgg.h"
#include "audio/generic/AudioDecoderMp3.h"
#include "audio/generic/AudioMacros.h"
#include "platform/OGFileUtils.h"
//#include "base/OGConsole.h"
#include "mpg123.h"

namespace orange {

static bool __mp3Inited = false;

bool AudioDecoderManager::init()
{
    return true;
}

void AudioDecoderManager::destroy()
{
    AudioDecoderMp3::destroy();
}

AudioDecoder* AudioDecoderManager::createDecoder(const char* path)
{
    std::string suffix = FileUtils::getInstance()->getFileExtension(path);
    if (suffix == ".ogg")
    {
        return new (std::nothrow) AudioDecoderOgg();
    }
    else if (suffix == ".mp3")
    {
        return new (std::nothrow) AudioDecoderMp3();
    }

    return nullptr;
}

void AudioDecoderManager::destroyDecoder(AudioDecoder* decoder)
{
    delete decoder;
}

} // namespace cocos2d {

