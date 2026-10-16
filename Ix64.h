#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define global static
#define internal static
#define ASSERT(x) if(!(x)) __debugbreak();
	typedef unsigned char byte;

	struct IScreenBuffer{
		//CAREFUL WHEN CHANGING:
		//Type Punning when passing to Application
		int Width;
		int Height;
		int PixelSize;
		int unused;
		unsigned *Data;
	};
	struct IInput{
		short MouseX;
		short MouseY;
		int LeftClick;
		int RightClick;
		int MiddleClick;
	};
	#define RENDER_FRAME(name) void (name) (struct IDebugContext *DebugCTX, struct IScreenBuffer *Buffer, int XOffset, struct IInput *Input, bool *EnableFrameTime)
	typedef RENDER_FRAME(render_frame);



	_declspec(dllexport) RENDER_FRAME(RenderFrame);
#ifdef __cplusplus
}
#endif // __cplusplus
