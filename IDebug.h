#include "Ix64.h"

#pragma once
#if DEBUG

struct IDebugContext{
	void(*DebugDisplayBuffer)(IScreenBuffer *Buffer);
};
#endif