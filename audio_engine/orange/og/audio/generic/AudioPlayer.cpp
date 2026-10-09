
#include "audio/generic/AudioPlayer.h"
#include "audio/generic/AudioCache.h"
//#include "platform/OGFileUtils.h"
#include "audio/generic/AudioDecoderManager.h"
#include "audio/generic/Audiodecoder.h"

#define VERY_VERY_VERBOSE_LOGGING
#ifdef VERY_VERY_VERBOSE_LOGGING
#define ALOGVV ALOGV
#else
#define ALOGVV(...) do{} while(false)
#endif

#define LOG_TAG "AudioPlayer"
#include <assert.h>

using namespace orange;

namespace {
unsigned int __idIndex = 0;
}

AudioPlayer::AudioPlayer()
: _audioCache(nullptr)
, _finishCallbak(nullptr)
, _isDestroyed(false)
, _removeByAudioEngine(false)
, _ready(false)
, _currTime(0.0f)
, _streamingSource(false)
, _rotateBufferThread(nullptr)
, _timeDirty(false)
, _isRotateThreadExited(false)
, _id(++__idIndex)
{
    memset(_bufferIds, 0, sizeof(_bufferIds));
}

AudioPlayer::~AudioPlayer()
{
	//printf("=== end() entered ===\n");
    ALOGVV("~AudioPlayer() (%p), id=%u", this, _id);
    destroy();

    if (_streamingSource)
    {
        alDeleteBuffers(3, _bufferIds);
    }
}

void AudioPlayer::destroy()
{
    if (_isDestroyed)
        return;

    ALOGVV("AudioPlayer::destroy begin, id=%u", _id);

    _isDestroyed = true;

    do
    {
        if (_audioCache != nullptr)
        {
            if (_audioCache->_state == AudioCache::State::INITIAL)
            {
                ALOGV("AudioPlayer::destroy, id=%u, cache isn't ready!", _id);
                break;
            }

            while (!_audioCache->_isLoadingFinished)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        // Wait for play2d to be finished.
        _play2dMutex.lock();
        _play2dMutex.unlock();

        if (_streamingSource)
        {
            if (_rotateBufferThread != nullptr)
            {
                while (!_isRotateThreadExited)
                {
                    _sleepCondition.notify_one();
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }

                if (_rotateBufferThread->joinable()) {
                    _rotateBufferThread->join();
                }

                delete _rotateBufferThread;
                _rotateBufferThread = nullptr;
                ALOGVV("rotateBufferThread exited!");
            }
        }
    } while(false);

    ALOGVV("Before alSourceStop");
    alSourceStop(_alSource); CHECK_AL_ERROR_DEBUG();
    ALOGVV("Before alSourcei");
    alSourcei(_alSource, AL_BUFFER, NULL); CHECK_AL_ERROR_DEBUG();

    _removeByAudioEngine = true;

    _ready = false;
    ALOGVV("AudioPlayer::destroy end, id=%u", _id);
}

void AudioPlayer::setCache(AudioCache* cache)
{
    _audioCache = cache;
}

bool AudioPlayer::play2d()
{
    _play2dMutex.lock();
    ALOGV("AudioPlayer::play2d, _alSource: %u, player id=%u", _alSource, _id);

    /*********************************************************************/
    /*       Note that it may be in sub thread or in main thread.       **/
    /*********************************************************************/
    bool ret = false;
    do
    {
        if (_audioCache->_state != AudioCache::State::READY)
        {
            ALOGE("alBuffer isn't ready for play!");
            break;
        }

        alSourcei(_alSource, AL_BUFFER, 0);CHECK_AL_ERROR_DEBUG();
		alSourcef(_alSource, AL_PITCH, _pitch); CHECK_AL_ERROR_DEBUG();// 1.0f 改成 _pitch。
        alSourcef(_alSource, AL_GAIN, _volume);CHECK_AL_ERROR_DEBUG();
        alSourcei(_alSource, AL_LOOPING, AL_FALSE);CHECK_AL_ERROR_DEBUG();

        if (_audioCache->_queBufferFrames == 0)
        {
            if (_loop) {
                alSourcei(_alSource, AL_LOOPING, AL_TRUE);
                CHECK_AL_ERROR_DEBUG();
            }
        }
        else
        {
            alGenBuffers(3, _bufferIds);

            auto alError = alGetError();
            if (alError == AL_NO_ERROR)
            {
                for (int index = 0; index < QUEUEBUFFER_NUM; ++index)
                {
                    alBufferData(_bufferIds[index], _audioCache->_format, _audioCache->_queBuffers[index], _audioCache->_queBufferSize[index], _audioCache->_sampleRate);
                }
                CHECK_AL_ERROR_DEBUG();
            }
            else
            {
                ALOGE("%s:alGenBuffers error code:%x", __FUNCTION__,alError);
                break;
            }
            _streamingSource = true;
        }

        {
            std::unique_lock<std::mutex> lk(_sleepMutex);
            if (_isDestroyed)
                break;

            if (_streamingSource)
            {
                alSourceQueueBuffers(_alSource, QUEUEBUFFER_NUM, _bufferIds);
                CHECK_AL_ERROR_DEBUG();
                _rotateBufferThread = new std::thread(&AudioPlayer::rotateBufferThread, this, _audioCache->_queBufferFrames * QUEUEBUFFER_NUM + 1);
            }
            else
            {
                alSourcei(_alSource, AL_BUFFER, _audioCache->_alBufferId);
                CHECK_AL_ERROR_DEBUG();
            }

            alSourcePlay(_alSource);
        }

        auto alError = alGetError();
        if (alError != AL_NO_ERROR)
        {
            ALOGE("%s:alSourcePlay error code:%x", __FUNCTION__,alError);
            break;
        }

        ALint state;
        alGetSourcei(_alSource, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING)
        {
            ALOGE("state isn't playing, %d, %s, cache id=%u, player id=%u", state, _audioCache->_fileFullPath.c_str(), _audioCache->_id, _id);
        }
        assert(state == AL_PLAYING);
        _ready = true;
        ret = true;
    } while (false);

    if (!ret)
    {
        _removeByAudioEngine = true;
    }

    _play2dMutex.unlock();
    return ret;
}

void AudioPlayer::rotateBufferThread(int offsetFrame)
{
	char* tmpBuffer = nullptr;
	AudioDecoder* decoder = AudioDecoderManager::createDecoder(_audioCache->_fileFullPath.c_str());
	do
	{
		BREAK_IF(decoder == nullptr || !decoder->open(_audioCache->_fileFullPath.c_str()));

		uint32_t framesRead = 0;
		const uint32_t framesToRead = _audioCache->_queBufferFrames;   // 每个 buffer 的帧数
		const uint32_t bufferSize = framesToRead * decoder->getBytesPerFrame();
		tmpBuffer = (char*)malloc(bufferSize);
		memset(tmpBuffer, 0, bufferSize);

		if (offsetFrame != 0) {
			decoder->seek(offsetFrame);   // 从指定位置开始解码
		}

		ALint sourceState;
		ALint bufferProcessed = 0;
		bool needToExitThread = false;

		// ============================================================
		// 投喂主循环：不断把解码出来的 PCM 数据填进 OpenAL 的 buffer 队列
		// 每 75ms 醒一次，检查有没有 buffer 播完，播完就 unqueue/refill/queue
		// ============================================================
		while (!_isDestroyed) {

			// 【关键改动 1】不再判断 Source 是不是 AL_PLAYING
			// 原版代码：if (sourceState == AL_PLAYING) { ... }
			// 问题：断点期间 Source 被音频服务消耗到 STOPPED，恢复后这个 if 不成立，
			//       投喂线程就永远不再补数据，音频永久静音。
			// 改法：无条件查 AL_BUFFERS_PROCESSED，不管 Source 什么状态都补。
			alGetSourcei(_alSource, AL_BUFFERS_PROCESSED, &bufferProcessed);

			while (bufferProcessed > 0) {
				bufferProcessed--;

				if (_timeDirty) {
					// 用户主动 seek：按 _currTime 重新定位解码器
					_timeDirty = false;
					offsetFrame = (int)(_currTime * decoder->getSampleRate());
					decoder->seek(offsetFrame);
				}
				else {
					// 正常推进：每播完一个 buffer，时间轴前进一个 buffer 时长
					_currTime += QUEUEBUFFER_TIME_STEP;
					if (_currTime > _audioCache->_duration) {
						if (_loop) _currTime = 0.0f;
						else _currTime = _audioCache->_duration;
					}
				}

				framesRead = decoder->readFixedFrames(framesToRead, tmpBuffer);
				if (framesRead == 0) {
					if (_loop) {
						// 循环音频：回到开头继续读
						decoder->seek(0);
						framesRead = decoder->readFixedFrames(framesToRead, tmpBuffer);
					}
					else {
						// 非循环音频读到文件尾：标记线程可以退出
						needToExitThread = true;
						break;
					}
				}

				// 把一个播完的 buffer unqueue 出来，填入新数据，再 queue 回去
				ALuint bid;
				alSourceUnqueueBuffers(_alSource, 1, &bid);
				alBufferData(bid, _audioCache->_format, tmpBuffer,
					framesRead * decoder->getBytesPerFrame(),
					decoder->getSampleRate());
				alSourceQueueBuffers(_alSource, 1, &bid);
			}

			if (needToExitThread) break;

			// 【关键改动 2】补完数据后，如果 Source 因为“饥饿”停了，把它重新拉起来
			// 场景：断点调试时，投喂线程被暂停，但音频输出没停，
			//       队列里的 buffer 被消耗光 → Source 自动变 AL_STOPPED。
			//       此时我们刚补了新数据进队列（queued > 0），但 Source 还是停的，
			//       必须主动 alSourcePlay 才能续上。
			// 注意：只在 queued > 0 时才重拉，避免对“真播完”的音频死循环重拉。
			alGetSourcei(_alSource, AL_SOURCE_STATE, &sourceState);
			if (sourceState == AL_STOPPED && !_isDestroyed) {
				ALint queued = 0;
				alGetSourcei(_alSource, AL_BUFFERS_QUEUED, &queued);
				if (queued > 0) {
					alSourcePlay(_alSource);
				}
			}

			// 睡 75ms 再检查下一轮
			// 队列里有 3 个 buffer × 0.1s = 0.3s 缓冲，75ms 一轮足够追上
			std::unique_lock<std::mutex> lk(_sleepMutex);
			if (_isDestroyed) break;
			_sleepCondition.wait_for(lk, std::chrono::milliseconds(75));
		}
	} while (false);

	// 清理资源
	if (decoder != nullptr) decoder->close();
	AudioDecoderManager::destroyDecoder(decoder);
	free(tmpBuffer);
	_isRotateThreadExited = true;   // 通知 destroy() 线程已退出
}

bool AudioPlayer::setLoop(bool loop)
{
    if (!_isDestroyed ) {
        _loop = loop;
        return true;
    }

    return false;
}

bool AudioPlayer::setTime(float time)
{
    if (!_isDestroyed && time >= 0.0f && time < _audioCache->_duration) {

        _currTime = time;
        _timeDirty = true;

        return true;
    }
    return false;
}
