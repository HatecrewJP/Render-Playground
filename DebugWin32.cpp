#pragma once
#ifdef DEBUG

#include "IDebug.h"


struct DebugContext{
	HWND DebugWindowHandle;
	HDC DebugDeviceContext;
	struct Win32FrameBuffer *DebugFrameBuffer;
};

global struct DebugContext GlobalDebugCTX;

internal void DebugDisplayBuffer(IScreenBuffer *Buffer){
	Win32DisplayBuffer(GlobalDebugCTX.DebugWindowHandle, GlobalDebugCTX.DebugDeviceContext,
		(Win32FrameBuffer *)Buffer);
}

internal long long DebugGetTime(void){
	long long x = 0;
	Assert(QueryPerformanceCounter((LARGE_INTEGER*) &x));
	return x;

}


#endif