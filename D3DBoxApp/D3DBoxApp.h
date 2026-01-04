#pragma once

#include "resource.h"

#ifndef KEY_UP
//! 키 눌림 테스트. 
#define KEY_UP(key)	    ((GetAsyncKeyState(key)&0x8001) == 0x8001)
#define KEY_DOWN(key)	((GetAsyncKeyState(key)&0x8000) == 0x8000)
//키 테스트 (구형 호환성 유지)
#define IsKeyUp         KEY_UP
#define IsKeyDown       KEY_DOWN
#endif