#pragma once
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#include "Application.h"
#include "fxd.h"
#include "line.h"

//https://godbolt.org/z/verK5x8Gq

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

	internal inline fxd_48_16 fxd_TriOrientationD(
		fxd_24_8 AX, fxd_24_8 AY,
		fxd_24_8 BX, fxd_24_8 BY,
		fxd_24_8 CX, fxd_24_8 CY){
		fxd_48_16 tmp1 = FixedMul((BX - AX), (CY - AY));
		fxd_48_16 tmp2 = FixedMul((BY - AY), (CX - AX));
		return tmp1 - tmp2;
	}
	internal inline fxd_24_8 fxd_TriOrientationS(
		fxd_24_8 AX, fxd_24_8 AY,
		fxd_24_8 BX, fxd_24_8 BY,
		fxd_24_8 CX, fxd_24_8 CY){
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
		fxd_24_8 _AX, fxd_24_8 _AY, float _AttrA[3],
		fxd_24_8 _BX, fxd_24_8 _BY, float _AttrB[3],
		fxd_24_8 _CX, fxd_24_8 _CY, float _AttrC[3],
		unsigned Color){
		//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
		//Input is Y-Down Counter-Clockwise tri. But the math is for Y-Up coordinate systems
		//Change the orientation by swapping the order of the second and third vertex
		//Then process this as a Y-Up Counter-Clockwise try
		fxd_24_8 AX = _AX; fxd_24_8 AY = _AY;
		fxd_24_8 BX = _CX; fxd_24_8 BY = _CY;
		fxd_24_8 CX = _BX; fxd_24_8 CY = _BY;

		float *AttrA = _AttrA;
		float *AttrB = _AttrC;
		float *AttrC = _AttrB;

		fxd_24_8 Area = fxd_TriOrientationS(AX, AY, BX, BY, CX, CY);
		// Orientation < 0: Clockwise Orientation
		// Orientation == 0: Triangle with Area 0
		if(Area <= 0){
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

		fxd_24_8 W0Row = fxd_TriOrientationS(AX, AY, BX, BY, PX, PY);
		fxd_24_8 W1Row = fxd_TriOrientationS(BX, BY, CX, CY, PX, PY);
		fxd_24_8 W2Row = fxd_TriOrientationS(AX, AY, PX, PY, CX, CY);
		
		

		float tmp1 = FxdToFloat(W0Row);
		float tmp2 = FxdToFloat(Area);
		float tmp3 = tmp1 / tmp2;
		float AttrW0Row = tmp1 / tmp2;
		
		tmp1 = FxdToFloat(W1Row);
		tmp2 = FxdToFloat(Area);
		tmp3 = tmp1 / tmp2;
		float AttrW1Row = FxdToFloat(W1Row) / FxdToFloat(Area);

		tmp1 = FxdToFloat(W2Row);
		tmp2 = FxdToFloat(Area);
		tmp3 = tmp1 / tmp2;
		float AttrW2Row = FxdToFloat(W2Row) / FxdToFloat(Area);

		float AttrInc0 = FxdToFloat(Inc0) / FxdToFloat(Area);
		float AttrInc1 = FxdToFloat(Inc1) / FxdToFloat(Area);
		float AttrInc2 = FxdToFloat(Inc2) / FxdToFloat(Area);
		
		float AttrRowInc0 = FxdToFloat(RowInc0) / FxdToFloat(Area);
		float AttrRowInc1 = FxdToFloat(RowInc1) / FxdToFloat(Area);
		float AttrRowInc2 = FxdToFloat(RowInc2) / FxdToFloat(Area);

		//Add Bias after calculating weights for Attribute Interpolation
		W0Row += Bias0;
		W1Row += Bias1;
		W2Row += Bias2;

		unsigned *Row = Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX);
		for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
			fxd_24_8 W0 = W0Row;
			fxd_24_8 W1 = W1Row;
			fxd_24_8 W2 = W2Row;

			float AttrW0 = AttrW0Row;
			float AttrW1 = AttrW1Row;
			float AttrW2 = AttrW2Row;

			unsigned *Pixels = Row;
			for(fxd_24_8 X = MinX; X <= MaxX; X += FXD_ONE){
				unsigned Pixel = *Pixels;

				float AttrColorR = AttrA[0] * AttrW1 + AttrB[0] * AttrW2 + AttrC[0] * AttrW0;
				float AttrColorG = AttrA[1] * AttrW1 + AttrB[1] * AttrW2 + AttrC[1] * AttrW0;
				float AttrColorB = AttrA[2] * AttrW1 + AttrB[2] * AttrW2 + AttrC[2] * AttrW0;

				unsigned AttrColor = 0xff000000;
				AttrColor |= (unsigned)AttrColorR << 16;
				AttrColor |= (unsigned)AttrColorG <<  8;
				AttrColor |= (unsigned)AttrColorB <<  0;
				
				Pixel = ((W0 | W1 | W2) >= 0) ? AttrColor : Pixel;
				*Pixels = Pixel;
				W0 += Inc0;
				W1 += Inc1;
				W2 += Inc2;

				AttrW0 += AttrInc0;
				AttrW1 += AttrInc1;
				AttrW2 += AttrInc2;


				Pixels++;
			}
			Row += Buffer->Width;
			W0Row += RowInc0;
			W1Row += RowInc1;
			W2Row += RowInc2;

			AttrW0Row += AttrRowInc0;
			AttrW1Row += AttrRowInc1;
			AttrW2Row += AttrRowInc2;

		}
	}


	internal void fxd_RasterizeTri2(IScreenBuffer *Buffer,
		fxd_24_8 AX, fxd_24_8 AY,
		fxd_24_8 BX, fxd_24_8 BY,
		fxd_24_8 CX, fxd_24_8 CY,
		unsigned Color){
		//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

		int Orientation = fxd_TriOrientationS(AX, AY, BX, BY, CX, CY);
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


		fxd_24_8 W0Row = fxd_TriOrientationS(AX, AY, BX, BY, PX, PY) + Bias0;
		fxd_24_8 W1Row = fxd_TriOrientationS(BX, BY, CX, CY, PX, PY) + Bias1;
		fxd_24_8 W2Row = fxd_TriOrientationS(AX, AY, PX, PY, CX, CY) + Bias2;

		for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
			fxd_24_8 W0 = W0Row;
			fxd_24_8 W1 = W1Row;
			fxd_24_8 W2 = W2Row;
			for(fxd_24_8 X = MinX; X <= MaxX; X += FXD_ONE){
				//process Y-Down Counter-Clockwise tri as Y-Up clockwise triangle
				if(W0 <= 0 && W1 <= 0 && W2 <= 0){
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
		fxd_24_8 CX, fxd_24_8 CY){
		//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/

		int Orientation = fxd_TriOrientationS(AX, AY, BX, BY, CX, CY);
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


		fxd_24_8 W0Row = fxd_TriOrientationS(AX, AY, BX, BY, PX, PY) + Bias0;
		fxd_24_8 W1Row = fxd_TriOrientationS(BX, BY, CX, CY, PX, PY) + Bias1;
		fxd_24_8 W2Row = fxd_TriOrientationS(AX, AY, PX, PY, CX, CY) + Bias2;

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



	internal void fxd_RasterizeTriSSE(IScreenBuffer *Buffer,
		fxd_24_8 _AX, fxd_24_8 _AY,
		fxd_24_8 _BX, fxd_24_8 _BY,
		fxd_24_8 _CX, fxd_24_8 _CY,
		unsigned Color){
		//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
		//Input is Y-Down Counter-Clockwise tri. But the math is for Y-Up coordinate systems
		//Change the orientation by swapping the order of the second and third vertex
		//Then process this as a Y-Up Counter-Clockwise try
		fxd_24_8 AX = _AX; fxd_24_8 AY = _AY;
		fxd_24_8 BX = _CX; fxd_24_8 BY = _CY;
		fxd_24_8 CX = _BX; fxd_24_8 CY = _BY;

		int Orientation = fxd_TriOrientationS(AX, AY, BX, BY, CX, CY);
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

		__m128i W0Row = _mm_set1_epi32(fxd_TriOrientationS(AX, AY, BX, BY, PX, PY) + Bias0);
		__m128i W1Row = _mm_set1_epi32(fxd_TriOrientationS(BX, BY, CX, CY, PX, PY) + Bias1);
		__m128i W2Row = _mm_set1_epi32(fxd_TriOrientationS(AX, AY, PX, PY, CX, CY) + Bias2);

		__m128i W0RowBias = _mm_set_epi32(3 * Inc0, 2 * Inc0, Inc0, 0);
		__m128i W1RowBias = _mm_set_epi32(3 * Inc1, 2 * Inc1, Inc1, 0);
		__m128i W2RowBias = _mm_set_epi32(3 * Inc2, 2 * Inc2, Inc2, 0);

		W0Row = _mm_add_epi32(W0Row, W0RowBias);
		W1Row = _mm_add_epi32(W1Row, W1RowBias);
		W2Row = _mm_add_epi32(W2Row, W2RowBias);

		__m128i Inc0V = _mm_set1_epi32(4 * Inc0);
		__m128i Inc1V = _mm_set1_epi32(4 * Inc1);
		__m128i Inc2V = _mm_set1_epi32(4 * Inc2);

		__m128i RowInc0V = _mm_set1_epi32(RowInc0);
		__m128i RowInc1V = _mm_set1_epi32(RowInc1);
		__m128i RowInc2V = _mm_set1_epi32(RowInc2);


		unsigned *Row = (Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX));

		fxd_24_8 ModuloMask = ~(4 * FXD_ONE - 1);

		//might not be needed?
		__m128i ColorV = _mm_set1_epi32(Color);
		for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
			__m128i W0V = W0Row;
			__m128i W1V = W1Row;
			__m128i W2V = W2Row;
			__m128i *Pixels = (__m128i *)Row;
			fxd_24_8 X = MinX;
			for(; X < (((MaxX - MinX) & ModuloMask) + MinX); X += 4 * FXD_ONE){
				__m128i Mask = _mm_or_si128(W0V, W1V);
				Mask = _mm_or_si128(W2V, Mask);
				__m128i PixelV = _mm_lddqu_si128(Pixels);
				__m128i MaskLt = _mm_cmplt_epi32(Mask, _mm_setzero_si128());
				__m128i Tmp = _mm_blendv_epi8(ColorV, PixelV, MaskLt);
				_mm_storeu_si128(Pixels, Tmp);
				Pixels++;

				W0V = _mm_add_epi32(Inc0V, W0V);
				W1V = _mm_add_epi32(Inc1V, W1V);
				W2V = _mm_add_epi32(Inc2V, W2V);

			}

			unsigned *Pixel = (unsigned *)Pixels;
			fxd_24_8 W0 = _mm_cvtsi128_si32(W0V);
			fxd_24_8 W1 = _mm_cvtsi128_si32(W1V);
			fxd_24_8 W2 = _mm_cvtsi128_si32(W2V);
			while(X <= MaxX){

				unsigned tmp = *Pixel;
				tmp = ((W0 | W1 | W2) >= 0) ? Color : tmp;
				*Pixel++ = tmp;
				W0 += Inc0;
				W1 += Inc1;
				W2 += Inc2;

				X += FXD_ONE;
			}

			Row += Buffer->Width;
			W0Row = _mm_add_epi32(RowInc0V, W0Row);
			W1Row = _mm_add_epi32(RowInc1V, W1Row);
			W2Row = _mm_add_epi32(RowInc2V, W2Row);
		}
	}



	internal void fxd_RasterizeTriAVX(IScreenBuffer *Buffer,
		fxd_24_8 _AX, fxd_24_8 _AY,
		fxd_24_8 _BX, fxd_24_8 _BY,
		fxd_24_8 _CX, fxd_24_8 _CY,
		unsigned Color){
		//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
		//Input is Y-Down Counter-Clockwise tri. But the math is for Y-Up coordinate systems
		//Change the orientation by swapping the order of the second and third vertex
		//Then process this as a Y-Up Counter-Clockwise try
		fxd_24_8 AX = _AX; fxd_24_8 AY = _AY;
		fxd_24_8 BX = _CX; fxd_24_8 BY = _CY;
		fxd_24_8 CX = _BX; fxd_24_8 CY = _BY;

		int Orientation = fxd_TriOrientationS(AX, AY, BX, BY, CX, CY);
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

		__m256i W0Row = _mm256_set1_epi32(fxd_TriOrientationS(AX, AY, BX, BY, PX, PY) + Bias0);
		__m256i W1Row = _mm256_set1_epi32(fxd_TriOrientationS(BX, BY, CX, CY, PX, PY) + Bias1);
		__m256i W2Row = _mm256_set1_epi32(fxd_TriOrientationS(AX, AY, PX, PY, CX, CY) + Bias2);

		__m256i W0RowBias = _mm256_set_epi32(7 * Inc0, 6 * Inc0, 5 * Inc0, 4 * Inc0, 3 * Inc0, 2 * Inc0, 1 * Inc0, 0 * Inc0);
		__m256i W1RowBias = _mm256_set_epi32(7 * Inc1, 6 * Inc1, 5 * Inc1, 4 * Inc1, 3 * Inc1, 2 * Inc1, 1 * Inc1, 0 * Inc1);
		__m256i W2RowBias = _mm256_set_epi32(7 * Inc2, 6 * Inc2, 5 * Inc2, 4 * Inc2, 3 * Inc2, 2 * Inc2, 1 * Inc2, 0 * Inc2);

		W0Row = _mm256_add_epi32(W0Row, W0RowBias);
		W1Row = _mm256_add_epi32(W1Row, W1RowBias);
		W2Row = _mm256_add_epi32(W2Row, W2RowBias);

		__m256i Inc0V = _mm256_set1_epi32(8 * Inc0);
		__m256i Inc1V = _mm256_set1_epi32(8 * Inc1);
		__m256i Inc2V = _mm256_set1_epi32(8 * Inc2);

		__m256i RowInc0V = _mm256_set1_epi32(RowInc0);
		__m256i RowInc1V = _mm256_set1_epi32(RowInc1);
		__m256i RowInc2V = _mm256_set1_epi32(RowInc2);


		unsigned *Row = (Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX));


		fxd_24_8 ModuloMask = ~(8 * FXD_ONE - 1);
		__m256i ColorV = _mm256_set1_epi32(Color);


		for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
			__m256i W0V = W0Row;
			__m256i W1V = W1Row;
			__m256i W2V = W2Row;
			__m256i *Pixels = (__m256i *)Row;
			fxd_24_8 X = MinX;
			for(; X < (((MaxX - MinX) & ModuloMask) + MinX); X += 8 * FXD_ONE){
				__m256i Mask = _mm256_or_si256(W0V, W1V);
				Mask = _mm256_or_si256(W2V, Mask);
				__m256i MaskGt = _mm256_cmpgt_epi32(Mask, _mm256_setzero_si256());
				__m256i MaskEq = _mm256_cmpeq_epi32(Mask, _mm256_setzero_si256());
				__m256i MaskGe = _mm256_or_epi32(MaskEq, MaskGt);

				__m256i PixelV = _mm256_loadu_si256(Pixels);
				__m256i Tmp = _mm256_blendv_epi8(PixelV, ColorV, MaskGe);
				_mm256_storeu_si256(Pixels, Tmp);
				Pixels++;

				W0V = _mm256_add_epi32(Inc0V, W0V);
				W1V = _mm256_add_epi32(Inc1V, W1V);
				W2V = _mm256_add_epi32(Inc2V, W2V);
			}

			unsigned *Pixel = (unsigned *)Pixels;
			fxd_48_16 W0 = _mm256_cvtsi256_si32(W0V);
			fxd_48_16 W1 = _mm256_cvtsi256_si32(W1V);
			fxd_48_16 W2 = _mm256_cvtsi256_si32(W2V);
			while(X <= MaxX){
				unsigned tmp = *Pixel;
				tmp = ((W0 | W1 | W2) >= 0) ? Color : tmp;
				*Pixel++ = tmp;
				W0 += Inc0;
				W1 += Inc1;
				W2 += Inc2;

				X += FXD_ONE;
			}
			Row += Buffer->Width;
			W0Row = _mm256_add_epi32(RowInc0V, W0Row);
			W1Row = _mm256_add_epi32(RowInc1V, W1Row);
			W2Row = _mm256_add_epi32(RowInc2V, W2Row);
		}


	}
