#pragma once
#include "Application.h"
#include "rpg_math.h"



internal inline int TriOrientation(
	int AX, int AY,
	int BX, int BY,
	int CX, int CY){
	//Y-Down coordinate System
	return ((BY - AY) * (CX - AX)) - ((BX - AX) * (CY - AY));
}

internal int TestTopLeftRule(int AX, int AY, int BX, int BY){
	int Horizontal = AY == BY;
	int DirLeft = (BX - AX) < 0;
	int TopEdge = Horizontal && DirLeft;
	int DownEdge = (AY - BY) < 0;
	return TopEdge || DownEdge;
}

internal void RasterizeTri(IScreenBuffer *Buffer,
	int AX, int AY,
	int BX, int BY,
	int CX, int CY,
	unsigned Color){
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
			int PX = X;
			int PY = Y;
			InsideTri[0] = (TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0) >= 0;
			InsideTri[1] = (TriOrientation(AX, AY, PX, PY, CX, CY) + Bias1) >= 0;
			InsideTri[2] = (TriOrientation(BX, BY, CX, CY, PX, PY) + Bias2) >= 0;

			if(InsideTri[0] && InsideTri[1] && InsideTri[2]){
				*Pixels = Color;

			}
			Pixels++;

		}
		Pixels += Buffer->Width - (MaxX - MinX);
	}
}
internal void IncRasterizeTri(IScreenBuffer *Buffer,
	int AX, int AY,
	int BX, int BY,
	int CX, int CY,
	unsigned Color){
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
	if(MaxX > Buffer->Width){
		MaxX = Buffer->Width;
	}
	if(MaxY > Buffer->Height){
		MaxY = Buffer->Height;
	}


	int RowInc0 = (AX - BX);
	int RowInc1 = (BX - CX);
	int RowInc2 = (CX - AX);

	int Inc0 = (BY - AY);
	int Inc1 = (CY - BY);
	int Inc2 = (AY - CY);

	int Bias0 = TestTopLeftRule(AX, AY, BX, BY) ? 0 : -Inc0;
	int Bias1 = TestTopLeftRule(BX, BY, CX, CY) ? 0 : -Inc1;
	int Bias2 = TestTopLeftRule(CX, CY, AX, AY) ? 0 : -Inc2;

	int PX = MinX;
	int PY = MinY;

	int W0Row = TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0;
	int W1Row = TriOrientation(BX, BY, CX, CY, PX, PY) + Bias1;
	int W2Row = TriOrientation(AX, AY, PX, PY, CX, CY) + Bias2;



	unsigned *Pixels = Buffer->Data + MinX + MinY * Buffer->Width;
	for(int Y = MinY; Y < MaxY; Y++){
		int W0 = W0Row;
		int W1 = W1Row;
		int W2 = W2Row;

		for(int X = MinX; X < MaxX; X++){
#if 1
			if((W0 | W1 | W2) >= 0){
				*Pixels = Color;
			}
#endif
#if 0
			if(W0 >= 0){
				*Pixels = Color;
			}
#endif
#if 0
			if(W1 >= 0){
				*Pixels = Color;
			}
#endif
#if 0
			if(W2 >= 0){
				*Pixels = Color;
			}
#endif


			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;

			Pixels++;

		}
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;
		Pixels += Buffer->Width - (MaxX - MinX);
	}
}

internal void TriHalfSpaces(IScreenBuffer *Buffer,
	int AX, int AY,
	int BX, int BY,
	int CX, int CY,
	unsigned Color){
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

	int Padding = 150;

	//MinY -= Padding;
	//MaxY += Padding;
	//MinX -= Padding;
	//MaxX += Padding;


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
	
	
	int RowInc0 = (AX - BX);
	int RowInc1 = (BX - CX);
	int RowInc2 = (CX - AX);

	int Inc0 = (BY - AY);
	int Inc1 = (CY - BY);
	int Inc2 = (AY - CY);

	int Bias0 = TestTopLeftRule(AX, AY, BX, BY) ? 0 : -Inc0;
	int Bias1 = TestTopLeftRule(BX, BY, CX, CY) ? 0 : -Inc1;
	int Bias2 = TestTopLeftRule(CX, CY, AX, AY) ? 0 : -Inc2;

	int PX = MinX;
	int PY = MinY;

	int W0Row = TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0;
	int W1Row = TriOrientation(BX, BY, CX, CY, PX, PY) + Bias1;
	int W2Row = TriOrientation(AX, AY, PX, PY, CX, CY) + Bias2;

	

	unsigned *Pixels = Buffer->Data + MinX + MinY * Buffer->Width;

	for(int Y = MinY; Y < MaxY; Y++){
		int W0 = W0Row;
		int W1 = W1Row;
		int W2 = W2Row;

		for(int X = MinX; X < MaxX; X++){
#if 0
			if(Abs32(W0) <= Abs32(Inc0)  || Abs32(W1) <= Abs32(Inc1) || Abs32(W2) <= Abs32(Inc2)){
				*Pixels = 0xff000000;
			}
			else if(Abs32(W0) <= Abs32(RowInc0) || Abs32(W1) <= Abs32(RowInc1) || Abs32(W2) <= Abs32(RowInc2)){
				*Pixels = 0xff000000;
			}
			else
#endif


			if(Abs32(W0) <= Abs32(Inc0)){
				*Pixels = 0xff000000;
			} else if(Abs32(W0) <= Abs32(RowInc0)){
				*Pixels = 0xff000000;
			} else
			if((W0 | W1 | W2) >= 0){
				*Pixels = 0xff969696;
			}

			else if((W0 | W1 | (~W2+1))>= 0){
				*Pixels = 0xffD164FC;
			}

			else if((W0 | (~W1+1) | W2) >= 0){
				*Pixels = 0xff6575FC;
			}

			else if(((~W0+1) | W1 | W2) >= 0){
				*Pixels = 0xff65FCAC;
			}
			else if(((~W0 + 1) | (~W1+1) | W2) >= 0){
				*Pixels = 0xfffbfc65;
			}
			else if(((~W0+1)| W1 | (~W2+1)) >= 0){
				*Pixels = 0xffFC8965;
			}
			else if((W0 | (~W1+1) | (~W2 + 1)) >= 0){
				*Pixels = 0xff36F533;
			}
			else if(((~W0 + 1) | (~W1 + 1) | (~W2+1)) >= 0){
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
		Pixels += Buffer->Width - (MaxX - MinX);
	}
}