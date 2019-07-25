/*
* Copyright (c) 2018-present, lmyooyo@gmail.com.
*
* This source code is licensed under the GPL license found in the
* LICENSE file in the root directory of this source tree.
*/

#include <libavutil/samplefmt.h>
#include "../include/HwSpeaker.h"

HwSpeaker::HwSpeaker(string alias) : HwSpeaker(alias, HwAudioDeviceMode::Normal) {
}

HwSpeaker::HwSpeaker(string alias, HwAudioDeviceMode mode) : Unit(alias), mode(mode) {
    registerEvent(EVENT_COMMON_PREPARE, reinterpret_cast<EventFunc>(&HwSpeaker::eventPrepare));
    registerEvent(EVENT_SPEAKER_FEED, reinterpret_cast<EventFunc>(&HwSpeaker::eventFeed));
}

HwSpeaker::~HwSpeaker() {
    LOGI("HwSpeaker::~HwSpeaker");
    if (player) {
        player->stop();
        delete player;
        player = nullptr;
    }
}

bool HwSpeaker::eventPrepare(Message *msg) {
    return false;
}

bool HwSpeaker::eventRelease(Message *msg) {
    return false;
}

bool HwSpeaker::eventFeed(Message *msg) {
    if (msg->obj) {
        HwAudioFrame *frame = dynamic_cast<HwAudioFrame *>(msg->obj);
        createFromAudioFrame(frame);
        if (player) {
//            Logcat::i("HWVC", "HwSpeaker::play audio: %d, %d, %lld, %lld",
//                      frame->getChannels(),
//                      frame->getSampleRate(),
//                      frame->getSampleCount(),
//                      frame->getDataSize());
            player->write(frame->getBuffer()->getData(), frame->getBufferSize(), 1000000);
        }
    }
    return false;
}

void HwSpeaker::createFromAudioFrame(HwAudioFrame *frame) {
    if (player) {
        return;
    }
    int format;
    switch (frame->getFormat()) {
        case HwFrameFormat::HW_SAMPLE_U8:
            format = SL_PCMSAMPLEFORMAT_FIXED_8;
            break;
        case HwFrameFormat::HW_SAMPLE_S16:
            format = SL_PCMSAMPLEFORMAT_FIXED_16;
            break;
        default:
            format = SL_PCMSAMPLEFORMAT_FIXED_32;
    }
    player = new HwAudioPlayer(mode,
                               frame->getChannels(),
                               frame->getSampleRate(),
                               static_cast<uint16_t>(format),
                               static_cast<uint32_t>(frame->getSampleCount()));
    player->start();
}