
set(ORANGE_AUDIO_SRC
    audio/AudioEngine.cpp
    )

    set(ORANGE_AUDIO_PLATFORM_HEADER
        audio/generic/AudioDecoderManager.h
        audio/generic/AudioDecoder.h
        audio/generic/AudioPlayer.h
        audio/generic/AudioDecoderOgg.h
        audio/generic/AudioMacros.h
        audio/generic/AudioEngineImpl.h
        audio/generic/AudioDecoderMp3.h
        audio/generic/AudioCache.h
        )

    set(ORANGE_AUDIO_PLATFORM_SRC
        audio/generic/AudioEngineImpl.cpp
        audio/generic/AudioCache.cpp
        audio/generic/AudioPlayer.cpp
        audio/generic/AudioDecoder.cpp
        audio/generic/AudioDecoderManager.cpp
        audio/generic/AudioDecoderMp3.cpp
        audio/generic/AudioDecoderOgg.cpp
        )

set(ORANGE_AUDIO_HEADER
    audio/include/AudioEngine.h
 
    ${ORANGE_AUDIO_PLATFORM_HEADER}
    )
list(APPEND ORANGE_AUDIO_SRC ${ORANGE_AUDIO_PLATFORM_SRC})
