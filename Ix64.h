#pragma once
#define global static
#define internal static
#define ASSERT(x) if(!(x)) __debugbreak();
typedef unsigned char byte;

typedef struct IScreenBuffer{
	//CAREFUL WHEN CHANGING:
	//Type Punning when passing to Application
	unsigned Width;
	unsigned Height;
	unsigned PixelSize;
	unsigned unused;
	void **Data;
};