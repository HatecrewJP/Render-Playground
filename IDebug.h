#include "Ix64.h"

#pragma once
#if DEBUG

struct DebugContext{
	void(*DebugDisplayBuffer)(struct DebugContext*);
	HWND DebugWindowHandle;
	HDC DebugDeviceContext;
	struct Win32FrameBuffer *DebugFrameBuffer;
};

#endif