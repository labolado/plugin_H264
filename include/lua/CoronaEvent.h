// Corona SDK event system definitions  
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "CoronaLua.h"

// 事件类型常量
#define CORONA_EVENT_TYPE_GENERIC "generic"
#define CORONA_EVENT_TYPE_VIDEO "video"
#define CORONA_EVENT_TYPE_AUDIO "audio"

// 事件相关函数
void CoronaEventInit(void);
void CoronaEventCleanup(void);

#ifdef __cplusplus
}
#endif