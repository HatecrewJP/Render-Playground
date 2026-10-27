#pragma once
#include "Application.h"
#include "rpg_math.h"

internal inline int TriOrientation(
	int AX, int AY,
	int BX, int BY,
	int CX, int CY){
	//Y-Down coordinate System
	return ((BX - AX) * (CY - AY)) - ((BY - AY) * (CX - AX));
}

internal int TestTopLeftRule(int AX, int AY, int BX, int BY){
	bool Horizontal = (AY == BY);
	bool DirLeft = (BX - AX) < 0;
	bool TopEdge = Horizontal && DirLeft;
	bool DownEdge = (AY - BY) < 0;
	return TopEdge || DownEdge;
}

internal void RasterizeTri(IScreenBuffer *Buffer,
	int _AX, int _AY,
	int _BX, int _BY,
	int _CX, int _CY,
	unsigned Color)
{
	int AX = _AX; int AY = _AY;
	int BX = _CX; int BY = _CY;
	int CX = _BX; int CY = _BY;

	//https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

	int Orientation = TriOrientation(AX, AY, BX, BY, CX, CY);
	// Orientation < 0: Clockwise Orientation
	// Orientation == 0: Triangle with Area 0
	if(Orientation <= 0){
		return;
	}

	int MinX = Min3(AX, BX, CX);
	int MinY = Min3(AY, BY, CY);
	int MaxX = Max3(AX, BX, CX);
	int MaxY = Max3(AY, BY, CY);

	if(MinX < 0){
		MinX = 0;
	}
	if(MinY < 0){
		MinY = 0;
	}
	if(MaxX >= Buffer->Width){
		MaxX = Buffer->Width - 1;
	}
	if(MaxY >= Buffer->Height){
		MaxY = Buffer->Height - 1;
	}

	int Inc0 = (AY - BY);
	int Inc1 = (BY - CY);
	int Inc2 = (CY - AY);

	int RowInc0 = (BX - AX);
	int RowInc1 = (CX - BX);
	int RowInc2 = (AX - CX);

	int Bias0 = TestTopLeftRule(BX, BY, AX, AY) ? 0 : -1;
	int Bias1 = TestTopLeftRule(CX, CY, BX, BY) ? 0 : -1;
	int Bias2 = TestTopLeftRule(AX, AY, CX, CY) ? 0 : -1;

	int PX = MinX;
	int PY = MinY;

	int W0Row = TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0;
	int W1Row = TriOrientation(BX, BY, CX, CY, PX, PY) + Bias1;
	int W2Row = TriOrientation(AX, AY, PX, PY, CX, CY) + Bias2;



	unsigned *Row = Buffer->Data + PX + PY * Buffer->Width;
	for(int Y = MinY; Y <= MaxY; Y++){
		int W0 = W0Row;
		int W1 = W1Row;
		int W2 = W2Row;
		unsigned *Pixels = Row;
		for(int X = MinX; X <= MaxX; X++){
			*Pixels = ((W0 | W1 | W2) >= 0) ? Color : *Pixels;
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;
			Pixels++;
		}
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;
		Row += Buffer->Width;
	}
}

internal void TriHalfPlanes(IScreenBuffer *Buffer,
	int _AX, int _AY,
	int _BX, int _BY,
	int _CX, int _CY,
	unsigned Color)
{
	int AX = _AX; int AY = _AY;
	int BX = _CX; int BY = _CY;
	int CX = _BX; int CY = _BY;

	//https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

	int Orientation = TriOrientation(AX, AY, BX, BY, CX, CY);
	// Orientation < 0: Clockwise Orientation
	// Orientation == 0: Triangle with Area 0
	if(Orientation <= 0){
		return;
	}
	//Bounding Box
	int MinX = Min3(AX, BX, CX);
	int MinY = Min3(AY, BY, CY);
	int MaxX = Max3(AX, BX, CX);
	int MaxY = Max3(AY, BY, CY);

	if(MinX < 0){
		MinX = 0;
	}
	if(MinY < 0){
		MinY = 0;
	}
	if(MaxX >= Buffer->Width){
		MaxX = Buffer->Width - 1;
	}
	if(MaxY >= Buffer->Height){
		MaxY = Buffer->Height - 1;
	}

	int Inc0 = (AY - BY);
	int Inc1 = (BY - CY);
	int Inc2 = (CY - AY);

	int RowInc0 = (BX - AX);
	int RowInc1 = (CX - BX);
	int RowInc2 = (AX - CX);

	int Bias0 = TestTopLeftRule(BX, BY, AX, AY) ? 0 : -1;
	int Bias1 = TestTopLeftRule(CX, CY, BX, BY) ? 0 : -1;
	int Bias2 = TestTopLeftRule(AX, AY, CX, CY) ? 0 : -1;

	int PX = MinX;
	int PY = MinY;

	int W0Row = TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0;
	int W1Row = TriOrientation(BX, BY, CX, CY, PX, PY) + Bias1;
	int W2Row = TriOrientation(AX, AY, PX, PY, CX, CY) + Bias2;



	unsigned *Row = Buffer->Data + PX + PY * Buffer->Width;
	for(int Y = MinY; Y <= MaxY; Y++){
		int W0 = W0Row;
		int W1 = W1Row;
		int W2 = W2Row;
		unsigned *Pixels = Row;
		for(int X = MinX; X <= MaxX; X++){
			if((W0 | W1 | W2) >= 0){
				*Pixels = 0xff969696;
			}
			
			else if((W0 | W1 | (~W2 + 1)) >= 0){
				*Pixels = 0xffD164FC;
			}
			
			else if((W0 | (~W1 + 1) | W2) >= 0){
				*Pixels = 0xff6575FC;
			}
			else if(((~W0 + 1) | W1 | W2) >= 0){
				*Pixels = 0xff65FCAC;
			} else if(((~W0 + 1) | (~W1 + 1) | W2) >= 0){
				*Pixels = 0xfffbfc65;
			} else if(((~W0 + 1) | W1 | (~W2 + 1)) >= 0){
				*Pixels = 0xffFC8965;
			} else if((W0 | (~W1 + 1) | (~W2 + 1)) >= 0){
				*Pixels = 0xff36F533;
			} else if(((~W0 + 1) | (~W1 + 1) | (~W2 + 1)) >= 0){
				*Pixels = 0xffF63434;
			}
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;
			Pixels++;
		}
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;
		Row += Buffer->Width;
	}
}
