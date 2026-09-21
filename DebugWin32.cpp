#pragma once
#ifdef DEBUG
#include "IDebug.h"


internal void DebugDisplayBuffer(IDebugContext *CTX){
	Win32DisplayBuffer(CTX->DebugWindowHandle, 
		CTX->DebugDeviceContext, 
		CTX->DebugFrameBuffer);
}




#endif