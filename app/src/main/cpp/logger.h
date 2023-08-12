#ifndef VRVIDEOPLAYER_LOGGER_H
#define VRVIDEOPLAYER_LOGGER_H

#include <strings.h>

#include <android/log.h>

#define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#endif //VRVIDEOPLAYER_LOGGER_H
