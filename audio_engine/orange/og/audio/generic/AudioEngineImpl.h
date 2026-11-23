

#include <unordered_map>

#include "base/OGRef.h"
#include "audio/generic/AudioCache.h"
#include "audio/generic/AudioPlayer.h"

OG_BEGIN

class Scheduler;

#define MAX_AUDIOINSTANCES 32



class AudioEngineImpl : public orange::Ref
{
public:
    AudioEngineImpl();
    ~AudioEngineImpl();

    bool init();
    int play2d(const std::string &fileFullPath ,bool loop ,float volume);
    void setVolume(int audioID,float volume);
    void setLoop(int audioID, bool loop);
    bool pause(int audioID);
    bool resume(int audioID);
    void stop(int audioID);
    void stopAll();
    float getDuration(int audioID);
    float getCurrentTime(int audioID);
    bool setCurrentTime(int audioID, float time);
    void setFinishCallback(int audioID, const std::function<void (int, const std::string &)> &callback);

    void uncache(const std::string& filePath);
    void uncacheAll();
    AudioCache* preload(const std::string& filePath, std::function<void(bool)> callback);
    void update(float dt);

	void setGlobalPitch(float pitch);
	float getGlobalPitch() const {
		return _globalPitch;
	}

	float _globalPitch = 1.0f;   // ★ 全局 pitch 意图

private:
    void _play2d(AudioCache *cache, int audioID);

    ALuint _alSources[MAX_AUDIOINSTANCES];

    //source,used
    std::unordered_map<ALuint, bool> _alSourceUsed;

    //filePath,bufferInfo
    std::unordered_map<std::string, AudioCache> _audioCaches;

    //audioID,AudioInfo
    std::unordered_map<int, AudioPlayer*>  _audioPlayers;
    std::mutex _threadMutex;

    bool _lazyInitLoop;

    int _currentAudioID;
    Scheduler* _scheduler;
};

OG_END
