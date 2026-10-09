
#include "audio/generic/AudioEngineImpl.h"

#ifdef OPENAL_PLAIN_INCLUDES
#include "alc.h"
#include "alext.h"
#else
#include "AL/alc.h"
#include "AL/alext.h"
#endif
#include "audio/include/AudioEngine.h"
#include "base/OGDirector.h"
#include "base/OGScheduler.h"
#include "platform/OGFileUtils.h"
#include "audio/generic/AudioDecoderManager.h"
#include <stdarg.h>
//#include <windows.h>

#define LOG_TAG "AudioEngine-Win32"


// is needed. Define the following macros (ALOGV, ALOGD, ALOGI, ALOGW, ALOGE) for threadsafe log output.

//FIXME: Move _winLog, winLog to a separated file

static void _winLog(const char *format, va_list args)
{
    static const int MAX_LOG_LENGTH = 16 * 1024;
    int bufferSize = MAX_LOG_LENGTH;
    char* buf = nullptr;

    do
    {
        buf = new (std::nothrow) char[bufferSize];
        if (buf == nullptr)
            return; // not enough memory

        int ret = vsnprintf(buf, bufferSize - 3, format, args);
        if (ret < 0)
        {
            bufferSize *= 2;

            delete[] buf;
        }
        else
            break;

    } while (true);

    strcat(buf, "\n");

    int pos = 0;
    int len = (int)strlen(buf);

    do
    {
        int chunkLen = (len - pos > MAX_LOG_LENGTH) ? MAX_LOG_LENGTH : (len - pos);

        // 直接写入标准输出，替代 Windows 专有的 OutputDebugStringW
        fwrite(buf + pos, 1, chunkLen, stdout);

        pos += MAX_LOG_LENGTH;

    } while (pos < len);

    fflush(stdout);

    delete[] buf;
}

void audioLog(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    _winLog(format, args);
    va_end(args);
}

using namespace orange;

static ALCdevice *s_ALDevice = nullptr;
static ALCcontext *s_ALContext = nullptr;

AudioEngineImpl::AudioEngineImpl()
: _lazyInitLoop(true)
, _currentAudioID(0)
, _scheduler(nullptr)
{

}

AudioEngineImpl::~AudioEngineImpl()
{
    if (_scheduler != nullptr)
    {
        _scheduler->unschedule(OG_SCHEDULE_SELECTOR(AudioEngineImpl::update), this);
    }

    if (s_ALContext) {
        alDeleteSources(MAX_AUDIOINSTANCES, _alSources);

        _audioCaches.clear();

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(s_ALContext);
        s_ALContext = nullptr;
    }

    if (s_ALDevice) {
        alcCloseDevice(s_ALDevice);
        s_ALDevice = nullptr;
    }

    AudioDecoderManager::destroy();
}

bool AudioEngineImpl::init()
{
    bool ret = false;
    do{
        s_ALDevice = alcOpenDevice(nullptr);

        if (s_ALDevice) {
            alGetError();
            s_ALContext = alcCreateContext(s_ALDevice, nullptr);
            alcMakeContextCurrent(s_ALContext);

            alGenSources(MAX_AUDIOINSTANCES, _alSources);
            auto alError = alGetError();
            if(alError != AL_NO_ERROR)
            {
                ALOGE("%s:generating sources failed! error = %x\n", __FUNCTION__, alError);
                break;
            }

            for (int i = 0; i < MAX_AUDIOINSTANCES; ++i) {
                _alSourceUsed[_alSources[i]] = false;
            }

            _scheduler = Director::getInstance()->getScheduler();
            ret = AudioDecoderManager::init();
            ALOGI("OpenAL was initialized successfully!");
        }
    }while (false);

    return ret;
}

AudioCache* AudioEngineImpl::preload(const std::string& filePath, std::function<void(bool)> callback)
{
    AudioCache* audioCache = nullptr;

    auto it = _audioCaches.find(filePath);
    if (it == _audioCaches.end()) {
        audioCache = &_audioCaches[filePath];
        audioCache->_fileFullPath = FileUtils::getInstance()->fullPathForFilename(filePath);
        unsigned int cacheId = audioCache->_id;
        auto isCacheDestroyed = audioCache->_isDestroyed;
        AudioEngine::addTask([audioCache, cacheId, isCacheDestroyed](){
            if (*isCacheDestroyed)
            {
                ALOGV("AudioCache (id=%u) was destroyed, no need to launch readDataTask.", cacheId);
                audioCache->setSkipReadDataTask(true);
                return;
            }
            audioCache->readDataTask(cacheId);
        });
    }
    else {
        audioCache = &it->second;
    }

    if (audioCache && callback)
    {
        audioCache->addLoadCallback(callback);
    }
    return audioCache;
}

int AudioEngineImpl::play2d(const std::string &filePath ,bool loop ,float volume)
{
    if (s_ALDevice == nullptr) {
        return AudioEngine::INVALID_AUDIO_ID;
    }

    bool sourceFlag = false;
    ALuint alSource = 0;
    for (int i = 0; i < MAX_AUDIOINSTANCES; ++i) {
        alSource = _alSources[i];

        if ( !_alSourceUsed[alSource]) {
            sourceFlag = true;
            break;
        }
    }
    if(!sourceFlag){
        return AudioEngine::INVALID_AUDIO_ID;
    }

    auto player = new (std::nothrow) AudioPlayer;
    if (player == nullptr) {
        return AudioEngine::INVALID_AUDIO_ID;
    }

	player->_alSource = alSource;
	player->_loop = loop;
	player->_volume = volume;
	player->_pitch = _globalPitch;   // ★ 新建的 player 继承全局 pitch

    auto audioCache = preload(filePath, nullptr);
    if (audioCache == nullptr) {
        delete player;
        return AudioEngine::INVALID_AUDIO_ID;
    }

    player->setCache(audioCache);
    _threadMutex.lock();
    _audioPlayers[_currentAudioID] = player;
    _threadMutex.unlock();

    _alSourceUsed[alSource] = true;

    audioCache->addPlayCallback(std::bind(&AudioEngineImpl::_play2d,this,audioCache,_currentAudioID));

    if (_lazyInitLoop) {
        _lazyInitLoop = false;
        _scheduler->schedule(OG_SCHEDULE_SELECTOR(AudioEngineImpl::update), this, 0.05f, false);
    }
 
    return _currentAudioID++;
}

void AudioEngineImpl::_play2d(AudioCache *cache, int audioID)
{
    //Note: It may bn in sub thread or main thread :(
    if (!*cache->_isDestroyed && cache->_state == AudioCache::State::READY)
    {
        _threadMutex.lock();
        auto playerIt = _audioPlayers.find(audioID);
		if (playerIt != _audioPlayers.end() && playerIt->second->play2d()) {
			_scheduler->performFunctionInOrangeThread([audioID]() {

				if (AudioEngine::_audioIDInfoMap.find(audioID) != AudioEngine::_audioIDInfoMap.end()) {
					AudioEngine::_audioIDInfoMap[audioID].state = AudioEngine::AudioState::PLAYING;
					// ★ 如果之前 pauseAll 给它打过标记，立刻暂停
					if (AudioEngine::_audioIDInfoMap[audioID].pauseRequested) {
						AudioEngine::pause(audioID);
						AudioEngine::_audioIDInfoMap[audioID].pauseRequested = false;
					}
				}
			});
		}
        _threadMutex.unlock();
    }
    else
    {
        ALOGD("AudioEngineImpl::_play2d, cache was destroyed or not ready!");
        auto iter = _audioPlayers.find(audioID);
        if (iter != _audioPlayers.end())
        {
            iter->second->_removeByAudioEngine = true;
        }
    }
}

void AudioEngineImpl::setVolume(int audioID,float volume)
{
    auto player = _audioPlayers[audioID];
    player->_volume = volume;

    if (player->_ready) {
        alSourcef(_audioPlayers[audioID]->_alSource, AL_GAIN, volume);

        auto error = alGetError();
        if (error != AL_NO_ERROR) {
            ALOGE("%s: audio id = %d, error = %x", __FUNCTION__,audioID,error);
        }
    }
}

void AudioEngineImpl::setLoop(int audioID, bool loop)
{
    auto player = _audioPlayers[audioID];

    if (player->_ready) {
        if (player->_streamingSource) {
            player->setLoop(loop);
        } else {
            if (loop) {
                alSourcei(player->_alSource, AL_LOOPING, AL_TRUE);
            } else {
                alSourcei(player->_alSource, AL_LOOPING, AL_FALSE);
            }

            auto error = alGetError();
            if (error != AL_NO_ERROR) {
                ALOGE("%s: audio id = %d, error = %x", __FUNCTION__,audioID,error);
            }
        }
    }
    else {
        player->_loop = loop;
    }
}

bool AudioEngineImpl::pause(int audioID)
{
    bool ret = true;
    alSourcePause(_audioPlayers[audioID]->_alSource);

    auto error = alGetError();
    if (error != AL_NO_ERROR) {
        ret = false;
        ALOGE("%s: audio id = %d, error = %x\n", __FUNCTION__,audioID,error);
    }

    return ret;
}

bool AudioEngineImpl::resume(int audioID)
{
    bool ret = true;
    alSourcePlay(_audioPlayers[audioID]->_alSource);

    auto error = alGetError();
    if (error != AL_NO_ERROR) {
        ret = false;
        ALOGE("%s: audio id = %d, error = %x\n", __FUNCTION__,audioID,error);
    }

    return ret;
}

void AudioEngineImpl::stop(int audioID)
{
    auto player = _audioPlayers[audioID];
    player->destroy();
    //Note: Don't set the flag to false here, it should be set in 'update' function.
    // Otherwise, the state got from alSourceState may be wrong
//    _alSourceUsed[player->_alSource] = false;

    // Call 'update' method to cleanup immediately since the schedule may be cancelled without any notification.
    update(0.0f);
}

void AudioEngineImpl::stopAll()
{
    for(auto&& player : _audioPlayers)
    {
        player.second->destroy();
    }
    //Note: Don't set the flag to false here, it should be set in 'update' function.
    // Otherwise, the state got from alSourceState may be wrong
//    for(int index = 0; index < MAX_AUDIOINSTANCES; ++index)
//    {
//        _alSourceUsed[_alSources[index]] = false;
//    }

    // Call 'update' method to cleanup immediately since the schedule may be cancelled without any notification.
    update(0.0f);
}

float AudioEngineImpl::getDuration(int audioID)
{
    auto player = _audioPlayers[audioID];
    if(player->_ready){
        return player->_audioCache->_duration;
    } else {
        return AudioEngine::TIME_UNKNOWN;
    }
}

float AudioEngineImpl::getCurrentTime(int audioID)
{
    float ret = 0.0f;
    auto player = _audioPlayers[audioID];
    if(player->_ready){
        if (player->_streamingSource) {
            ret = player->getTime();
        } else {
            alGetSourcef(player->_alSource, AL_SEC_OFFSET, &ret);

            auto error = alGetError();
            if (error != AL_NO_ERROR) {
                ALOGE("%s, audio id:%d,error code:%x", __FUNCTION__,audioID,error);
            }
        }
    }

    return ret;
}

bool AudioEngineImpl::setCurrentTime(int audioID, float time)
{
    bool ret = false;
    auto player = _audioPlayers[audioID];

    do {
        if (!player->_ready) {
            break;
        }

        if (player->_streamingSource) {
            ret = player->setTime(time);
            break;
        }
        else {
            if (player->_audioCache->_framesRead != player->_audioCache->_totalFrames &&
                (time * player->_audioCache->_sampleRate) > player->_audioCache->_framesRead) {
                ALOGE("%s: audio id = %d", __FUNCTION__,audioID);
                break;
            }

            alSourcef(player->_alSource, AL_SEC_OFFSET, time);

            auto error = alGetError();
            if (error != AL_NO_ERROR) {
                ALOGE("%s: audio id = %d, error = %x", __FUNCTION__,audioID,error);
            }
            ret = true;
        }
    } while (0);

    return ret;
}

void AudioEngineImpl::setFinishCallback(int audioID, const std::function<void (int, const std::string &)> &callback)
{
    _audioPlayers[audioID]->_finishCallbak = callback;
}
void AudioEngineImpl::update(float dt)
{
	ALint sourceState;
	int audioID;
	AudioPlayer* player;
	ALuint alSource;

	// 遍历所有活跃的 AudioPlayer
	for (auto it = _audioPlayers.begin(); it != _audioPlayers.end(); ) {
		audioID = it->first;
		player = it->second;
		alSource = player->_alSource;
		alGetSourcei(alSource, AL_SOURCE_STATE, &sourceState);

		// ------------------------------------------------------------
		// 情况 1：player 被标记为“该移除”（比如 play2d 失败、destroy 后）
		// 直接从 map 里删掉，释放 source
		// ------------------------------------------------------------
		if (player->_removeByAudioEngine)
		{
			AudioEngine::remove(audioID);
			_threadMutex.lock();
			it = _audioPlayers.erase(it);
			_threadMutex.unlock();
			delete player;
			_alSourceUsed[alSource] = false;
		}

		// ------------------------------------------------------------
		// 情况 2：player 已就绪，但 Source 处于 AL_STOPPED
		// 关键：STOPPED 有两种可能，必须区分！
		//   a) 饥饿停止：投喂线程来不及补数据，队列被消耗光 → 应该重拉
		//   b) 真播完：非循环音频解码到文件尾，数据真的放完了 → 应该删除
		// 区分依据：queued（队列里还有没有 buffer）+ _inputEnded（解码器是否读到尾）
		// ------------------------------------------------------------
		else if (player->_ready && sourceState == AL_STOPPED) {

			ALint queued = 0;
			alGetSourcei(alSource, AL_BUFFERS_QUEUED, &queued);

			// 【关键改动】饥饿停止：不删，跳过，交给 rotateBufferThread 重拉
			// 条件全部满足才是“饥饿”：
			//   - _streamingSource：是流式播放（非整块 buffer 那种）
			//   - queued > 0：队列里还有没播完的 buffer（说明数据没放完）
			//   - !_inputEnded：解码器还没读到文件尾
			// 满足则说明只是“暂时没喂上”，不是“播放结束”。
			// 注意：这里只是 ++it; continue; —— 不调 alSourcePlay
			//       重拉的动作由 rotateBufferThread 负责，避免双重触发。
			if (player->_streamingSource && queued > 0 && !player->_inputEnded) {
				++it;
				continue;   // 饥饿，不删
			}

			// 【原逻辑】真播完，删除 player
			// 到这里说明：queue 空了，或者输入已经结束，是真的播放完毕。
			// 取出文件路径用于回调
			std::string filePath;
			if (player->_finishCallbak) {
				auto& audioInfo = AudioEngine::_audioIDInfoMap[audioID];
				filePath = audioInfo.filePath;
			}

			// 从全局 map 和 _audioPlayers 里移除
			AudioEngine::remove(audioID);
			_threadMutex.lock();
			it = _audioPlayers.erase(it);
			_threadMutex.unlock();

			// 触发播放完成回调
			if (player->_finishCallbak) {
				player->_finishCallbak(audioID, filePath);
			}

			// 释放 player 和 source
			delete player;
			_alSourceUsed[alSource] = false;
		}

		// ------------------------------------------------------------
		// 情况 3：正常播放中（AL_PLAYING / AL_PAUSED / AL_INITIAL），继续遍历
		// ------------------------------------------------------------
		else {
			++it;
		}
	}

	// 如果所有 player 都没了，取消定时器，节省 CPU
	// 下一次 play2d 会重新 schedule
	if (_audioPlayers.empty()) {
		_lazyInitLoop = true;
		_scheduler->unschedule(OG_SCHEDULE_SELECTOR(AudioEngineImpl::update), this);
	}
}

void AudioEngineImpl::uncache(const std::string &filePath)
{
    _audioCaches.erase(filePath);
}

void AudioEngineImpl::uncacheAll()
{
    _audioCaches.clear();
}


void AudioEngineImpl::setGlobalPitch(float pitch)
{
	if (pitch < 0.1f) 
		pitch = 0.1f;
	if (pitch > 4.0f) 
		pitch = 4.0f;

	_globalPitch = pitch;

	// 遍历所有 player：能改的立刻改 Source，没就绪的只改 _pitch
	for (auto& pair : _audioPlayers) {
		AudioPlayer* player = pair.second;
		player->_pitch = pitch;

		if (player->_ready) {
			alSourcef(player->_alSource, AL_PITCH, pitch);
			CHECK_AL_ERROR_DEBUG();
		}
	}
}