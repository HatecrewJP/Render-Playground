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



internal void RasterizeTriOutline(IScreenBuffer *Buffer, 
	int AX, int AY,
	int BX, int BY,
	int CX, int CY, 
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

internal void fxd_RasterizePoint2(IScreenBuffer* Buffer, 
	fxd_24_8 X, fxd_24_8 Y,
	unsigned Color) 
{
	fxd_RasterizeTri(Buffer, X - FXD_HALF, Y - FXD_HALF, X - FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y - FXD_HALF, Color);
	fxd_RasterizeTri(Buffer, X - FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y - FXD_HALF, Color);
}

internal void fxd_RasterizePoint3(IScreenBuffer *Buffer,
	fxd_24_8 X, fxd_24_8 Y,
	unsigned Color){
	fxd_IncRasterizeTri2(Buffer, X - FXD_HALF, Y - FXD_HALF, X - FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y - FXD_HALF, Color);
	fxd_IncRasterizeTri2(Buffer, X - FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y + FXD_HALF, X + FXD_HALF, Y - FXD_HALF, Color);
}

internal void fxd_RasterizePoint(IScreenBuffer *Buffer,
	fxd_24_8 X, fxd_24_8 Y,
	unsigned Color)
{	
	fxd_24_8 TmpX = fxd_Floor(X - FXD_FRAC_MIN) + FXD_HALF;
	fxd_24_8 TmpY = fxd_Floor(Y - FXD_FRAC_MIN) + FXD_HALF;
	if(TmpX >= 0 && TmpX < IntToFxd(Buffer->Width) && TmpY >= 0 && TmpY < IntToFxd(Buffer->Height)){
		fxd_SetPixel(Buffer, TmpX, TmpY, Color);

	}
	
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
RENDER_FRAME(RenderFrame){
	int &Init = *(int*)MemBuffer;

	float *State = (float*) ((byte*)MemBuffer + sizeof(Init));
	static int Direction = 1;
	if(Input->KeyR){
		Init = 0;
	}

	if(!Init){
		State[0] =  2262.5f; State[1] =  351.5f;
		State[2] =  130.5f; State[3] = 846.5f;
		State[4] = 846.5f; State[5] = 846.5f;
		State[6] = 1100.0f; State[7] =  100.0f;


		printf("fxd_min: %d\n", fxd_min);
		printf("fxd_max: %d\n", fxd_max);
		printf("float_fxd_min: %f\n", float_fxd_min);
		printf("float_fxd_max: %f\n", float_fxd_max);

		fxd_24_8 A = FloatToFixed(-8465.00390624f);
		fxd_24_8 B = FloatToFixed(-8465.00390624f);

		char SA[32];
		char SB[32];
		fxd_FormatFxd(A, SA, 32); 
		fxd_FormatFxd(B, SB, 32); 

		printf("A: %s\n",SA);
		printf("B: %s\n",SB);



		Init++;
	}
	
	*EnableFrameTime = 0;
	GlobalDebugCtx = DebugCTX;
	Assert(Buffer);
	unsigned *Pixels = Buffer->Data;
	Assert(Pixels);
	unsigned BackgroundColor = 0xffffffff;
	SetBackgroundColor(Buffer, BackgroundColor);
	static float &X1 = State[0]; static float &Y1 = State[1];
	static float &X2 = State[2]; static float &Y2 = State[3];
	static float &X3 = State[4]; static float &Y3 = State[5];
	static float &X4 = State[6]; static float &Y4 = State[7];
	
	if(*XOffset > 256){
		Direction = -1;
	}
	if (*XOffset < -128) {
		Direction = 1;
	}

	*XOffset += Direction * 8;

	/*X1 = 100.5f + 0.5f * (float)*XOffset;
	X2 = 100.5f + 0.5f * (float)*XOffset;
	X3 = 500.5f + 0.5f * (float)*XOffset;
	X4 = 500.5f + 0.5f * (float)*XOffset;

	Y3 = 500.5f + 0.5f * (float)*XOffset;*/

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
	if(Input->KeySpace){
		X4 = Input->MouseX;
		Y4 = Input->MouseY;
	}
	
	IScreenBuffer TmpBuffer = {0};
	TmpBuffer.Width = 26;
	TmpBuffer.Height = 16;
	TmpBuffer.PixelSize = 4;
	TmpBuffer.Data = (unsigned *)((byte*)MemBuffer + 4096);
	Buffer = &TmpBuffer;

	SetBackgroundColor(Buffer, BackgroundColor);

	fxd_24_8 fxd_X1 = FloatToFixed(17.0f); fxd_24_8 fxd_Y1 = FloatToFixed(1.5f);
	fxd_24_8 fxd_X2 = FloatToFixed(5.0f); fxd_24_8 fxd_Y2 = FloatToFixed(8.0f);
	fxd_24_8 fxd_X3 = FloatToFixed(9.0f); fxd_24_8 fxd_Y3 = FloatToFixed(8.0f);
	
	fxd_IncRasterizeTri(Buffer, fxd_X1, fxd_Y1, fxd_X2, fxd_Y2, fxd_X3, fxd_Y3, 0xffff0000);
		
	fxd_RasterizePoint(Buffer, fxd_X1, fxd_Y1,0xffff00ff);
	fxd_RasterizePoint(Buffer, fxd_X2, fxd_Y2,0xffff00ff);
	fxd_RasterizePoint(Buffer, fxd_X3, fxd_Y3,0xffff00ff);


	fxd_24_8 fxd_X4 = FloatToFixed(1.5f); fxd_24_8 fxd_Y4 = FloatToFixed(3.5f);
	fxd_24_8 fxd_X5 = FloatToFixed(7.5f); fxd_24_8 fxd_Y5 = FloatToFixed(2.5f);
	fxd_RasterizePoint(Buffer, fxd_X4, fxd_Y4, 0xffff00ff);
	fxd_RasterizePoint(Buffer, fxd_X5, fxd_Y5, 0xffff00ff);
	fxd_BresenhamLine(Buffer, fxd_X5, fxd_Y5, fxd_X4, fxd_Y4, 0xff000000);

	


}