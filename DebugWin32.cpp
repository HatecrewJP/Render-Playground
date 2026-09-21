#pragma once
#ifdef DEBUG
#include "IDebug.h"


internal void DebugDisplayBuffer(DebugContext *CTX){
	Win32DisplayBuffer(CTX->DebugWindowHandle, 
		CTX->DebugDeviceContext, 
		CTX->DebugFrameBuffer);
}




#endif