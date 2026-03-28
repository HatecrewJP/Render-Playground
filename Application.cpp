//#include "Ix64.h"
#define DEBUG 1
#include "IDebug.h"
#include "math.h"
#include "stdio.h"
#include "memory.h"

#define PI 3.141592653589793238462643383279f

struct Pos2{
	float x;
	float y;
};

struct Vec2{
	float x;
	float y;
};





internal float RoundFloat(float In){
	return (float)((int)(In + 0.5f));
}
internal unsigned RoundFloatToUInt(float In){
	return ((unsigned)(In + 0.5f));
}

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
	unsigned *Pixels = *(unsigned **)Buffer->Data;
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
		CurrentPos.x = RoundFloat(Start.x + (float)i * SlopeNormalized.x);
		CurrentPos.y = RoundFloat(Start.y + (float)i * SlopeNormalized.y);
		if(CurrentPos.x >= Buffer->Width || CurrentPos.y >= Buffer->Height){
			continue;
		}
		if(CurrentPos.x < 0 || CurrentPos.y < 0){
			continue;
		}

		int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
		Pixels[Index] = 0x000000;

	}




}
internal void NaiveRasterizeTri(IScreenBuffer *Buffer, Pos2 VL, Pos2 VX, Pos2 VR){
	NaiveRasterizeLine(Buffer, VL, VX);
	NaiveRasterizeLine(Buffer, VX, VR);
	NaiveRasterizeLine(Buffer, VL, VR);

}
internal void RasterizePoint(IScreenBuffer *Buffer, Pos2 P, unsigned Color){
	unsigned *Pixels = *(unsigned **)Buffer->Data;
	Pos2 CurrentPos;
	CurrentPos.x = (float)RoundFloat(P.x);
	CurrentPos.y = (float)RoundFloat(P.y);
	int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
	Pixels[Index] = Color;
}



internal void NaiveRasterizeCircle(IScreenBuffer *Buffer, Pos2 Center, float Radius){
	ASSERT(Buffer);
	float Resolution = 0.0001f;
	unsigned *Pixels = *(unsigned **)Buffer->Data;

	for(float a = 0; a < 2 * PI; a += Resolution){
		Pos2 CurrentPos;
		CurrentPos.x = Center.x + (float)RoundFloat((float)cos(a) * Radius);
		CurrentPos.y = Center.y + (float)RoundFloat((float)sin(a) * Radius);

		if(CurrentPos.x >= Buffer->Width || CurrentPos.y >= Buffer->Height){
			continue;
		}
		if(CurrentPos.x < 0 || CurrentPos.y < 0){
			continue;
		}
		int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
		Pixels[Index] = 0x00FF00;
	}



}




internal void NaiveRasterizeQuadraticBezierCurve(IScreenBuffer *Buffer, Pos2 S, Pos2 E, Pos2 C){
	unsigned *Pixels = *(unsigned **)Buffer->Data;
	float Resolution = 0.0001f;
	for(float t = 0; t <= 1; t += Resolution){
		Pos2 CurrentPos;
		CurrentPos.x = RoundFloat((1 - t) * (1 - t) * S.x + 2 * (1 - t) * t * C.x + t * t * E.x);
		CurrentPos.y = RoundFloat((1 - t) * (1 - t) * S.y + 2 * (1 - t) * t * C.y + t * t * E.y);


		if(CurrentPos.x >= Buffer->Width || CurrentPos.y >= Buffer->Height){
			continue;
		}
		if(CurrentPos.x < 0 || CurrentPos.y < 0){
			continue;
		}

		int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
		Pixels[Index] = 0xFF00FF;
	}
}


internal void NaiveRasterizeCubicBezierCurve(IScreenBuffer *Buffer, Pos2 P0, Pos2 P1, Pos2 P2, Pos2 P3){
	unsigned *Pixels = *(unsigned **)Buffer->Data;
	float Resolution = 0.0001f;
	for(float t = 0; t <= 1; t += Resolution){
		Pos2 CurrentPos;
		CurrentPos.x = RoundFloat(P0.x + t * (-3 * P0.x + 3 * P1.x) + t * t * (3 * P0.x - 6 * P1.x + 3 * P2.x) + t * t * t * (-P0.x + 3 * P1.x - 3 * P2.x + P3.x));
		CurrentPos.y = RoundFloat(P0.y + t * (-3 * P0.y + 3 * P1.y) + t * t * (3 * P0.y - 6 * P1.y + 3 * P2.y) + t * t * t * (-P0.y + 3 * P1.y - 3 * P2.y + P3.y));

		if(CurrentPos.x >= Buffer->Width || CurrentPos.y >= Buffer->Height){
			continue;
		}
		if(CurrentPos.x < 0 || CurrentPos.y < 0){
			continue;
		}

		int Index = (int)CurrentPos.x + (int)CurrentPos.y * Buffer->Width;
		Pixels[Index] = 0xFF00FF;
	}
}

internal void NaiveDrawRectangle(IScreenBuffer *Buffer, float Left, float Top, float Right, float Bottom){
	unsigned MinX = RoundFloatToUInt(Left);
	unsigned MinY = RoundFloatToUInt(Top);
	unsigned MaxX = RoundFloatToUInt(Right - (float)MinX);
	unsigned MaxY = RoundFloatToUInt(Bottom - (float)MinY);

	if(MinX < 0){
		MinX = 0;
	}
	if(MinY < 0){
		MinY = 0;
	}
	if(MaxX >= (unsigned)Buffer->Width){
		MaxX = (unsigned)Buffer->Width;
	}
	if(MaxY >= (unsigned)Buffer->Height){
		MaxY = (unsigned)Buffer->Height;
	}
	unsigned *Row = *(unsigned **)Buffer->Data + MinX + MinY * Buffer->Width * Buffer->PixelSize;
	
	for(unsigned Y = MinY; Y < MaxY; Y++){
		unsigned *Pixels = Row;
		for(unsigned X = MinX; X < MaxX; X++){
			*Pixels = 0xFFFF00;
			Pixels++;
		}
		Row += Buffer->Width;
	}
}


internal void SetBackgroundColor(IScreenBuffer *Buffer, unsigned BackgroundColor){
	unsigned *CurrentPixel = *(unsigned **)Buffer->Data;
	for(int Col = 0; Col < (int)Buffer->Width; Col++){
		for(int Row = 0; Row < (int)Buffer->Height; Row++){
			*CurrentPixel = BackgroundColor;
			CurrentPixel++;
		}
	}
}


internal void RenderFrame(struct IScreenBuffer *Buffer, int XOffset, struct IInput *Input){
	ASSERT(Buffer);
	unsigned *Pixels = *(unsigned **)(Buffer->Data);
	ASSERT(Pixels);
	unsigned BackgroundColor = 0xFFFFFF;
	SetBackgroundColor(Buffer, BackgroundColor);
	static Pos2 V1 = {100,100};
	static Pos2 V2 = {100,500};
	static Pos2 V3 = {500,500};
	static Pos2 V4 = {500,100};
	

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

	NaiveDrawRectangle(Buffer, 200, 40, 600, 700);

	NaiveRasterizeLine(Buffer, V1, V2);
	NaiveRasterizeLine(Buffer, V2, V3);
	NaiveRasterizeLine(Buffer, V3, V4);
	NaiveRasterizeCubicBezierCurve(Buffer, V1, V2, V3, V4);


	NaiveRasterizeCircle(Buffer, V1, 2.0f);
	NaiveRasterizeCircle(Buffer, V2, 2.0f);
	NaiveRasterizeCircle(Buffer, V3, 2.0f);
	NaiveRasterizeCircle(Buffer, V4, 2.0f);













}