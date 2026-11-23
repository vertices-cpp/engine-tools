

#pragma once

#define QUEUEBUFFER_NUM (3)
#define QUEUEBUFFER_TIME_STEP (0.1f)



void audioLog(const char * format, ...);

#define QUOTEME_(x) #x
#define QUOTEME(x) QUOTEME_(x)

#if defined(ORANGE_DEBUG) && ORANGE_DEBUG > 0
#define ALOGV(fmt, ...) audioLog("V/" LOG_TAG " (" QUOTEME(__LINE__) "): " fmt "", ##__VA_ARGS__)
#else
#define ALOGV(fmt, ...) do {} while(false)
#endif
#define ALOGD(fmt, ...) audioLog("D/" LOG_TAG " (" QUOTEME(__LINE__) "): " fmt "", ##__VA_ARGS__)
#define ALOGI(fmt, ...) audioLog("I/" LOG_TAG " (" QUOTEME(__LINE__) "): " fmt "", ##__VA_ARGS__)
#define ALOGW(fmt, ...) audioLog("W/" LOG_TAG " (" QUOTEME(__LINE__) "): " fmt "", ##__VA_ARGS__)
#define ALOGE(fmt, ...) audioLog("E/" LOG_TAG " (" QUOTEME(__LINE__) "): " fmt "", ##__VA_ARGS__)

#if defined(ORANGE_DEBUG) && ORANGE_DEBUG > 0
#define CHECK_AL_ERROR_DEBUG() \
do { \
    ALenum __error = alGetError(); \
    if (__error) { \
        ALOGE("OpenAL error 0x%04X in %s %s %d\n", __error, __FILE__, __FUNCTION__, __LINE__); \
    } \
} while (false)
#else
#define CHECK_AL_ERROR_DEBUG()
#endif

#define BREAK_IF(condition) \
    if (!!(condition)) { \
        break; \
    }

#define BREAK_IF_ERR_LOG(condition, fmt, ...) \
    if (!!(condition)) { \
        ALOGE("(" QUOTEME(condition) ") failed, message: " fmt, ##__VA_ARGS__); \
        break; \
    }
