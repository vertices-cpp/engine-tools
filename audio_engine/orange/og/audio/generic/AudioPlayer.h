

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#ifdef OPENAL_PLAIN_INCLUDES
#include <al.h>
#else
#include <AL/al.h>
#endif
#include "platform/OGPlatformMacros.h"

OG_BEGIN

class AudioCache;
class AudioEngineImpl;

class  AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    void destroy();

    //queue buffer related stuff
    bool setTime(float time);
    float getTime() { return _currTime;}
    bool setLoop(bool loop);

protected:
    void setCache(AudioCache* cache);
    void rotateBufferThread(int offsetFrame);
    bool play2d();

    AudioCache* _audioCache;

    float _volume;
    bool _loop;
    std::function<void (int, const std::string &)> _finishCallbak;

    bool _isDestroyed;
    bool _removeByAudioEngine;
    bool _ready;
    ALuint _alSource;

    //play by circular buffer
    float _currTime;
    bool _streamingSource;
    ALuint _bufferIds[3];
    std::thread* _rotateBufferThread;
    std::condition_variable _sleepCondition;
    std::mutex _sleepMutex;
    bool _timeDirty;
    bool _isRotateThreadExited;

    std::mutex _play2dMutex;

    unsigned int _id;

    friend class AudioEngineImpl;

	// ★ 新增：标记 rotate 线程是否已经因“真播完”而结束
 
	bool _inputEnded = false;
	// ★ 新增：记录“上一次已知的已播时间”，用于断点恢复后对齐
	float _lastKnownPlayTime = 0.0f;
	float _pitch = 1.0f;   // ★ 每个 player 的 pitch
};

OG_END
