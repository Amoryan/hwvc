/*
* Copyright (c) 2018-present, lmyooyo@gmail.com.
*
* This source code is licensed under the GPL license found in the
* LICENSE file in the root directory of this source tree.
*/

#include "../include/DefaultAudioDecoder.h"
#include <cassert>
#include "Logcat.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "../include/FFUtils.h"

#ifdef __cplusplus
}
#endif

DefaultAudioDecoder::DefaultAudioDecoder() : AbsAudioDecoder() {
    hwFrameAllocator = new HwFrameAllocator();
}

DefaultAudioDecoder::~DefaultAudioDecoder() {
    if (avPacket) {
        av_packet_free(&avPacket);
        avPacket = nullptr;
    }
    if (audioFrame) {
        av_frame_free(&audioFrame);
        audioFrame = nullptr;
    }
    if (aCodecContext) {
        avcodec_close(aCodecContext);
        aCodecContext = nullptr;
    }
    if (pFormatCtx) {
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = nullptr;
    }
    if (outHwFrame) {
        outHwFrame->recycle();
        outHwFrame = nullptr;
    }
    if (hwFrameAllocator) {
        delete hwFrameAllocator;
        hwFrameAllocator = nullptr;
    }
};

bool DefaultAudioDecoder::prepare(string path) {
    Logcat::i("HWVC", "DefaultAudioDecoder::prepare: %s", path.c_str());
    this->path = path;
    av_register_all();
    printCodecInfo();
    pFormatCtx = avformat_alloc_context();
    //打开输入视频文件
    if (avformat_open_input(&pFormatCtx, path.c_str(), NULL, NULL) != 0) {
        Logcat::e("HWVC", "Couldn't open input stream.");
        return false;
    }
    //获取视频文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        Logcat::e("HWVC", "Couldn't find stream information.");
        return -1;
    }
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (-1 == audioTrack &&
            AVMediaType::AVMEDIA_TYPE_AUDIO == pFormatCtx->streams[i]->codecpar->codec_type) {
            audioTrack = i;
        }
    }
    if (-1 != audioTrack && !openTrack(audioTrack, &aCodecContext)) {
        Logcat::e("HWVC", "******** Open audio track failed. *********");
        return false;
    }
    Logcat::e("HWVC",
              "DefaultAudioDecoder::prepare(duration=%lld channels=%d, sampleHz=%d, frameSize=%d)",
              getAudioDuration(), getChannels(), getSampleHz(), aCodecContext->frame_size);
    outSampleFormat = getBestSampleFormat(aCodecContext->sample_fmt);
    translator = new HwAudioTranslator(
            HwSampleFormat(HwAbsMediaFrame::convertToAudioFrameFormat(outSampleFormat),
                           getChannels(),
                           getSampleHz()),
            HwSampleFormat(HwAbsMediaFrame::convertToAudioFrameFormat(aCodecContext->sample_fmt),
                           getChannels(),
                           getSampleHz()));
    //准备资源
    avPacket = av_packet_alloc();
    audioFrame = av_frame_alloc();
//    seek(170749167);
    return true;
}

/**
 * Get an audio or a video frame.
 * @param frame 每次返回的地址可能都一样，所以获取一帧音视频后请立即使用，在下次grab之后可能会被释放
 */
HwResult DefaultAudioDecoder::grab(HwAbsMediaFrame **frame) {
    while (true) {
        readPkgLock.lock();
        int ret = av_read_frame(pFormatCtx, avPacket);
        readPkgLock.unlock();
        if (0 == ret && audioTrack == avPacket->stream_index) {
            avcodec_send_packet(aCodecContext, avPacket);
        }
        av_packet_unref(avPacket);//Or av_free_packet?
//            switch (ret) {
//                case AVERROR(EAGAIN): {
//                    LOGI("you must read output with avcodec_receive_frame");
//                }
//                case AVERROR(EINVAL): {
//                    LOGI("codec not opened, it is an encoder, or requires flush");
//                    break;
//                }
//                case AVERROR(ENOMEM): {
//                    LOGI("failed to add packet to internal queue");
//                    break;
//                }
//                case AVERROR_EOF: {
//                    LOGI("eof");
//                    eof = true;
//                    break;
//                }
//                default:
//                    LOGI("avcodec_send_packet ret=%d", ret);
//            }
        if (AVERROR_EOF == ret) {
            eof = true;
        }
        if (0 == avcodec_receive_frame(aCodecContext, audioFrame)) {
            matchPts(audioFrame, audioTrack);
            if (outHwFrame) {
                outHwFrame->recycle();
                outHwFrame = nullptr;
            }
            outHwFrame = resample(audioFrame);
            *frame = outHwFrame;
            av_frame_unref(audioFrame);
            return Hw::MEDIA_SUCCESS;
        }
        //如果缓冲区中既没有音频也没有视频，并且已经读取完文件，则播放完了
        if (eof) {
            Logcat::i("HWVC", "DefaultAudioDecoder::grab EOF");
            return Hw::MEDIA_EOF;
        }
    }
}

void DefaultAudioDecoder::seek(int64_t us) {
    us = av_rescale_q_rnd(us, outputTimeBase,
                          pFormatCtx->streams[audioTrack]->time_base,
                          AV_ROUND_NEAR_INF);
    readPkgLock.lock();
    av_seek_frame(pFormatCtx, audioTrack, us, AVSEEK_FLAG_BACKWARD);
    readPkgLock.unlock();
}

int DefaultAudioDecoder::getChannels() {
    return aCodecContext->channels;
}

int DefaultAudioDecoder::getSampleHz() {
    return aCodecContext->sample_rate;
}

int DefaultAudioDecoder::getSampleFormat() {
    assert(aCodecContext);
    return outSampleFormat;
}

int DefaultAudioDecoder::getSamplesPerBuffer() {
    assert(aCodecContext);
    return aCodecContext->frame_size;
}

int64_t DefaultAudioDecoder::getAudioDuration() {
    if (audioDurationUs >= 0) {
        return audioDurationUs;
    }
    audioDurationUs = pFormatCtx->streams[audioTrack]->duration;
    audioDurationUs = av_rescale_q_rnd(audioDurationUs,
                                       pFormatCtx->streams[audioTrack]->time_base,
                                       pFormatCtx->streams[audioTrack]->codec->time_base,
                                       AV_ROUND_NEAR_INF);
    audioDurationUs = av_rescale_q_rnd(audioDurationUs,
                                       pFormatCtx->streams[audioTrack]->codec->time_base,
                                       outputTimeBase,
                                       AV_ROUND_NEAR_INF);
    return audioDurationUs;
}

bool DefaultAudioDecoder::openTrack(int track, AVCodecContext **context) {
    AVCodecParameters *avCodecParameters = pFormatCtx->streams[track]->codecpar;
    AVCodec *codec = NULL;
    if (AV_CODEC_ID_H264 == avCodecParameters->codec_id) {
        codec = avcodec_find_decoder_by_name("h264_mediacodec");
        if (NULL == codec) {
            Logcat::e("HWVC", "Selected AV_CODEC_ID_H264.");
            codec = avcodec_find_decoder(avCodecParameters->codec_id);
        }
    } else {
        codec = avcodec_find_decoder(avCodecParameters->codec_id);
    }
    if (NULL == codec) {
        Logcat::e("HWVC", "Couldn't find codec.");
        return false;
    }
    //打开编码器
    *context = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(*context, avCodecParameters);
    if (avcodec_open2(*context, codec, NULL) < 0) {
        Logcat::e("HWVC", "Couldn't open codec.");
        return false;
    }
    string typeName = "unknown";
    if (AVMEDIA_TYPE_VIDEO == codec->type) {
        typeName = "video";
    } else if (AVMEDIA_TYPE_AUDIO == codec->type) {
        typeName = "audio";
    }
    Logcat::e("HWVC", "Open %s track with %s, fmt=%d, frameSize=%d", typeName.c_str(), codec->name,
              avCodecParameters->format, avCodecParameters->frame_size);
    return true;
}

HwAbsMediaFrame *DefaultAudioDecoder::resample(AVFrame *avFrame) {
    if (!translator) {
        return nullptr;
    }
    AVFrame *outFrame = nullptr;
    if (translator->translate(&outFrame, &avFrame)) {
        return hwFrameAllocator->ref(outFrame);
    }
    return nullptr;
}

void DefaultAudioDecoder::matchPts(AVFrame *frame, int track) {
//    frame->pts = av_rescale_q_rnd(frame->pts,
//                                  pFormatCtx->streams[track]->time_base,
//                                  pFormatCtx->streams[track]->codec->time_base,
//                                  AV_ROUND_NEAR_INF);
    frame->pts = av_rescale_q_rnd(frame->pts,
                                  pFormatCtx->streams[track]->time_base,
                                  outputTimeBase,
                                  AV_ROUND_NEAR_INF);
}

AVSampleFormat DefaultAudioDecoder::getBestSampleFormat(AVSampleFormat in) {
    AVSampleFormat out = av_get_packed_sample_fmt(in);
    if (AV_SAMPLE_FMT_FLT == out || AV_SAMPLE_FMT_DBL == out) {
        out = AV_SAMPLE_FMT_S32;
    }
    return out;
}

void DefaultAudioDecoder::printCodecInfo() {
    char info[1024] = {0};
    AVCodec *c_temp = av_codec_next(NULL);
    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%s[Dec]", info);
        } else {
            sprintf(info, "%s[Enc]", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s[Video]", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s[Audio]", info);
                break;
            default:
                sprintf(info, "%s[Other]", info);
                break;
        }
        sprintf(info, "%s[%10s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }
    Logcat::e("HWVC", "%s", info);
}