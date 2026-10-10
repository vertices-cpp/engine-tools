
set(orange_audio_src
    audio/AudioEngine.cpp
    )

    set(orange_audio_platform_header
        audio/generic/AudioDecoderManager.h
        audio/generic/AudioDecoder.h
        audio/generic/AudioPlayer.h
        audio/generic/AudioDecoderOgg.h
        audio/generic/AudioMacros.h
        audio/generic/AudioEngineImpl.h
        audio/generic/AudioDecoderMp3.h
        audio/generic/AudioCache.h
        )

    set(orange_audio_platform_src
        audio/generic/AudioEngineImpl.cpp
        audio/generic/AudioCache.cpp
        audio/generic/AudioPlayer.cpp
        audio/generic/AudioDecoder.cpp
        audio/generic/AudioDecoderManager.cpp
        audio/generic/AudioDecoderMp3.cpp
        audio/generic/AudioDecoderOgg.cpp
        )

set(orange_audio_header
    audio/include/AudioEngine.h
 
    ${orange_audio_platform_header}
    )
list(APPEND orange_audio_src ${orange_audio_platform_src})
