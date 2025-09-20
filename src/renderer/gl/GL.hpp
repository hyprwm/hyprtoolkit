#pragma once

#include <GLES3/gl32.h>

#ifndef HYPRTOOLKIT_DEBUG

#define GLCALL(__CALL__) __CALL__;

#else

#define GLCALL(__CALL__)                                                                                                                                                           \
    {                                                                                                                                                                              \
        __CALL__;                                                                                                                                                                  \
        auto err = glGetError();                                                                                                                                                   \
        if (err != GL_NO_ERROR) {                                                                                                                                                  \
            g_logger->log(HT_LOG_ERROR, "[GLES] Error in call at {}@{}: 0x{:x}", __LINE__,                                                                                         \
                          ([]() constexpr -> std::string { return std::string(__FILE__).substr(std::string(__FILE__).find_last_of('/') + 1); })(), err);                           \
        }                                                                                                                                                                          \
    }

#endif