//
// Created by mingyi.li on 2018/12/25.
//

#include "../include/PictureProcessor.h"
#include "../include/HwRender.h"
#include "../include/HwScreen.h"
#include "../include/Image.h"
#include "../include/NativeWindow.h"
#include "ObjectBox.h"

PictureProcessor::PictureProcessor() {
    pipeline = new UnitPipeline(__FUNCTION__);
    pipeline->registerAnUnit(new Image());
    pipeline->registerAnUnit(new HwRender());
    //注意顺序问题，包含EGL环境的模块放到最后，因为要最后释放
    pipeline->registerAnUnit(new HwScreen());
}

PictureProcessor::~PictureProcessor() {
    if (pipeline) {
        pipeline->release();
        delete pipeline;
        pipeline = nullptr;
    }
}

void PictureProcessor::prepare(HwWindow *win) {
    if (pipeline) {
        Message *msg = new Message(EVENT_COMMON_PREPARE, nullptr);
        msg->obj = new ObjectBox(new NativeWindow(win, nullptr));
        pipeline->postEvent(msg);
    }
}

void PictureProcessor::updateWindow(HwWindow *win) {
    if (pipeline) {
        Message *msg = new Message(EVENT_SCREEN_UPDATE_WINDOW, nullptr);
        msg->obj = new ObjectBox(new NativeWindow(win, nullptr));
        pipeline->postEvent(msg);
    }
}

void PictureProcessor::show(char *path) {
    if (!pipeline) return;
    Message *msg = new Message(EVENT_IMAGE_SHOW, nullptr);
    msg->obj = new ObjectBox(path);
    pipeline->postEvent(msg);
}


void PictureProcessor::setFilter(Filter *filter) {
    Message *msg = new Message(EVENT_RENDER_SET_FILTER, nullptr);
    msg->obj = new ObjectBox(filter);
    pipeline->postEvent(msg);
}

void PictureProcessor::invalidate() {
    if (!pipeline) return;
    Message *invalidateMsg = new Message(EVENT_COMMON_INVALIDATE, nullptr);
    pipeline->postEvent(invalidateMsg);
}