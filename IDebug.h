#include "Ix64.h"

#pragma once
#if DEBUG

struct IDebugContext{
	void(*DebugDisplayBuffer)(struct IScreenBuffer *Buffer);
	long long PerfCountFrequency;
	long long (*GetTime)(void);
};


#endif