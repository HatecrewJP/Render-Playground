#include "Ix64.h"
#define DEBUG 1
#define APP 1
#include "IDebug.h"
#include "math.h"
#include "stdio.h"
#include "memory.h"


#define PI 3.141592653589793238462643383279f
#define FRACTION_BITS 8
#define INTEGER_BITS 24
typedef int fxd_24_8;


global constexpr float float_fxd_max = (float)(1 << (INTEGER_BITS - 1)) - (1.0f / (float)(1 << FRACTION_BITS));
global constexpr float float_fxd_min = (float)-(1 << (INTEGER_BITS - 1));
global constexpr fxd_24_8 fxd_max = (1 << (INTEGER_BITS + FRACTION_BITS - 1)) - 1;
global constexpr fxd_24_8 fxd_min = -(1 << (INTEGER_BITS + FRACTION_BITS - 1));

struct Vec2{
	float x;
	float y;
};


internal inline fxd_24_8 FloatToFixed(float n){
	

}

internal inline int Abs32(int n) {
	return (n + (n >> 31)) ^ (n >> 31);
}




internal inline float RoundFloat(float In){
	return (float)((int)(In + 0.5f));
}
internal inline int RoundFloatToInt(float In){
	return (int)(In + 0.5f);
}
internal inline unsigned  RoundFloatToUInt(float In){
	return (unsigned)(In + 0.5f);
}

internal inline float DistVec2(Vec2 a, Vec2 b){
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

internal void NaiveRasterizeLine(IScreenBuffer *Buffer, 
	float SX, float SY,
	float EX, float EY, 
	unsigned Color)
{
	ASSERT(Buffer);
	unsigned *Pixels = Buffer->Data;
	ASSERT(Pixels);


	Vec2 Slope;
	Slope.x = EX - SX;
	Slope.y = EY - SY;

	Vec2 SlopeNormalized = NormalizeVec2(Slope);
	float Magnitude = (float)sqrt(
		Slope.x * Slope.x + Slope.y * Slope.y);
	int IterationCount = (int)floor(Magnitude) + 1;


	for(int i = 0; i < IterationCount; i++){
		
		float CurrentPosX = RoundFloat(SX + (float)i * SlopeNormalized.x);
		float CurrentPosY = RoundFloat(SY + (float)i * SlopeNormalized.y);
		if(CurrentPosX >= Buffer->Width || CurrentPosY >= Buffer->Height){
			continue;
		}
		if(CurrentPosX < 0 || CurrentPosY < 0){
			continue;
		}

		int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
		Pixels[Index] = 0x000000;
	}
}


internal void BresenhamLine(IScreenBuffer *Buffer,
	float SX, float SY, //Line Start
	float EX, float EY, //Line End
	unsigned Color)
{
	ASSERT(Buffer);
	int SlopeX = (int)EX - (int)SX;
	int SlopeY = (int)EY - (int)SY;
	unsigned *Pixels;

	if(Abs32(SlopeX) >= Abs32(SlopeY)){
		//x-major lines
		int MinX = (SX <= EX) ? (int)SX : (int)EX;
		int MinY = (SX <= EX) ? (int)SY : (int)EY;
		int MaxX = (SX <= EX) ? (int)EX : (int)SX;
		int MaxY = (SX <= EX) ? (int)EY : (int)SY;

		int dX = (MaxX - MinX);
		int dY = (MaxY - MinY);

		int YInc = (dY < 0) ? -1 : 1;
		dY = (dY < 0) ? -dY : dY;

		int dInc1 = 2 * dY;
		int dInc2 = 2 * (dY - dX);
		int D = (2 * dY) - dX;

		Pixels = Buffer->Data + Buffer->Width * MinY + MinX;
		int Pitch = Buffer->Width * YInc;
		int inc = 0;
		for(int X = MinX; X < MaxX; X++){
			*Pixels = Color;
			inc = (D > 0) ? (Pitch + 1) : 1;
			D += (D > 0) ? dInc2 : dInc1;
			Pixels += inc;
		}
	} else{
		int MinX = (SY <= EY) ? (int)SX : (int)EX;
		int MinY = (SY <= EY) ? (int)SY : (int)EY;
		int MaxX = (SY <= EY) ? (int)EX : (int)SX;
		int MaxY = (SY <= EY) ? (int)EY : (int)SY;

		int dX = (MaxX - MinX);
		int dY = (MaxY - MinY);

		int XInc = (dX < 0) ? -1 : 1;
		dX = (dX < 0) ? -dX : dX;

		int dInc1 = 2 * (-dX);
		int dInc2 = 2 * (dY - dX);
		int D = (2 * dY) - dX;

		Pixels = Buffer->Data + Buffer->Width * MinY + MinX;
		int Pitch = Buffer->Width;
		int inc = 0;
		for(int Y = MinY; Y < MaxY; Y++){
			*Pixels = Color;
			//printf("%d", D < 0 ? 1 : 0);
			inc = (D < 0) ? (Pitch + XInc) : Pitch;
			D += (D < 0) ? dInc2 : dInc1;
			Pixels += inc;
		}
	}
}


internal void fxd_BresenhamLine(IScreenBuffer *Buffer,
	int SX, int SY, //Line Start
	int EX, int EY, //Line End
	unsigned Color){
	ASSERT(Buffer);
	int SlopeX = EX - SX;
	int SlopeY = EY - SY;
	unsigned *Pixels;
	
	if(Abs32(SlopeX) >= Abs32(SlopeY)){
		//x-major lines
		int MinX = (SX <= EX) ? SX : EX;
		int MinY = (SX <= EX) ? SY : EY;
		int MaxX = (SX <= EX) ? EX : SX;
		int MaxY = (SX <= EX) ? EY : SY;

		int dX = (MaxX - MinX);
		int dY = (MaxY - MinY);

		int YInc = (dY < 0) ?  -(1 << FRACTION_BITS) : 1 << FRACTION_BITS;
		dY = (dY < 0) ? -dY : dY;

		int dInc1 = 2 * dY;
		int dInc2 = 2 * (dY - dX);
		int D = (2 * dY) - dX;

		Pixels = Buffer->Data + Buffer->Width * MinY + MinX;
		int Pitch = Buffer->Width * YInc;
		int inc = 0;
		for(int X = MinX; X < MaxX; X++){
			*Pixels = Color;
			inc = (D > 0) ? (Pitch + 1) : 1;
			D += (D > 0) ? dInc2 : dInc1;
			Pixels += inc;
		}
	} else{
		int MinX = (SY <= EY) ? (int)SX : (int)EX;
		int MinY = (SY <= EY) ? (int)SY : (int)EY;
		int MaxX = (SY <= EY) ? (int)EX : (int)SX;
		int MaxY = (SY <= EY) ? (int)EY : (int)SY;

		int dX = (MaxX - MinX);
		int dY = (MaxY - MinY);

		int XInc = (dX < 0) ? -1 : 1;
		dX = (dX < 0) ? -dX : dX;

		int dInc1 = 2 * (-dX);
		int dInc2 = 2 * (dY - dX);
		int D = (2 * dY) - dX;

		Pixels = Buffer->Data + Buffer->Width * MinY + MinX;
		int Pitch = Buffer->Width;
		int inc = 0;
		for(int Y = MinY; Y < MaxY; Y++){
			*Pixels = Color;
			//printf("%d", D < 0 ? 1 : 0);
			inc = (D < 0) ? (Pitch + XInc) : Pitch;
			D += (D < 0) ? dInc2 : dInc1;
			Pixels += inc;
		}
	}
}

internal void RasterizeRectangle(IScreenBuffer *Buffer,
	float Left, float Top,
	float Right, float Bottom,
	unsigned Color)
{
	unsigned MinX = RoundFloatToUInt(Left);
	unsigned MinY = RoundFloatToUInt(Top);
	unsigned MaxX = RoundFloatToUInt(Right);
	unsigned MaxY = RoundFloatToUInt(Bottom);
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
	unsigned *Row = Buffer->Data + MinX + MinY * Buffer->Width;

	for(unsigned Y = MinY; Y < MaxY; Y++){
		unsigned *Pixels = Row;
		for(unsigned X = MinX; X < MaxX; X++){
			*Pixels = Color;
			Pixels++;
		}
		Row += Buffer->Width;
	}
}

internal inline float fMin3(float A, float B, float C){
	float Min = A;
	Min = Min < B ? Min : B;
	Min = Min < C ? Min : C;
	return Min;
}


internal inline float fMax3(float A, float B, float C){
	float Max = A;
	Max = Max > B ? Max : B;
	Max = Max > C ? Max : C;
	return Max;
}

internal inline int TriOrientation(
	float AX, float AY,
	float BX, float BY,
	float CX, float CY)
{
	return (int)(((CX - AX) * (BY - AY)) + ((CY - AY) * (AX - BX)));
}

internal int TestTopLeftRule(float AX, float AY, float BX, float BY){
	int Horizontal = AY == BY;
	int DirLeft = (BX - AX) < 0;
	int TopEdge = Horizontal && DirLeft;
	int DownEdge = (BY - AY) < 0;
	return TopEdge || DownEdge;


}

internal void RasterizeTri(IScreenBuffer *Buffer, 
	float AX, float AY,
	float BX, float BY,
	float CX, float CY,
	unsigned Color)
{
	//https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

	int Orientation = TriOrientation(AX,AY,BX,BY,CX,CY);
	// Orientation < 0: Clockwise Orientation
	// Orientation == 0: Triangle with Area 0
	if(Orientation <= 0){
		return;
	}

	int MinX = RoundFloatToInt(fMin3(AX, BX, CX));
	int MinY = RoundFloatToInt(fMin3(AY, BY, CY));
	int MaxX = RoundFloatToInt(fMax3(AX, BX, CX));
	int MaxY = RoundFloatToInt(fMax3(AY, BY, CY));
	
	if(MinX < 0){
		MinX = 0;
	}
	if(MinY < 0){
		MinY = 0;
	}
	if(MaxX > Buffer->Width){
		MaxX = Buffer->Width;
	}
	if(MaxY > Buffer->Height){
		MaxY = Buffer->Height;
	}
	int Bias0 = TestTopLeftRule(AX, AY, BX, BY) ? 0 : -1;
	int Bias1 = TestTopLeftRule(CX, CY, AX, AY) ? 0 : -1;
	int Bias2 = TestTopLeftRule(BX, BY, CX, CY) ? 0 : -1;

	unsigned *Pixels = Buffer->Data + MinX + MinY * Buffer->Width;
	for(int Y = MinY; Y < MaxY; Y++){
		for(int X = MinX; X < MaxX; X++){
			int InsideTri[3];
			float PX = (float)X;
			float PY = (float)Y;
			InsideTri[0] = (TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0)>=0;
			InsideTri[1] = (TriOrientation(AX, AY, PX, PY, CX, CY) + Bias1)>=0;
			InsideTri[2] = (TriOrientation(BX, BY, CX, CY, PX, PY) + Bias2)>=0;
			
			if(InsideTri[0] && InsideTri[1] && InsideTri[2]){
				*Pixels = Color;
				
			}
			Pixels++;

		}
		Pixels += Buffer->Width - (MaxX - MinX);
	}
}

internal void RasterizeTriOutline(IScreenBuffer *Buffer, 
	float AX, float AY,
	float BX, float BY,
	float CX, float CY, 
	unsigned Color)
{
	BresenhamLine(Buffer, AX,AY,BX,BY, Color);
	BresenhamLine(Buffer, BX,BY,CX,CY, Color);
	BresenhamLine(Buffer, CX,CY,AX,AY, Color);
}

internal void NaiveRasterizeTriOutline(IScreenBuffer *Buffer,
	float AX, float AY,
	float BX, float BY,
	float CX, float CY,
	unsigned Color)
{
	NaiveRasterizeLine(Buffer, AX, AY, BX, BY, Color);
	NaiveRasterizeLine(Buffer, BX, BY, CX, CY, Color);
	NaiveRasterizeLine(Buffer, CX, CY, AX, AY, Color);
}




internal void NaiveRasterizePoint(IScreenBuffer *Buffer,
	float X, float Y,
	unsigned Color)
{
	unsigned *Pixels = Buffer->Data;
	
	float CurrentPosX = (float)RoundFloat(X);
	float CurrentPosY = (float)RoundFloat(Y);
	int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
	Pixels[Index] = Color;
}

internal void RasterizePoint(IScreenBuffer* Buffer, 
	float X, float Y,
	unsigned Color) 
{
	//Requires Subpixel precision
}


internal void NaiveRasterizeCircleOutline(IScreenBuffer *Buffer, 
	float X,float Y,
	float Radius, unsigned Color)
{
	ASSERT(Buffer);
	float Resolution = 0.0001f;
	unsigned *Pixels = Buffer->Data;

	for(float a = 0; a < 2 * PI; a += Resolution){
		
		float CurrentPosX = X + (float)RoundFloat((float)cos(a) * Radius);
		float CurrentPosY = Y + (float)RoundFloat((float)sin(a) * Radius);

		if(CurrentPosX >= Buffer->Width || CurrentPosY >= Buffer->Height){
			continue;
		}
		if(CurrentPosX < 0 || CurrentPosY < 0){
			continue;
		}
		int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
		Pixels[Index] = Color;
	}



}




internal void NaiveRasterizeQuadraticBezierCurve(IScreenBuffer *Buffer, 
	float SX, float SY, //Start
	float EX, float EY, //End
	float CX, float CY, //Control Point
	unsigned Color)
{
	unsigned *Pixels = Buffer->Data;
	float Resolution = 0.0001f;
	
	for(float t = 0; t <= 1; t += Resolution){
		
		float CurrentPosX = RoundFloat((1 - t) * (1 - t) * SX + 2 * (1 - t) * t * CX + t * t * EX);
		float CurrentPosY = RoundFloat((1 - t) * (1 - t) * SY + 2 * (1 - t) * t * CY + t * t * EY);


		if(CurrentPosX >= Buffer->Width || CurrentPosY >= Buffer->Height){
			continue;
		}
		if(CurrentPosX < 0 || CurrentPosY < 0){
			continue;
		}

		int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
		Pixels[Index] = Color;
	}
}


internal void NaiveRasterizeCubicBezierCurve(IScreenBuffer *Buffer, 
	float P0X, float P0Y,
	float P1X, float P1Y,
	float P2X, float P2Y,
	float P3X, float P3Y,
	unsigned Color)
{
	unsigned *Pixels = Buffer->Data;
	float Resolution = 0.0001f;
	for(float t = 0; t <= 1; t += Resolution){
		
		float CurrentPosX = RoundFloat(P0X + t * (-3 * P0X + 3 * P1X) + t * t * (3 * P0X - 6 * P1X + 3 * P2X) + t * t * t * (-P0X + 3 * P1X - 3 * P2X + P3X));
		float CurrentPosY = RoundFloat(P0Y + t * (-3 * P0Y + 3 * P1Y) + t * t * (3 * P0Y - 6 * P1Y + 3 * P2Y) + t * t * t * (-P0Y + 3 * P1Y - 3 * P2Y + P3Y));

		if(CurrentPosX >= Buffer->Width || CurrentPosY >= Buffer->Height){
			continue;
		}
		if(CurrentPosX < 0 || CurrentPosY < 0){
			continue;
		}

		int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
		Pixels[Index] = Color;
	}
}



internal void SetBackgroundColor(IScreenBuffer *Buffer, unsigned BackgroundColor){
	unsigned *CurrentPixel = Buffer->Data;
	for(int Col = 0; Col < Buffer->Width; Col++){
		for(int Row = 0; Row < Buffer->Height; Row++){
			*CurrentPixel = BackgroundColor;
			CurrentPixel++;
		}
	}
}
IDebugContext *GlobalDebugCtx;
global int Init = 0;
RENDER_FRAME(RenderFrame){
	if(!Init){
		printf("fxd_min: %d\n", fxd_min);
		printf("fxd_max: %d\n", fxd_max);
		printf("float_fxd_min: %f\n", float_fxd_min);
		printf("float_fxd_min: %f\n", float_fxd_min);
		Init++;
	}
	unsigned x = 0x4b000000;
	//float y = *((float *)&x);
	float y = -8388608.0f;
	int z = (int)y;
	//const float *xx = &float_fxd_min;
	*EnableFrameTime = 0;
	GlobalDebugCtx = DebugCTX;
	ASSERT(Buffer);
	unsigned *Pixels = Buffer->Data;
	ASSERT(Pixels);
	unsigned BackgroundColor = 0xffffffff;
	SetBackgroundColor(Buffer, BackgroundColor);
	static float X1 = 100;static float Y1 = 100;
	static float X2 = 100;static float Y2 = 500;
	static float X3 = 500;static float Y3 = 500;
	static float X4 = 500;static float Y4 = 100;

	int& Test = XOffset;
	Test = 5;

	if(Input->LeftClick){
		X1 = Input->MouseX;
		Y1 = Input->MouseY;
	}
	if(Input->RightClick){
		X2 = Input->MouseX;
		Y2 = Input->MouseY;
	}
	if(Input->MiddleClick){
		X3 = Input->MouseX;
		Y3 = Input->MouseY;
	}

	

	RasterizeTri(Buffer, 100,100,100,500,500,500, 0xffffff00);
	RasterizeRectangle(Buffer, X1, Y1, X3, Y3,0xffff0000);
	NaiveRasterizeQuadraticBezierCurve(Buffer, X1, Y1, X3, Y3, (X3-X1)/4.0f + X1, 3.0f * (Y3 - Y1) / 2.0f + Y1, 0xff0000ff);
	NaiveRasterizeCircleOutline(Buffer, ((X3 - X1) / 4.0f) + X1, (3.0f * (Y3 - Y1)) / 4.0f + Y1, 2.0f,0xff00ff00);




}