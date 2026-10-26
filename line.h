#pragma once
#include "Application.h"
#include "rpg_math.h"


internal void NaiveRasterizeLine(IScreenBuffer *Buffer,
	float SX, float SY,
	float EX, float EY,
	unsigned Color){
	Assert(Buffer);
	unsigned *Pixels = Buffer->Data;
	Assert(Pixels);


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
	int SX, int SY, //Line Start
	int EX, int EY, //Line End
	unsigned Color){
	Assert(Buffer);
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
		int MinX = (SY <= EY) ? SX : EX;
		int MinY = (SY <= EY) ? SY : EY;
		int MaxX = (SY <= EY) ? EX : SX;
		int MaxY = (SY <= EY) ? EY : SY;

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
			inc = (D < 0) ? (Pitch + XInc) : Pitch;
			D += (D < 0) ? dInc2 : dInc1;
			Pixels += inc;
		}
	}
}

internal void fxd_BresenhamLine(IScreenBuffer *Buffer,
	fxd_24_8 SX, fxd_24_8 SY, //Line Start
	fxd_24_8 EX, fxd_24_8 EY, //Line End
	unsigned Color)
{
	Assert(Buffer);
	fxd_24_8 SlopeX = EX - SX;
	fxd_24_8 SlopeY = EY - SY;

	if(Abs32(SlopeX) >= Abs32(SlopeY)){
		fxd_24_8 MinX = (SX <= EX) ? SX : EX;
		fxd_24_8 MinY = (SX <= EX) ? SY : EY;
		fxd_24_8 MaxX = (SX <= EX) ? EX : SX;
		fxd_24_8 MaxY = (SX <= EX) ? EY : SY;

		fxd_24_8 dX = (MaxX - MinX);
		fxd_24_8 dY = (MaxY - MinY);

		MinX = fxd_Floor(MinX) + FXD_HALF;
		MinY = fxd_Floor(MinY) + FXD_HALF;

		MaxX = fxd_Floor(MaxX) + FXD_HALF;
		MaxY = fxd_Floor(MaxY) + FXD_HALF;

		int YInc = (dY < 0) ? -FXD_ONE : FXD_ONE;
		//x-major lines

		dY = (dY < 0) ? NegateFxd(dY) : dY;

		fxd_24_8 dInc1 = 2 * dY;
		fxd_24_8 dInc2 = 2 * (dY - dX);
		fxd_24_8 D = (2 * dY) - dX;

		fxd_24_8 Y = MinY;
		fxd_24_8 X = MinX;
		for(; X < MaxX; X += FXD_ONE){
			fxd_SetPixel(Buffer, X, Y, Color);
			Y += (D > 0) ? YInc : 0;
			D += (D > 0) ? dInc2 : dInc1;
		}
	} else{
		fxd_24_8 MinX = (SY <= EY) ? SX : EX;
		fxd_24_8 MinY = (SY <= EY) ? SY : EY;
		fxd_24_8 MaxX = (SY <= EY) ? EX : SX;
		fxd_24_8 MaxY = (SY <= EY) ? EY : SY;

		
		fxd_24_8 dX = (MaxX - MinX);
		fxd_24_8 dY = (MaxY - MinY);
		
		
		

		fxd_24_8 XInc = (dX < 0) ? -FXD_ONE : FXD_ONE;
		dX = (dX < 0) ? NegateFxd(dX) : dX;

		fxd_24_8 dInc1 = 2 * (-dX);
		fxd_24_8 dInc2 = 2 * (dY - dX);
		fxd_24_8 D = (2 * dY) - dX;

		fxd_24_8 X = MinX;
		fxd_24_8 Y = MinY;
		for(; Y < MaxY; Y += FXD_ONE){
			fxd_SetPixel(Buffer, X, Y, Color);
			X += (D < 0) ? XInc : 0;
			D += (D < 0) ? dInc2 : dInc1;

		}
	}
}
