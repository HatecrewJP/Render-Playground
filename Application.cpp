#include "Application.h"

internal inline void SetPixel(IScreenBuffer *Buffer, int X, int Y, unsigned Color){
	//Assumes that the point is valid
	//Pixel center is at 0,0
	unsigned *Pixel = Buffer->Data + Y * Buffer->Width + X;
	*Pixel = Color;
}

#include "math.h"
#include "stdio.h"
#include "memory.h"

#include "fxd_tri.h"
#include "line.h"
#include "tri.h"

#include "rpg_math.h"


internal void SetBackgroundColor(IScreenBuffer *Buffer, unsigned BackgroundColor){
	unsigned *Pixels = Buffer->Data;
	for(int i = 0; i < (Buffer->Width * Buffer->Height); i++){
		*Pixels = BackgroundColor;
		Pixels++;
	}
}

internal void fxd_RasterizeRectangle(IScreenBuffer *Buffer,
	fxd_24_8 Left, fxd_24_8 Top,
	fxd_24_8 Right, fxd_24_8 Bottom,
	unsigned Color){
	fxd_24_8 MinX = fxd_Round(Left - FXD_HALF) + FXD_HALF;
	fxd_24_8 MinY = fxd_Round(Top - FXD_HALF) + FXD_HALF;
	fxd_24_8 MaxX = fxd_Round(Right - FXD_HALF) + FXD_HALF;
	fxd_24_8 MaxY = fxd_Round(Bottom - FXD_HALF) + FXD_HALF;

	if(MinX < FXD_HALF){
		MinX = FXD_HALF;
	}
	if(MinY < FXD_HALF){
		MinY = FXD_HALF;
	}
	if(MaxX >= IntToFxd(Buffer->Width)){
		MaxX = IntToFxd(Buffer->Width);
	}
	if(MaxY >= IntToFxd(Buffer->Height)){
		MaxY = IntToFxd(Buffer->Height);
	}
	unsigned *Row = Buffer->Data + FxdToIntF(MinY) * Buffer->Width + FxdToIntR(MinX);
	for(fxd_24_8 Y = MinY; Y < MaxY; Y += FXD_ONE){
		unsigned *Pixels = Row;
		for(fxd_24_8 X = MinX; X < MaxX; X += FXD_ONE){
			*Pixels = Color;
			Pixels++;
		}
		Row += Buffer->Width;
	}
}

internal void DrawQuad(IScreenBuffer *Buffer,
	int X1, int Y1,
	int X2, int Y2,
	int X3, int Y3,
	int X4, int Y4,
	unsigned Color)
{
	//Diagonal is 2-4
	RasterizeTri(Buffer, X1, Y1, X2, Y2, X4, Y4, Color);
	RasterizeTri(Buffer, X2, Y2, X3, Y3, X4, Y4, Color);
}

internal void fxd_DrawQuad(IScreenBuffer *Buffer,
	fxd_24_8 X1, fxd_24_8 Y1,
	fxd_24_8 X2, fxd_24_8 Y2,
	fxd_24_8 X3, fxd_24_8 Y3,
	fxd_24_8 X4, fxd_24_8 Y4,
	unsigned Color)
{
	//Diagonal is 2-4
	fxd_RasterizeTri(Buffer, X1, Y1, X2, Y2, X4, Y4, Color);
	fxd_RasterizeTri(Buffer, X2, Y2, X3, Y3, X4, Y4, Color);
}

internal void fxd_DrawQuadOutline(IScreenBuffer *Buffer,
	fxd_24_8 X1, fxd_24_8 Y1,
	fxd_24_8 X2, fxd_24_8 Y2,
	fxd_24_8 X3, fxd_24_8 Y3,
	fxd_24_8 X4, fxd_24_8 Y4,
	unsigned Color)
{
	//Diagonal is 2-4
	fxd_BresenhamLine2(Buffer, X1, Y1, X2, Y2, Color);
	fxd_BresenhamLine2(Buffer, X2, Y2, X3, Y3, Color);
	fxd_BresenhamLine2(Buffer, X3, Y3, X4, Y4, Color);
	fxd_BresenhamLine2(Buffer, X4, Y4, X1, Y1, Color);
}


internal void RasterizeTriOutline(IScreenBuffer *Buffer, 
	int AX, int AY,
	int BX, int BY,
	int CX, int CY, 
	unsigned Color)
{
	BresenhamLine2(Buffer, AX,AY,BX,BY, Color);
	BresenhamLine2(Buffer, BX,BY,CX,CY, Color);
	BresenhamLine2(Buffer, CX,CY,AX,AY, Color);
}

internal void RasterizePoint(IScreenBuffer *Buffer,
	int X, int Y,
	unsigned Color)
{
	if(X >= 0 && X <= Buffer->Width && Y >= 0 && Y <= Buffer->Height){
		*(Buffer->Data + Y * Buffer->Width + X) = Color;
	}
}

internal void fxd_RasterizePoint(IScreenBuffer *Buffer,
	fxd_24_8 _X, fxd_24_8 _Y,
	unsigned Color)
{
	fxd_24_8 X = FxdToIntF(_X - FXD_FRAC_MIN);
	fxd_24_8 Y = FxdToIntF(_Y - FXD_FRAC_MIN);
	if(X >= 0 && X <= Buffer->Width && Y >= 0 && Y <= Buffer->Height){
		*(Buffer->Data + Y * Buffer->Width + X) = Color;
	}
}

internal void fxd_RasterizePoint2(IScreenBuffer *Buffer,
	fxd_24_8 X, fxd_24_8 Y,
	unsigned Color){
	fxd_24_8 TmpX = fxd_Floor(X - FXD_FRAC_MIN) + FXD_HALF;
	fxd_24_8 TmpY = fxd_Floor(Y - FXD_FRAC_MIN) + FXD_HALF;
	if(TmpX >= 0 && TmpX <= (IntToFxd(Buffer->Width) - FXD_HALF) && TmpY >= 0 && TmpY <= (IntToFxd(Buffer->Height) - FXD_HALF)){
		fxd_SetPixel(Buffer, TmpX, TmpY, Color);
	}
}

internal void fxd_RasterizePoint3(IScreenBuffer *Buffer,
	fxd_24_8 X, fxd_24_8 Y,
	unsigned Color){
	fxd_RasterizeTri(Buffer, X - FXD_HALF, Y - FXD_HALF, X - FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y - FXD_HALF, Color);
	fxd_RasterizeTri(Buffer, X - FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y - FXD_HALF, Color);
}



internal void NaiveRasterizeCircleOutline(IScreenBuffer *Buffer, 
	float X,float Y,
	float Radius, unsigned Color)
{
	Assert(Buffer);
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



IDebugContext *GlobalDebugCtx;
RENDER_FRAME(RenderFrame){
	int &Init = *(int*)MemBuffer;
	*EnableFrameTime = 1;
	GlobalDebugCtx = DebugCTX;
	Assert(Buffer);
	Assert(Buffer->Data);
	if(Input->KeyR){
		Init = 0;
	}

	if(!Init){
		printf("fxd_min: %d\n", fxd_min);
		printf("fxd_max: %d\n", fxd_max);
		printf("float_fxd_min: %f\n", float_fxd_min);
		printf("float_fxd_max: %f\n", float_fxd_max);

		Init++;
	}

	
	unsigned BackgroundColor = 0xffa0a0a0;
	SetBackgroundColor(Buffer, BackgroundColor);

	float X = 1200.0f;
	float Y = 700.0f;
	float Width = 800.0f;
	float Height = 800.0f;
	static float Angle = 3.0f;


	float X1, X2, X3, X4;
	float Y1, Y2, Y3, Y4;

	X1 = X - (Width / 2); Y1 = Y - (Height / 2);
	X2 = X - (Width / 2); Y2 = Y + (Height / 2);
	X3 = X + (Width / 2); Y3 = Y + (Height / 2);
	X4 = X + (Width / 2); Y4 = Y - (Height / 2);

	float RA, RB, RC, RD;

	RA =  cosf(Angle);
	RB =  sinf(Angle);
	RC = -sinf(Angle);
	RD =  cosf(Angle);

	float NX1, NX2, NX3, NX4;
	float NY1, NY2, NY3, NY4;

	X1 = X1 - X;
	X2 = X2 - X;
	X3 = X3 - X;
	X4 = X4 - X;

	Y1 = Y1 - Y;
	Y2 = Y2 - Y;
	Y3 = Y3 - Y;
	Y4 = Y4 - Y;

	NX1 = RA * X1 + RC * Y1;
	NX2 = RA * X2 + RC * Y2;
	NX3 = RA * X3 + RC * Y3;
	NX4 = RA * X4 + RC * Y4;

	NY1 = RB * X1 + RD * Y1;
	NY2 = RB * X2 + RD * Y2;
	NY3 = RB * X3 + RD * Y3;
	NY4 = RB * X4 + RD * Y4;

	NX1 = NX1 + X;
	NX2 = NX2 + X;
	NX3 = NX3 + X;
	NX4 = NX4 + X;

	NY1 = NY1 + Y;
	NY2 = NY2 + Y;
	NY3 = NY3 + Y;
	NY4 = NY4 + Y;

	fxd_24_8 fxd_X1 = FloatToFxd(NX1); fxd_24_8 fxd_Y1 = FloatToFxd(NY1);
	fxd_24_8 fxd_X2 = FloatToFxd(NX2); fxd_24_8 fxd_Y2 = FloatToFxd(NY2);
	fxd_24_8 fxd_X3 = FloatToFxd(NX3); fxd_24_8 fxd_Y3 = FloatToFxd(NY3);
	fxd_24_8 fxd_X4 = FloatToFxd(NX4); fxd_24_8 fxd_Y4 = FloatToFxd(NY4);
	


	fxd_DrawQuad(Buffer, fxd_X1, fxd_Y1, fxd_X2, fxd_Y2, fxd_X3, fxd_Y3, fxd_X4, fxd_Y4, 0xffff0000);
	fxd_DrawQuadOutline(Buffer, fxd_X1, fxd_Y1, fxd_X2, fxd_Y2, fxd_X3, fxd_Y3, fxd_X4, fxd_Y4, 0xffffffff);
	
	//fxd_TriHalfPlanes(Buffer, fxd_X1, fxd_Y1, fxd_X2, fxd_Y2, fxd_X3, fxd_Y3);

	fxd_RasterizePoint(Buffer, fxd_X1, fxd_Y1,0xff00ff00);
	fxd_RasterizePoint(Buffer, fxd_X2, fxd_Y2,0xff00ff00);
	fxd_RasterizePoint(Buffer, fxd_X3, fxd_Y3,0xff00ff00);
	fxd_RasterizePoint(Buffer, fxd_X4, fxd_Y4,0xff00ff00);

	Angle += PI / 360;


	






}