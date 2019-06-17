//
// Created by mingyi.li on 2018/12/25.
//

#include "../include/NativeWindow.h"

NativeWindow::NativeWindow(HwWindow *win, Egl *egl) {
    this->win = win;
    this->egl = egl;
}

NativeWindow::~NativeWindow() {
    this->win = nullptr;
    this->egl = nullptr;
}
