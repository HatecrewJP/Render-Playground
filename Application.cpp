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
		if(CurrentPos.x < 0 || CurrentPos.y < 0){
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
internal void RasterizePoint(IScreenBuffer *Buffer, Pos2 P, ColorBGRA Color){
	ColorBGRA *Pixels = *(ColorBGRA **)Buffer->Data;
	Pos2 CurrentPos;
	CurrentPos.x = (float) round(P.x);
	CurrentPos.y = (float) round(P.y);
	int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
	Pixels[Index] = Color;
}

internal void RasterizeQuadraticBezierCurve(IScreenBuffer *Buffer,Pos2 S, Pos2 E, Pos2 C){
	ColorBGRA *Pixels = *(ColorBGRA **)Buffer->Data;
	float LerpResolution = 0.0001f;
	for(float t = 0; t <= 1; t += LerpResolution){
		Pos2 CurrentPos;
		/*CurrentPos.x = roundf((1-t) * (1 - t) * S.x + 2 * (1 - t) * t * C.x + t * t * E.x);
		CurrentPos.y = roundf((1-t) * (1 - t) * S.y + 2 * (1 - t) * t * C.y + t * t * E.y);*/
		CurrentPos.x = roundf(S.x + 2 * t * (C.x - S.x) + t * t * (E.x + S.x - 2*C.x));
		CurrentPos.y = roundf(S.y + 2 * t * (C.y - S.y) + t * t * (E.y + S.y - 2*C.y));
	
		if(CurrentPos.x >= Buffer->Width || CurrentPos.y >= Buffer->Height){
			continue;
		}
		if(CurrentPos.x < 0 || CurrentPos.y < 0){
			continue;
		}
		int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
		Pixels[Index] = ColorBGRA{255,0,0,255};
	}
}

internal void RasterizeQuadraticBezierCurve2(IScreenBuffer *Buffer,Pos2 S, Pos2 E, Pos2 C){
	ColorBGRA *Pixels = *(ColorBGRA **)Buffer->Data;
	float LerpResolution = 0.0001f;
	for(float t = 0; t <= 1; t += LerpResolution){
		Pos2 CurrentPos;
		/*CurrentPos.x = roundf((1-t) * (1 - t) * S.x + 2 * (1 - t) * t * C.x + t * t * E.x);
		CurrentPos.y = roundf((1-t) * (1 - t) * S.y + 2 * (1 - t) * t * C.y + t * t * E.y);*/
		CurrentPos.x = roundf(S.x + 2 * t * (C.x - S.x) + t * t * (E.x + S.x - 2*C.x));
		CurrentPos.y = roundf(S.y + 2 * t * (C.y - S.y) + t * t * (E.y + S.y - 2*C.y));
	
		if(CurrentPos.x >= Buffer->Width || CurrentPos.y >= Buffer->Height){
			continue;
		}
		if(CurrentPos.x < 0 || CurrentPos.y < 0){
			continue;
		}
		int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
		Pixels[Index] = ColorBGRA{255,0,0,255};
	}
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



internal void RenderFrame(struct IScreenBuffer *Buffer, int XOffset, struct IInput *Input){
	ASSERT(Buffer);
	ColorBGRA *Pixels = *(ColorBGRA**)(Buffer->Data);
	ASSERT(Pixels);
	ColorBGRA BackgroundColor = {255,255,255,0};
	SetBackgroundColor(Buffer, BackgroundColor);
	static Pos2 V1 = {100,100};
	static Pos2 V2 = {100,500};
	static Pos2 V3 = {500,500};
	ColorBGRA ColorV1 = {0,0,255,255};
	ColorBGRA ColorV2 = {0,255,0,255};
	ColorBGRA ColorV3 = {0,255,255,255};

	if(Input->LeftClick){
		V1.x = Input->MouseX;
		V1.y = Input->MouseY;
	}
	if(Input->RightClick){
		V2.x = Input->MouseX;
		V2.y = Input->MouseY;
	}
	if(Input->MiddleClick){
		V3.x = Input->MouseX;
		V3.y = Input->MouseY;
	}

	NaiveRasterizeTri(Buffer,V1,V2,V3);
	RasterizeQuadraticBezierCurve(Buffer, V1, V2, V3);
	RasterizePoint(Buffer, V1, ColorV1);
	RasterizePoint(Buffer, V2, ColorV2);
	RasterizePoint(Buffer, V3, ColorV3);
	

	
	

	









}