/*
 * Copyright (c) 2018-present, lmyooyo@gmail.com.
 *
 * This source code is licensed under the GPL license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef HARDWAREVIDEOCODEC_ABSAUDIODECODER_H
#define HARDWAREVIDEOCODEC_ABSAUDIODECODER_H

#include "AbsDecoder.h"

class AbsAudioDecoder : virtual public AbsDecoder {
public:
    AbsAudioDecoder();

    virtual ~AbsAudioDecoder();

    virtual int getChannels()=0;

    virtual int getSampleHz()=0;

    virtual int getSampleFormat()=0;

    virtual int getSamplesPerBuffer()=0;

    virtual int64_t getAudioDuration()=0;
};


#endif //HARDWAREVIDEOCODEC_ABSAUDIODECODER_H
