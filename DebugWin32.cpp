#pragma once
#ifdef DEBUG

global HWND DebugWindowHandle;
global HDC DebugDeviceContext;
global Win32FrameBuffer *DebugFrameBuffer;

void DebugDisplayBuffer(){
	Win32DisplayBuffer(DebugWindowHandle, DebugDeviceContext, DebugFrameBuffer);
}


#endif