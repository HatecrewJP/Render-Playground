#pragma once
#include "Application.h"
#include "fxd.h"
#include "line.h"

internal inline void fxd_SetPixel(IScreenBuffer *Buffer, fxd_24_8 X, fxd_24_8 Y, unsigned Color){
	//Assumes that the point is valid
	unsigned *Pixel = Buffer->Data + FxdToIntF(Y) * Buffer->Width + FxdToIntF(X);
	*Pixel = Color;
}

internal void fxd_RasterizeTriOutline(IScreenBuffer *Buffer,
	fxd_24_8 AX, fxd_24_8 AY,
	fxd_24_8 BX, fxd_24_8 BY,
	fxd_24_8 CX, fxd_24_8 CY,
	unsigned Color){
	fxd_BresenhamLine(Buffer, AX, AY, BX, BY, Color);
	fxd_BresenhamLine(Buffer, BX, BY, CX, CY, Color);
	fxd_BresenhamLine(Buffer, CX, CY, AX, AY, Color);
}

internal inline int fxd_TriOrientation(
	fxd_24_8 AX, fxd_24_8 AY,
	fxd_24_8 BX, fxd_24_8 BY,
	fxd_24_8 CX, fxd_24_8 CY)
{
	fxd_48_16 tmp1 = FixedMul((BX - AX), (CY - AY));
	fxd_48_16 tmp2 = FixedMul((BY - AY), (CX - AX));
	return (int)DToSFxd(tmp1 - tmp2);
}
internal inline int fxd_TriOrientation2(
	fxd_24_8 AX, fxd_24_8 AY,
	fxd_24_8 BX, fxd_24_8 BY,
	fxd_24_8 CX, fxd_24_8 CY)
{
	fxd_24_8 tmp1 = FixedMulS((BX - AX), (CY - AY));
	fxd_24_8 tmp2 = FixedMulS((BY - AY), (CX - AX));
	return tmp1 - tmp2;
}

internal bool fxd_TestTopLeftRule(fxd_24_8 AX, fxd_24_8 AY, fxd_24_8 BX, fxd_24_8 BY){
	bool Horizontal = (AY == BY);
	bool DirLeft = (BX - AX) < 0;
	bool TopEdge = Horizontal && DirLeft;
	bool DownEdge = (AY - BY) < 0;
	return TopEdge || DownEdge;
}


internal void fxd_RasterizeTri(IScreenBuffer *Buffer,
	fxd_24_8 _AX, fxd_24_8 _AY,
	fxd_24_8 _BX, fxd_24_8 _BY,
	fxd_24_8 _CX, fxd_24_8 _CY,
	unsigned Color)
{
	//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
	//Input is Y-Down Counter-Clockwise tri. But the math is for Y-Up coordinate systems
	//Change the orientation by swapping the order of the second and third vertex
	//Then process this as a Y-Up Counter-Clockwise try
	fxd_24_8 AX = _AX; fxd_24_8 AY = _AY;
	fxd_24_8 BX = _CX; fxd_24_8 BY = _CY;
	fxd_24_8 CX = _BX; fxd_24_8 CY = _BY;

	int Orientation = fxd_TriOrientation(AX, AY, BX, BY, CX, CY);
	// Orientation < 0: Clockwise Orientation
	// Orientation == 0: Triangle with Area 0
	if(Orientation <= 0){
		return;
	}

	fxd_24_8 MinX = fxd_Floor(fxd_Min3(AX, BX, CX)) + FXD_HALF;
	fxd_24_8 MinY = fxd_Floor(fxd_Min3(AY, BY, CY)) + FXD_HALF;
	fxd_24_8 MaxX = fxd_Floor(fxd_Max3(AX, BX, CX)) + FXD_HALF;
	fxd_24_8 MaxY = fxd_Floor(fxd_Max3(AY, BY, CY)) + FXD_HALF;

	if(MinX < FXD_HALF){
		MinX = FXD_HALF;
	}
	if(MinY < FXD_HALF){
		MinY = FXD_HALF;
	}
	if(MaxX >= IntToFxd(Buffer->Width) - FXD_HALF){
		MaxX = IntToFxd(Buffer->Width) - FXD_HALF;
	}
	if(MaxY >= IntToFxd(Buffer->Height) - FXD_HALF){
		MaxY = IntToFxd(Buffer->Height) - FXD_HALF;
	}
	fxd_24_8 Inc0 = (AY - BY);
	fxd_24_8 Inc1 = (BY - CY);
	fxd_24_8 Inc2 = (CY - AY);

	fxd_24_8 RowInc0 = (BX - AX);
	fxd_24_8 RowInc1 = (CX - BX);
	fxd_24_8 RowInc2 = (AX - CX);

	fxd_24_8 PX = MinX;
	fxd_24_8 PY = MinY;

	fxd_24_8 Bias0 = fxd_TestTopLeftRule(BX, BY, AX, AY) ? 0 : -FXD_FRAC_MIN;
	fxd_24_8 Bias1 = fxd_TestTopLeftRule(CX, CY, BX, BY) ? 0 : -FXD_FRAC_MIN;
	fxd_24_8 Bias2 = fxd_TestTopLeftRule(AX, AY, CX, CY) ? 0 : -FXD_FRAC_MIN;

	fxd_24_8 W0Row = fxd_TriOrientation2(AX, AY, BX, BY, PX, PY) + Bias0;
	fxd_24_8 W1Row = fxd_TriOrientation2(BX, BY, CX, CY, PX, PY) + Bias1;
	fxd_24_8 W2Row = fxd_TriOrientation2(AX, AY, PX, PY, CX, CY) + Bias2;

	unsigned *Row = Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX);
	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		fxd_24_8 W0 = W0Row;
		fxd_24_8 W1 = W1Row;
		fxd_24_8 W2 = W2Row;
		unsigned *Pixels = Row;
		for(fxd_24_8 X = MinX; X <= MaxX; X += FXD_ONE){
			*Pixels = ((W0 | W1 | W2) >= 0) ? Color : *Pixels;
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;
			Pixels++;
		}
		Row += Buffer->Width;
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;
	}
}


internal void fxd_RasterizeTri2(IScreenBuffer *Buffer,
	fxd_24_8 AX, fxd_24_8 AY,
	fxd_24_8 BX, fxd_24_8 BY,
	fxd_24_8 CX, fxd_24_8 CY,
	unsigned Color)
{
	//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

	int Orientation = fxd_TriOrientation(AX, AY, BX, BY, CX, CY);
	// Orientation < 0: Clockwise Orientation
	// Orientation == 0: Triangle with Area 0
	if(Orientation >= 0){
		return;
	}

	fxd_24_8 MinX = fxd_Floor(fxd_Min3(AX, BX, CX)) + FXD_HALF;
	fxd_24_8 MinY = fxd_Floor(fxd_Min3(AY, BY, CY)) + FXD_HALF;
	fxd_24_8 MaxX = fxd_Floor(fxd_Max3(AX, BX, CX)) + FXD_HALF;
	fxd_24_8 MaxY = fxd_Floor(fxd_Max3(AY, BY, CY)) + FXD_HALF;

	if(MinX < FXD_HALF){
		MinX = FXD_HALF;
	}
	if(MinY < FXD_HALF){
		MinY = FXD_HALF;
	}
	if(MaxX >= IntToFxd(Buffer->Width) - FXD_HALF){
		MaxX = IntToFxd(Buffer->Width) - FXD_HALF;
	}
	if(MaxY >= IntToFxd(Buffer->Height) - FXD_HALF){
		MaxY = IntToFxd(Buffer->Height) - FXD_HALF;
	}
	fxd_24_8 Inc0 = (AY - BY);
	fxd_24_8 Inc1 = (BY - CY);
	fxd_24_8 Inc2 = (CY - AY);

	fxd_24_8 RowInc0 = (BX - AX);
	fxd_24_8 RowInc1 = (CX - BX);
	fxd_24_8 RowInc2 = (AX - CX);

	fxd_24_8 PX = fxd_Floor(MinX) + FXD_HALF;
	fxd_24_8 PY = fxd_Floor(MinY) + FXD_HALF;

	fxd_24_8 Bias0 = fxd_TestTopLeftRule(AX, AY, BX, BY) ? 0 : FXD_FRAC_MIN;
	fxd_24_8 Bias1 = fxd_TestTopLeftRule(BX, BY, CX, CY) ? 0 : FXD_FRAC_MIN;
	fxd_24_8 Bias2 = fxd_TestTopLeftRule(CX, CY, AX, AY) ? 0 : FXD_FRAC_MIN;


	fxd_24_8 W0Row = fxd_TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0;
	fxd_24_8 W1Row = fxd_TriOrientation(BX, BY, CX, CY, PX, PY) + Bias1;
	fxd_24_8 W2Row = fxd_TriOrientation(AX, AY, PX, PY, CX, CY) + Bias2;

	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		fxd_24_8 W0 = W0Row;
		fxd_24_8 W1 = W1Row;
		fxd_24_8 W2 = W2Row;
		for(fxd_24_8 X = MinX; X <= MaxX; X += FXD_ONE){
			//process Y-Down Counter-Clockwise tri as Y-Up clockwise triangle
			if(W0 <= 0 &&  W1 <= 0 && W2 <= 0){
				fxd_SetPixel(Buffer, X, Y, Color);
			}
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;
		}
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;
	}
}

internal void fxd_TriHalfPlanes(IScreenBuffer *Buffer,
	fxd_24_8 AX, fxd_24_8 AY,
	fxd_24_8 BX, fxd_24_8 BY,
	fxd_24_8 CX, fxd_24_8 CY)
{
	//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

	int Orientation = fxd_TriOrientation(AX, AY, BX, BY, CX, CY);
	// Orientation < 0: Clockwise Orientation
	// Orientation == 0: Triangle with Area 0
	if(Orientation >= 0){
		return;
	}

	fxd_24_8 MinX = fxd_Floor(fxd_Min3(AX, BX, CX)) + FXD_HALF;
	fxd_24_8 MinY = fxd_Floor(fxd_Min3(AY, BY, CY)) + FXD_HALF;
	fxd_24_8 MaxX = fxd_Floor(fxd_Max3(AX, BX, CX)) + FXD_HALF;
	fxd_24_8 MaxY = fxd_Floor(fxd_Max3(AY, BY, CY)) + FXD_HALF;

	fxd_24_8 Padding = IntToFxd(150);
	MinX -= Padding;
	MinY -= Padding;
	MaxX += Padding;
	MaxY += Padding;


	if(MinX < FXD_HALF){
		MinX = FXD_HALF;
	}
	if(MinY < FXD_HALF){
		MinY = FXD_HALF;
	}
	if(MaxX >= IntToFxd(Buffer->Width) - FXD_HALF){
		MaxX = IntToFxd(Buffer->Width) - FXD_HALF;
	}
	if(MaxY >= IntToFxd(Buffer->Height) - FXD_HALF){
		MaxY = IntToFxd(Buffer->Height) - FXD_HALF;
	}
	fxd_24_8 Inc0 = (AY - BY);
	fxd_24_8 Inc1 = (BY - CY);
	fxd_24_8 Inc2 = (CY - AY);

	fxd_24_8 RowInc0 = (BX - AX);
	fxd_24_8 RowInc1 = (CX - BX);
	fxd_24_8 RowInc2 = (AX - CX);

	fxd_24_8 PX = fxd_Floor(MinX) + FXD_HALF;
	fxd_24_8 PY = fxd_Floor(MinY) + FXD_HALF;

	fxd_24_8 Bias0 = fxd_TestTopLeftRule(AX, AY, BX, BY) ? 0 : FXD_FRAC_MIN;
	fxd_24_8 Bias1 = fxd_TestTopLeftRule(BX, BY, CX, CY) ? 0 : FXD_FRAC_MIN;
	fxd_24_8 Bias2 = fxd_TestTopLeftRule(CX, CY, AX, AY) ? 0 : FXD_FRAC_MIN;


	fxd_24_8 W0Row = fxd_TriOrientation(AX, AY, BX, BY, PX, PY) + Bias0;
	fxd_24_8 W1Row = fxd_TriOrientation(BX, BY, CX, CY, PX, PY) + Bias1;
	fxd_24_8 W2Row = fxd_TriOrientation(AX, AY, PX, PY, CX, CY) + Bias2;

	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		fxd_24_8 W0 = W0Row;
		fxd_24_8 W1 = W1Row;
		fxd_24_8 W2 = W2Row;
		for(fxd_24_8 X = MinX; X <= MaxX; X += FXD_ONE){
			//process Y-Down Counter-Clockwise tri as Y-Up clockwise triangle
			if((W0 | W1 | W2) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xff969696);
			} else if((W0 | W1 | (~W2 + 1)) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xffD164FC);
			} else if((W0 | (~W1 + 1) | W2) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xff6575FC);
			} else if(((~W0 + 1) | W1 | W2) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xff65FCAC);
			} else if(((~W0 + 1) | (~W1 + 1) | W2) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xfffbfc65);
			} else if(((~W0 + 1) | W1 | (~W2 + 1)) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xffFC8965);
			} else if((W0 | (~W1 + 1) | (~W2 + 1)) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xff36F533);
			} else if(((~W0 + 1) | (~W1 + 1) | (~W2 + 1)) >= 0){
				fxd_SetPixel(Buffer, X, Y, 0xffF63434);
			}
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;
		}
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;
	}
}