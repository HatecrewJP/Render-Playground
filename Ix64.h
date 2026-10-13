#pragma once
#define global static
#define internal static
#define ASSERT(x) if(!(x)) __debugbreak();
typedef unsigned char byte;

typedef void(*RenderFrameProc)(struct IDebugContext*, struct IScreenBuffer*, int, struct IInput*) ; 

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
