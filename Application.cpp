//#include "Ix64.h"
#define DEBUG 1
#include "IDebug.h"
#include "math.h"
#include "stdio.h"
#include "memory.h"

struct Pos2{
	float x;
	float y;
};

struct Vec2{
	float x;
	float y;
};

struct ColorBGRA{
	unsigned char B;
	unsigned char G;
	unsigned char R;
	unsigned char A;
};

struct ColorRGBA{
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char A;
};



internal float DistVec2(Vec2 a, Vec2 b){
	return (float)sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}
internal Vec2 NormalizeVec2(Vec2 v){
	float Magnitude = (float)sqrt(v.x * v.x + v.y * v.y);
	if(-0.00001f <= Magnitude && Magnitude <= 0.00001f){
		return v;
	}
	Vec2 Result;
	Result.x = v.x / Magnitude;
	Result.y = v.y / Magnitude;
	return Result;
}

internal void NaiveRasterizeLine(IScreenBuffer *Buffer, Pos2 Start, Pos2 End){
	ASSERT(Buffer);
	ColorBGRA *Pixels = *(ColorBGRA**)Buffer->Data;
	ASSERT(Pixels);


	Vec2 Slope;
	Slope.x = End.x - Start.x;
	Slope.y = End.y - Start.y;

	Vec2 SlopeNormalized = NormalizeVec2(Slope);
	float Magnitude = (float)sqrt(
		Slope.x * Slope.x + Slope.y * Slope.y);
	int IterationCount = (int)floor(Magnitude) + 1;
	

	for(int i = 0; i < IterationCount; i++){
		Pos2 CurrentPos;
		CurrentPos.x = roundf(Start.x + (float)i * SlopeNormalized.x);
		CurrentPos.y = roundf(Start.y + (float)i * SlopeNormalized.y);
		if(CurrentPos.x >= Buffer->Width || CurrentPos.y >= Buffer->Height){
			continue;
		}

		int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
		Pixels[Index] = ColorBGRA{0,0,0,255};

	}




}
internal void NaiveRasterizeTri(IScreenBuffer *Buffer, Pos2 VL, Pos2 VX, Pos2 VR){
	NaiveRasterizeLine(Buffer, VL, VX);
	NaiveRasterizeLine(Buffer, VX, VR);
	NaiveRasterizeLine(Buffer, VL, VR);

}


internal void SetBackgroundColor(IScreenBuffer *Buffer, ColorBGRA BackgroundColor){
	ColorBGRA *CurrentPixel = *(ColorBGRA**)Buffer->Data;
	for(int Col = 0; Col < (int)Buffer->Width; Col++){
		for(int Row = 0; Row < (int)Buffer->Height; Row++){
			*CurrentPixel = BackgroundColor;
			CurrentPixel++;
		}
	}
}



internal void RenderFrame(struct IScreenBuffer *Buffer, int XOffset){
	ASSERT(Buffer);
	ColorBGRA *Pixels = *(ColorBGRA**)(Buffer->Data);
	ASSERT(Pixels);
	ColorBGRA BackgroundColor = {255,255,255,255};
	SetBackgroundColor(Buffer, BackgroundColor);
	Pos2 V1 = {20,20};
	Pos2 V2 = {50,400};
	Pos2 V3 = {800,20};
	NaiveRasterizeTri(Buffer,V1,V3,V2);
	
	

	









}