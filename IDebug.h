#include "Ix64.h"

#pragma once
#if DEBUG

struct IDebugContext{
	void(*DebugDisplayBuffer)(struct IDebugContext*);
	HWND DebugWindowHandle;
	HDC DebugDeviceContext;
	struct Win32FrameBuffer *DebugFrameBuffer;
};

#endif