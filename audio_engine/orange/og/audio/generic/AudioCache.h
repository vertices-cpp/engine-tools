

//#include "platform/OGPlatformConfig.h"


#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#ifdef OPENAL_PLAIN_INCLUDES
#include <al.h>
#else
#include <AL/al.h>
#endif
#include "platform/OGPlatformMacros.h"
#include "audio/generic/AudioMacros.h"

OG_BEGIN

class AudioEngineImpl;
class AudioPlayer;

class AudioCache
{
public:

    enum class State
    {
        INITIAL,
        LOADING,
        READY,
        FAILED
    };

    AudioCache();
    ~AudioCache();

    void addPlayCallback(const std::function<void()>& callback);

    void addLoadCallback(const std::function<void(bool)>& callback);

protected:
    void setSkipReadDataTask(bool isSkip) { _isSkipReadDataTask = isSkip; };
    void readDataTask(unsigned int selfId);

    void invokingPlayCallbacks();

    void invokingLoadCallbacks();

    //pcm data related stuff
    ALenum _format;
    ALsizei _sampleRate;
    float _duration;
    uint32_t _totalFrames;
    uint32_t _framesRead;

    /*Cache related stuff;
     * Cache pcm data when sizeInBytes less than PCMDATA_CACHEMAXSIZE
     */
    ALuint _alBufferId;
    char* _pcmData;

    /*Queue buffer related stuff
     *  Streaming in OpenAL when sizeInBytes greater then PCMDATA_CACHEMAXSIZE
     */
    char* _queBuffers[QUEUEBUFFER_NUM];
    ALsizei _queBufferSize[QUEUEBUFFER_NUM];
    uint32_t _queBufferFrames;

    std::mutex _playCallbackMutex;
    std::vector< std::function<void()> > _playCallbacks;

    // loadCallbacks doesn't need mutex since it's invoked only in Cocos thread.
    std::vector< std::function<void(bool)> > _loadCallbacks;

    std::mutex _readDataTaskMutex;

    State _state;

    std::shared_ptr<bool> _isDestroyed;
    std::string _fileFullPath;
    unsigned int _id;
    bool _isLoadingFinished;
    bool _isSkipReadDataTask;

    friend class AudioEngineImpl;
    friend class AudioPlayer;
};

OG_END

