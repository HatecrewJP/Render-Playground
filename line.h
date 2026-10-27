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

		int dX = (EX - EX);
		int dY = (EY - EY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;
		
		dY = Abs32(dX);
		dX = Abs32(dY);

		int dInc1 = 2 * dY;
		int dInc2 = 2 * (dY - dX);
		int D = (2 * dY) - dX;

		Pixels = Buffer->Data + Buffer->Width * SY + SX;
		int Pitch = Buffer->Width * YInc;
		int IterationCount = dX;
		for(int i = 0; i < IterationCount; i++){
			*Pixels = Color;
			Pixels += (D > 0) ? (Pitch + 1) : 1;
			D += (D > 0) ? dInc2 : dInc1;
		}
	} else{
		int dX = (EX - EX);
		int dY = (EY - EY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;

		dY = Abs32(dX);
		dX = Abs32(dY);

		int dInc1 = 2 * (-dX);
		int dInc2 = 2 * (dY - dX);
		int D = dY - (2 * dX);

		Pixels = Buffer->Data + Buffer->Width * SY + SX;
		int Pitch = Buffer->Width;
		int IterationCount = dY;
		for(int i = 0; i < IterationCount; i++){
			*Pixels = Color;
			Pixels += (D < 0) ? (Pitch + XInc) : Pitch;
			D += (D < 0) ? dInc2 : dInc1;
		}
	}
}

internal void BresenhamLine2(IScreenBuffer *Buffer,
	int SX, int SY, //Line Start
	int EX, int EY, //Line End
	unsigned Color)
{
	//Sets the end Pixel
	Assert(Buffer);
	int SlopeX = EX - SX;
	int SlopeY = EY - SY;
	unsigned *Pixels;

	if(Abs32(SlopeX) >= Abs32(SlopeY)){
		//x-major lines

		int dX = (EX - EX);
		int dY = (EY - EY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;

		dY = Abs32(dX);
		dX = Abs32(dY);

		int dInc1 = 2 * dY;
		int dInc2 = 2 * (dY - dX);
		int D = (2 * dY) - dX;

		Pixels = Buffer->Data + Buffer->Width * SY + SX;
		int Pitch = Buffer->Width * YInc;
		int IterationCount = dX;
		for(int i = 0; i <= IterationCount; i++){
			*Pixels = Color;
			Pixels += (D > 0) ? (Pitch + 1) : 1;
			D += (D > 0) ? dInc2 : dInc1;
		}
	} else{
		int dX = (EX - EX);
		int dY = (EY - EY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;

		dY = Abs32(dX);
		dX = Abs32(dY);

		int dInc1 = 2 * (-dX);
		int dInc2 = 2 * (dY - dX);
		int D = dY - (2 * dX);

		Pixels = Buffer->Data + Buffer->Width * SY + SX;
		int Pitch = Buffer->Width;
		int IterationCount = dY;
		for(int i = 0; i <= IterationCount; i++){
			*Pixels = Color;
			Pixels += (D < 0) ? (Pitch + XInc) : Pitch;
			D += (D < 0) ? dInc2 : dInc1;
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
		//x-major lines
		fxd_24_8 dX = (EX - SX);
		fxd_24_8 dY = (EY - SY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;

		dY = fxd_Abs(dY);
		dX = fxd_Abs(dX);

		fxd_24_8 dInc1 = 2 * dY;
		fxd_24_8 dInc2 = 2 * (dY - dX);
		fxd_24_8 D = (2 * dY) - dX;

		int IterationCount = FxdToIntF(dX);
		unsigned *Pixels = Buffer->Data + Buffer->Width * FxdToIntF(SY) + FxdToIntF(SX);
		int Pitch = Buffer->Width * YInc;
		int inc = 0;

		for(int i = 0; i < IterationCount; i++){
			*Pixels = Color;
			Pixels += (D > 0) ? (XInc + Pitch) : XInc;
			D += (D > 0) ? dInc2 : dInc1;
		}
	} else{
		fxd_24_8 dX = (EX - SX);
		fxd_24_8 dY = (EY - SY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;
		
		dX = fxd_Abs(dX);
		dY = fxd_Abs(dY);
		
		fxd_24_8 dInc1 = 2 * (-dX);
		fxd_24_8 dInc2 = 2 * (dY - dX);
		fxd_24_8 D = dY - (2 * dX);

		int IterationCount = FxdToIntF(dY);
		unsigned *Pixels = Buffer->Data + Buffer->Width * FxdToIntF(SY) + FxdToIntF(SX);
		int Pitch = Buffer->Width * YInc;
		int inc = 0;
		for(int i = 0; i < IterationCount; i++){
			*Pixels = Color;
			Pixels += (D < 0) ? (XInc + Pitch): Pitch;
			D += (D < 0) ? dInc2 : dInc1;
		}
	}
}

internal void fxd_BresenhamLine2(IScreenBuffer *Buffer,
	fxd_24_8 SX, fxd_24_8 SY, //Line Start
	fxd_24_8 EX, fxd_24_8 EY, //Line End
	unsigned Color)
{
	//Sets the end Pixel
	Assert(Buffer);
	fxd_24_8 SlopeX = EX - SX;
	fxd_24_8 SlopeY = EY - SY;

	if(Abs32(SlopeX) >= Abs32(SlopeY)){
		//x-major lines
		fxd_24_8 dX = (EX - SX);
		fxd_24_8 dY = (EY - SY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;

		dY = fxd_Abs(dY);
		dX = fxd_Abs(dX);

		fxd_24_8 dInc1 = 2 * dY;
		fxd_24_8 dInc2 = 2 * (dY - dX);
		fxd_24_8 D = (2 * dY) - dX;

		int IterationCount = FxdToIntF(dX);
		unsigned *Pixels = Buffer->Data + Buffer->Width * FxdToIntF(SY) + FxdToIntF(SX);
		int Pitch = Buffer->Width * YInc;
		int inc = 0;

		for(int i = 0; i <= IterationCount; i++){
			*Pixels = Color;
			Pixels += (D > 0) ? (XInc + Pitch) : XInc;
			D += (D > 0) ? dInc2 : dInc1;
		}
	} else{
		fxd_24_8 dX = (EX - SX);
		fxd_24_8 dY = (EY - SY);

		int YInc = (dY < 0) ? -1 : 1;
		int XInc = (dX < 0) ? -1 : 1;

		dX = fxd_Abs(dX);
		dY = fxd_Abs(dY);

		fxd_24_8 dInc1 = 2 * (-dX);
		fxd_24_8 dInc2 = 2 * (dY - dX);
		fxd_24_8 D = dY - (2 * dX);

		int IterationCount = FxdToIntF(dY);
		unsigned *Pixels = Buffer->Data + Buffer->Width * FxdToIntF(SY) + FxdToIntF(SX);
		int Pitch = Buffer->Width * YInc;
		int inc = 0;
		for(int i = 0; i <= IterationCount; i++){
			*Pixels = Color;
			Pixels += (D < 0) ? (XInc + Pitch) : Pitch;
			D += (D < 0) ? dInc2 : dInc1;
		}
	}
}
