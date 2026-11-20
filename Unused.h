#pragma once
#include "fxd_tri.h"
#include "memory.h"


//Hot loop code gen sucks
internal void fxd_RasterizeTriAVXD(IScreenBuffer *Buffer,
	fxd_24_8 _AX, fxd_24_8 _AY,
	fxd_24_8 _BX, fxd_24_8 _BY,
	fxd_24_8 _CX, fxd_24_8 _CY,
	unsigned Color){
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
	fxd_48_16 Inc0 = (fxd_48_16)(AY - BY) << FRACTION_BITS;
	fxd_48_16 Inc1 = (fxd_48_16)(BY - CY) << FRACTION_BITS;
	fxd_48_16 Inc2 = (fxd_48_16)(CY - AY) << FRACTION_BITS;

	fxd_48_16 RowInc0 = (fxd_48_16)(BX - AX) << FRACTION_BITS;
	fxd_48_16 RowInc1 = (fxd_48_16)(CX - BX) << FRACTION_BITS;
	fxd_48_16 RowInc2 = (fxd_48_16)(AX - CX) << FRACTION_BITS;

	fxd_24_8 PX = MinX;
	fxd_24_8 PY = MinY;

	fxd_48_16 Bias0 = fxd_TestTopLeftRule(BX, BY, AX, AY) ? 0 : -FXD_FRAC_MIN;
	fxd_48_16 Bias1 = fxd_TestTopLeftRule(CX, CY, BX, BY) ? 0 : -FXD_FRAC_MIN;
	fxd_48_16 Bias2 = fxd_TestTopLeftRule(AX, AY, CX, CY) ? 0 : -FXD_FRAC_MIN;

	__m256i W0Row = _mm256_set1_epi64x(fxd_TriOrientationD(AX, AY, BX, BY, PX, PY) + Bias0);
	__m256i W1Row = _mm256_set1_epi64x(fxd_TriOrientationD(BX, BY, CX, CY, PX, PY) + Bias1);
	__m256i W2Row = _mm256_set1_epi64x(fxd_TriOrientationD(AX, AY, PX, PY, CX, CY) + Bias2);

	__m256i W0RowBias = _mm256_set_epi64x(3 * Inc0, 2 * Inc0, 1 * Inc0, 0 * Inc0);
	__m256i W1RowBias = _mm256_set_epi64x(3 * Inc1, 2 * Inc1, 1 * Inc1, 0 * Inc1);
	__m256i W2RowBias = _mm256_set_epi64x(3 * Inc2, 2 * Inc2, 1 * Inc2, 0 * Inc2);

	W0Row = _mm256_add_epi64(W0Row, W0RowBias);
	W1Row = _mm256_add_epi64(W1Row, W1RowBias);
	W2Row = _mm256_add_epi64(W2Row, W2RowBias);

	__m256i Inc0V = _mm256_set1_epi64x(4 * Inc0);
	__m256i Inc1V = _mm256_set1_epi64x(4 * Inc1);
	__m256i Inc2V = _mm256_set1_epi64x(4 * Inc2);

	__m256i RowInc0V = _mm256_set1_epi64x(RowInc0);
	__m256i RowInc1V = _mm256_set1_epi64x(RowInc1);
	__m256i RowInc2V = _mm256_set1_epi64x(RowInc2);


	unsigned *Row = (Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX));

	fxd_24_8 ModuloMask = ~(4 * FXD_ONE - 1);
	//might not be needed?
	int iColor = *(int *)&Color;
	__m256i ColorV = _mm256_set1_epi32(iColor);
	__m256i ShuffleMask = _mm256_set_epi32(6, 4, 2, 0, 7, 5, 3, 1);

	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		__m256i W0V = W0Row;
		__m256i W1V = W1Row;
		__m256i W2V = W2Row;
		unsigned *Pixels = Row;
		fxd_24_8 X = MinX;
		for(; X < (((MaxX - MinX) & ModuloMask) + MinX); X += 4 * FXD_ONE){
			__m256i Mask = _mm256_or_si256(W0V, W1V);
			Mask = _mm256_or_si256(W2V, Mask);

			__m256i MaskGt = _mm256_cmpgt_epi64(Mask, _mm256_setzero_si256());
			__m256i MaskEq = _mm256_cmpeq_epi64(Mask, _mm256_setzero_si256());
			__m256i MaskGe = _mm256_or_si256(MaskEq, MaskGt);
			MaskGe = _mm256_permutevar8x32_epi32(MaskGe, ShuffleMask);

			/* Generates 4 unnecessary instructions
				vmovq	rcx, xmm3
				vmovq	xmm1, rcx
				vpextrq	rcx, xmm3, 1
				vpinsrq	xmm2, xmm1, rcx, 1
				
				same can be achieved by using __m128i. 
			
			*/
			MaskGe = _mm256_set_epi64x(0, 0, MaskGe.m256i_i64[1], MaskGe.m256i_i64[0]);
			__m256i PixelV = _mm256_loadu_si256((__m256i *)Pixels);
			PixelV = _mm256_blendv_epi8(PixelV, ColorV, MaskGe);
			_mm256_storeu_si256((__m256i *)Pixels, PixelV);


			Pixels += 4;

			W0V = _mm256_add_epi64(Inc0V, W0V);
			W1V = _mm256_add_epi64(Inc1V, W1V);
			W2V = _mm256_add_epi64(Inc2V, W2V);

		}
		unsigned *Pixel = (unsigned *)Pixels;
		fxd_48_16 W0 = _mm256_cvtsi256_si64(W0V);
		fxd_48_16 W1 = _mm256_cvtsi256_si64(W1V);
		fxd_48_16 W2 = _mm256_cvtsi256_si64(W2V);
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
		W0Row = _mm256_add_epi64(RowInc0V, W0Row);
		W1Row = _mm256_add_epi64(RowInc1V, W1Row);
		W2Row = _mm256_add_epi64(RowInc2V, W2Row);
	}
}



 


internal void fxd_RasterizeTriAVXD(IScreenBuffer *Buffer,
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
	fxd_48_16 Inc0 = (fxd_48_16)(AY - BY) << FRACTION_BITS;
	fxd_48_16 Inc1 = (fxd_48_16)(BY - CY) << FRACTION_BITS;
	fxd_48_16 Inc2 = (fxd_48_16)(CY - AY) << FRACTION_BITS;

	fxd_48_16 RowInc0 = (fxd_48_16)(BX - AX) << FRACTION_BITS;
	fxd_48_16 RowInc1 = (fxd_48_16)(CX - BX) << FRACTION_BITS;
	fxd_48_16 RowInc2 = (fxd_48_16)(AX - CX) << FRACTION_BITS;

	fxd_24_8 PX = MinX;
	fxd_24_8 PY = MinY;

	fxd_48_16 Bias0 = fxd_TestTopLeftRule(BX, BY, AX, AY) ? 0 : -FXD_FRAC_MIN;
	fxd_48_16 Bias1 = fxd_TestTopLeftRule(CX, CY, BX, BY) ? 0 : -FXD_FRAC_MIN;
	fxd_48_16 Bias2 = fxd_TestTopLeftRule(AX, AY, CX, CY) ? 0 : -FXD_FRAC_MIN;

	__m256i W0Row = _mm256_set1_epi64x(fxd_TriOrientationD(AX, AY, BX, BY, PX, PY) + Bias0);
	__m256i W1Row = _mm256_set1_epi64x(fxd_TriOrientationD(BX, BY, CX, CY, PX, PY) + Bias1);
	__m256i W2Row = _mm256_set1_epi64x(fxd_TriOrientationD(AX, AY, PX, PY, CX, CY) + Bias2);

	__m256i W0RowBias = _mm256_set_epi64x(3 * Inc0, 2 * Inc0, 1 * Inc0, 0 * Inc0);
	__m256i W1RowBias = _mm256_set_epi64x(3 * Inc1, 2 * Inc1, 1 * Inc1, 0 * Inc1);
	__m256i W2RowBias = _mm256_set_epi64x(3 * Inc2, 2 * Inc2, 1 * Inc2, 0 * Inc2);

	W0Row = _mm256_add_epi64(W0Row, W0RowBias);
	W1Row = _mm256_add_epi64(W1Row, W1RowBias);
	W2Row = _mm256_add_epi64(W2Row, W2RowBias);

	__m256i Inc0V = _mm256_set1_epi64x(4 * Inc0);
	__m256i Inc1V = _mm256_set1_epi64x(4 * Inc1);
	__m256i Inc2V = _mm256_set1_epi64x(4 * Inc2);

	__m256i RowInc0V = _mm256_set1_epi64x(RowInc0);
	__m256i RowInc1V = _mm256_set1_epi64x(RowInc1);
	__m256i RowInc2V = _mm256_set1_epi64x(RowInc2);


	unsigned *Row = (Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX));

	fxd_24_8 ModuloMask = ~(4 * FXD_ONE - 1);
	//might not be needed?
	int iColor = *(int *)&Color;
	__m128i ColorV = _mm_set1_epi32(iColor);
	__m256i ShuffleMask = _mm256_set_epi32(6, 4, 2, 0, 7, 5, 3, 1);

	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		__m256i W0V = W0Row;
		__m256i W1V = W1Row;
		__m256i W2V = W2Row;
		__m128i *Pixels = (__m128i *)Row;
		fxd_24_8 X = MinX;
		for(; X < (((MaxX - MinX) & ModuloMask) + MinX); X += 4 * FXD_ONE){
			__m256i WMask = _mm256_or_si256(W0V, W1V);
			WMask = _mm256_or_si256(W2V, WMask);

			__m256i MaskGt = _mm256_cmpgt_epi64(WMask, _mm256_setzero_si256());
			__m256i MaskEq = _mm256_cmpeq_epi64(WMask, _mm256_setzero_si256());
			__m256i MaskGe = _mm256_or_si256(MaskEq, MaskGt);

			MaskGe = _mm256_permutevar8x32_epi32(MaskGe, ShuffleMask);
			__m128i PixelMask = _mm256_extracti128_si256(MaskGe, 0);

			__m128i PixelV = _mm_loadu_si128(Pixels);
			PixelV = _mm_blendv_epi8(PixelV, ColorV, PixelMask);
			_mm_storeu_si128(Pixels, PixelV);
			Pixels++;

			W0V = _mm256_add_epi64(Inc0V, W0V);
			W1V = _mm256_add_epi64(Inc1V, W1V);
			W2V = _mm256_add_epi64(Inc2V, W2V);
		}
		unsigned *Pixel = (unsigned *)Pixels;
		fxd_48_16 W0 = _mm256_cvtsi256_si64(W0V);
		fxd_48_16 W1 = _mm256_cvtsi256_si64(W1V);
		fxd_48_16 W2 = _mm256_cvtsi256_si64(W2V);
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
		W0Row = _mm256_add_epi64(RowInc0V, W0Row);
		W1Row = _mm256_add_epi64(RowInc1V, W1Row);
		W2Row = _mm256_add_epi64(RowInc2V, W2Row);
	}
}

//Rasterizes with math for Counterclockwise triangle. Might be interesting for AVX, since avx has not lt comparison
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


//The idea was to get the distance to the tri and advance before entering the inner loop
//Worse performance than the normal version. Might be possible if steps are calculated as powers of two to avoid Mul/Div
internal void fxd_RasterizeTri2(IScreenBuffer *Buffer,
	fxd_24_8 _AX, fxd_24_8 _AY, float _AttrA[3],
	fxd_24_8 _BX, fxd_24_8 _BY, float _AttrB[3],
	fxd_24_8 _CX, fxd_24_8 _CY, float _AttrC[3]){
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

	//Setup Position Interpolation
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

	//Setup Attribute Interpolation
	float AttrW0Row = FxdToFloat(W0Row) / FxdToFloat(Area);
	float AttrW1Row = FxdToFloat(W1Row) / FxdToFloat(Area);
	float AttrW2Row = FxdToFloat(W2Row) / FxdToFloat(Area);

	float AttrInc0 = FxdToFloat(Inc0) / FxdToFloat(Area);
	float AttrInc1 = FxdToFloat(Inc1) / FxdToFloat(Area);
	float AttrInc2 = FxdToFloat(Inc2) / FxdToFloat(Area);

	float AttrRowInc0 = FxdToFloat(RowInc0) / FxdToFloat(Area);
	float AttrRowInc1 = FxdToFloat(RowInc1) / FxdToFloat(Area);
	float AttrRowInc2 = FxdToFloat(RowInc2) / FxdToFloat(Area);

	float AttrWARow[3] = {0}, AttrWBRow[3] = {0}, AttrWCRow[3] = {0};
	AttrWARow[0] = AttrA[0] * AttrW1Row;
	AttrWARow[1] = AttrA[1] * AttrW1Row;
	AttrWARow[2] = AttrA[2] * AttrW1Row;
	AttrWBRow[0] = AttrB[0] * AttrW2Row;
	AttrWBRow[1] = AttrB[1] * AttrW2Row;
	AttrWBRow[2] = AttrB[2] * AttrW2Row;
	AttrWCRow[0] = AttrC[0] * AttrW0Row;
	AttrWCRow[1] = AttrC[1] * AttrW0Row;
	AttrWCRow[2] = AttrC[2] * AttrW0Row;

	float AttrAInc[3], AttrBInc[3], AttrCInc[3];
	AttrAInc[0] = AttrA[0] * AttrInc1;
	AttrAInc[1] = AttrA[1] * AttrInc1;
	AttrAInc[2] = AttrA[2] * AttrInc1;
	AttrBInc[0] = AttrB[0] * AttrInc2;
	AttrBInc[1] = AttrB[1] * AttrInc2;
	AttrBInc[2] = AttrB[2] * AttrInc2;
	AttrCInc[0] = AttrC[0] * AttrInc0;
	AttrCInc[1] = AttrC[1] * AttrInc0;
	AttrCInc[2] = AttrC[2] * AttrInc0;

	float AttrARowInc[3], AttrBRowInc[3], AttrCRowInc[3];
	AttrARowInc[0] = AttrA[0] * AttrRowInc1;
	AttrARowInc[1] = AttrA[1] * AttrRowInc1;
	AttrARowInc[2] = AttrA[2] * AttrRowInc1;
	AttrBRowInc[0] = AttrB[0] * AttrRowInc2;
	AttrBRowInc[1] = AttrB[1] * AttrRowInc2;
	AttrBRowInc[2] = AttrB[2] * AttrRowInc2;
	AttrCRowInc[0] = AttrC[0] * AttrRowInc0;
	AttrCRowInc[1] = AttrC[1] * AttrRowInc0;
	AttrCRowInc[2] = AttrC[2] * AttrRowInc0;

	//Add Bias after calculating weights for correct Attribute Interpolation
	W0Row += Bias0;
	W1Row += Bias1;
	W2Row += Bias2;

	unsigned *Row = Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX);
	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		unsigned *Pixels = Row;
		fxd_24_8 X = MinX;

		fxd_24_8 W0 = W0Row;
		fxd_24_8 W1 = W1Row;
		fxd_24_8 W2 = W2Row;

		//Calculate how many steps to take
		fxd_24_8 StepsW0 = FixedDivS(W0, NegateFxd(Inc0));
		fxd_24_8 StepsW1 = FixedDivS(W1, NegateFxd(Inc1));
		fxd_24_8 StepsW2 = FixedDivS(W2, NegateFxd(Inc2));

		fxd_24_8 Steps = 0;
		Steps = (StepsW0 >= 0 && StepsW0 <= Steps) ? StepsW0 : Steps;
		Steps = (StepsW1 >= 0 && StepsW1 <= Steps) ? StepsW1 : Steps;
		Steps = (StepsW2 >= 0 && StepsW2 <= Steps) ? StepsW2 : Steps;

		Steps = fxd_Floor(Steps);

		//Advance by 'Steps'
		W0 = FixedMulS(StepsW0, Steps) + W0;
		W1 = FixedMulS(StepsW1, Steps) + W1;
		W2 = FixedMulS(StepsW2, Steps) + W2;

		AttrWARow[0] += AttrAInc[0] * FxdToFloat(Steps);
		AttrWARow[1] += AttrAInc[1] * FxdToFloat(Steps);
		AttrWARow[2] += AttrAInc[2] * FxdToFloat(Steps);

		AttrWBRow[0] += AttrBInc[0] * FxdToFloat(Steps);
		AttrWBRow[1] += AttrBInc[1] * FxdToFloat(Steps);
		AttrWBRow[2] += AttrBInc[2] * FxdToFloat(Steps);

		AttrWCRow[0] += AttrCInc[0] * FxdToFloat(Steps);
		AttrWCRow[1] += AttrCInc[1] * FxdToFloat(Steps);
		AttrWCRow[2] += AttrCInc[2] * FxdToFloat(Steps);

		Pixels += FxdToIntF(Steps);
		X += Steps;

		float AttrWA[3];
		float AttrWB[3];
		float AttrWC[3];

		memcpy(AttrWA, AttrWARow, sizeof(*AttrWA) * 3);
		memcpy(AttrWB, AttrWBRow, sizeof(*AttrWB) * 3);
		memcpy(AttrWC, AttrWCRow, sizeof(*AttrWC) * 3);


		for(; X <= MaxX; X += FXD_ONE){
			unsigned Pixel = *Pixels;

			float fColor[3] = {0};
			fColor[0] = AttrWA[0] + AttrWB[0] + AttrWC[0];
			fColor[1] = AttrWA[1] + AttrWB[1] + AttrWC[1];
			fColor[2] = AttrWA[2] + AttrWB[2] + AttrWC[2];

			unsigned AttrColor = Float3ToARGB(fColor);

			Pixel = ((W0 | W1 | W2) >= 0) ? AttrColor : Pixel;
			*Pixels = Pixel;
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;

			AttrWA[0] += AttrAInc[0];
			AttrWA[1] += AttrAInc[1];
			AttrWA[2] += AttrAInc[2];
			AttrWB[0] += AttrBInc[0];
			AttrWB[1] += AttrBInc[1];
			AttrWB[2] += AttrBInc[2];
			AttrWC[0] += AttrCInc[0];
			AttrWC[1] += AttrCInc[1];
			AttrWC[2] += AttrCInc[2];


			Pixels++;
		}
		Row += Buffer->Width;
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;


		AttrWARow[0] += AttrARowInc[0];
		AttrWARow[1] += AttrARowInc[1];
		AttrWARow[2] += AttrARowInc[2];
		AttrWBRow[0] += AttrBRowInc[0];
		AttrWBRow[1] += AttrBRowInc[1];
		AttrWBRow[2] += AttrBRowInc[2];
		AttrWCRow[0] += AttrCRowInc[0];
		AttrWCRow[1] += AttrCRowInc[1];
		AttrWCRow[2] += AttrCRowInc[2];

	}
}

//Bad Performance
internal void fxd_RasterizeTri2(IScreenBuffer *Buffer,
	fxd_24_8 _AX, fxd_24_8 _AY, float *_AttrA,
	fxd_24_8 _BX, fxd_24_8 _BY, float *_AttrB,
	fxd_24_8 _CX, fxd_24_8 _CY, float *_AttrC,
	int AttributeCount,
	struct Stack *Stack){
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

	//Setup Position Interpolation
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

	//Setup Attribute Interpolation
	float AttrW0Row = FxdToFloat(W0Row) / FxdToFloat(Area);
	float AttrW1Row = FxdToFloat(W1Row) / FxdToFloat(Area);
	float AttrW2Row = FxdToFloat(W2Row) / FxdToFloat(Area);

	float AttrInc0 = FxdToFloat(Inc0) / FxdToFloat(Area);
	float AttrInc1 = FxdToFloat(Inc1) / FxdToFloat(Area);
	float AttrInc2 = FxdToFloat(Inc2) / FxdToFloat(Area);

	float AttrRowInc0 = FxdToFloat(RowInc0) / FxdToFloat(Area);
	float AttrRowInc1 = FxdToFloat(RowInc1) / FxdToFloat(Area);
	float AttrRowInc2 = FxdToFloat(RowInc2) / FxdToFloat(Area);

	float *AttrWARow = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrWBRow = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrWCRow = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	for(int i = 0; i < AttributeCount; i++){
		AttrWARow[i] = AttrA[i] * AttrW1Row;
		AttrWBRow[i] = AttrB[i] * AttrW2Row;
		AttrWCRow[i] = AttrC[i] * AttrW0Row;
	}
	float *AttrAInc = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrBInc = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrCInc = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	for(int i = 0; i < AttributeCount; i++){
		AttrAInc[i] = AttrA[i] * AttrInc1;
		AttrBInc[i] = AttrB[i] * AttrInc2;
		AttrCInc[i] = AttrC[i] * AttrInc0;
	}

	float *AttrARowInc = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrBRowInc = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrCRowInc = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	for(int i = 0; i < AttributeCount; i++){
		AttrARowInc[i] = AttrA[i] * AttrInc1;
		AttrBRowInc[i] = AttrB[i] * AttrInc2;
		AttrCRowInc[i] = AttrC[i] * AttrInc0;
	}


	float *AttrWA = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrWB = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);
	float *AttrWC = (float *)StackAllocArray(Stack, sizeof(*AttrA), AttributeCount);

	//Add Bias after calculating weights for correct Attribute Interpolation
	W0Row += Bias0;
	W1Row += Bias1;
	W2Row += Bias2;

	unsigned *Row = Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX);
	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		fxd_24_8 W0 = W0Row;
		fxd_24_8 W1 = W1Row;
		fxd_24_8 W2 = W2Row;

		memcpy(AttrWA, AttrWARow, sizeof(*AttrWA) * AttributeCount);
		memcpy(AttrWB, AttrWBRow, sizeof(*AttrWB) * AttributeCount);
		memcpy(AttrWC, AttrWCRow, sizeof(*AttrWC) * AttributeCount);

		unsigned *Pixels = Row;
		for(fxd_24_8 X = MinX; X <= MaxX; X += FXD_ONE){
			unsigned Pixel = *Pixels;

			float fColor[3] = {0};
			fColor[0] = AttrWA[0] + AttrWB[0] + AttrWC[0];
			fColor[1] = AttrWA[1] + AttrWB[1] + AttrWC[1];
			fColor[2] = AttrWA[2] + AttrWB[2] + AttrWC[2];

			unsigned AttrColor = Float3ToARGB(fColor);

			Pixel = ((W0 | W1 | W2) >= 0) ? AttrColor : Pixel;
			*Pixels = Pixel;
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;

			for(int i = 0; i < AttributeCount; i++){
				AttrWA[i] += AttrAInc[i];
				AttrWB[i] += AttrBInc[i];
				AttrWC[i] += AttrCInc[i];
			}

			Pixels++;
		}
		Row += Buffer->Width;
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;

		for(int i = 0; i < AttributeCount; i++){
			AttrWARow[i] += AttrARowInc[i];
			AttrWBRow[i] += AttrBRowInc[i];
			AttrWCRow[i] += AttrCRowInc[i];
		}
	}
}

//Three channel Attribute Interpolation
internal void fxd_RasterizeTriAVX2(IScreenBuffer *Buffer,
	fxd_24_8 _AX, fxd_24_8 _AY, float _AttrA[3],
	fxd_24_8 _BX, fxd_24_8 _BY, float _AttrB[3],
	fxd_24_8 _CX, fxd_24_8 _CY, float _AttrC[3]){

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

	__m256 AttrAV[3], AttrBV[3], AttrCV[3];
	AttrAV[0] = _mm256_set1_ps(AttrA[0]);
	AttrBV[0] = _mm256_set1_ps(AttrB[0]);
	AttrCV[0] = _mm256_set1_ps(AttrC[0]);

	AttrAV[1] = _mm256_set1_ps(AttrA[1]);
	AttrBV[1] = _mm256_set1_ps(AttrB[1]);
	AttrCV[1] = _mm256_set1_ps(AttrC[1]);

	AttrAV[2] = _mm256_set1_ps(AttrA[2]);
	AttrBV[2] = _mm256_set1_ps(AttrB[2]);
	AttrCV[2] = _mm256_set1_ps(AttrC[2]);


	int Area = fxd_TriOrientationS(AX, AY, BX, BY, CX, CY);
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

	//Setup Position Interpolation
	fxd_24_8 Inc0 = (AY - BY);
	fxd_24_8 Inc1 = (BY - CY);
	fxd_24_8 Inc2 = (CY - AY);

	fxd_24_8 RowInc0 = (BX - AX);
	fxd_24_8 RowInc1 = (CX - BX);
	fxd_24_8 RowInc2 = (AX - CX);

	__m256i Inc0V = _mm256_set1_epi32(8 * Inc0);
	__m256i Inc1V = _mm256_set1_epi32(8 * Inc1);
	__m256i Inc2V = _mm256_set1_epi32(8 * Inc2);

	__m256i RowInc0V = _mm256_set1_epi32(RowInc0);
	__m256i RowInc1V = _mm256_set1_epi32(RowInc1);
	__m256i RowInc2V = _mm256_set1_epi32(RowInc2);


	fxd_24_8 PX = MinX;
	fxd_24_8 PY = MinY;

	__m256i Bias0 = _mm256_set1_epi32(fxd_TestTopLeftRule(BX, BY, AX, AY) ? 0 : -FXD_FRAC_MIN);
	__m256i Bias1 = _mm256_set1_epi32(fxd_TestTopLeftRule(CX, CY, BX, BY) ? 0 : -FXD_FRAC_MIN);
	__m256i Bias2 = _mm256_set1_epi32(fxd_TestTopLeftRule(AX, AY, CX, CY) ? 0 : -FXD_FRAC_MIN);

	__m256i W0RowV = _mm256_set1_epi32(fxd_TriOrientationS(AX, AY, BX, BY, PX, PY));
	__m256i W1RowV = _mm256_set1_epi32(fxd_TriOrientationS(BX, BY, CX, CY, PX, PY));
	__m256i W2RowV = _mm256_set1_epi32(fxd_TriOrientationS(AX, AY, PX, PY, CX, CY));

	__m256i W0RowBias = _mm256_set_epi32(7 * Inc0, 6 * Inc0, 5 * Inc0, 4 * Inc0, 3 * Inc0, 2 * Inc0, 1 * Inc0, 0 * Inc0);
	__m256i W1RowBias = _mm256_set_epi32(7 * Inc1, 6 * Inc1, 5 * Inc1, 4 * Inc1, 3 * Inc1, 2 * Inc1, 1 * Inc1, 0 * Inc1);
	__m256i W2RowBias = _mm256_set_epi32(7 * Inc2, 6 * Inc2, 5 * Inc2, 4 * Inc2, 3 * Inc2, 2 * Inc2, 1 * Inc2, 0 * Inc2);

	W0RowV = _mm256_add_epi32(W0RowV, W0RowBias);
	W1RowV = _mm256_add_epi32(W1RowV, W1RowBias);
	W2RowV = _mm256_add_epi32(W2RowV, W2RowBias);


	//Setup Attribute Interpolation
	float AttrInc0 = FxdToFloat(Inc0) / FxdToFloat(Area);
	float AttrInc1 = FxdToFloat(Inc1) / FxdToFloat(Area);
	float AttrInc2 = FxdToFloat(Inc2) / FxdToFloat(Area);

	float AttrRowInc0 = FxdToFloat(RowInc0) / FxdToFloat(Area);
	float AttrRowInc1 = FxdToFloat(RowInc1) / FxdToFloat(Area);
	float AttrRowInc2 = FxdToFloat(RowInc2) / FxdToFloat(Area);

	__m256 AttrRowInc0V = _mm256_set1_ps(AttrRowInc0);
	__m256 AttrRowInc1V = _mm256_set1_ps(AttrRowInc1);
	__m256 AttrRowInc2V = _mm256_set1_ps(AttrRowInc2);

	__m256 AttrW0RowV = _mm256_div_ps(Fxd8ToFloat(W0RowV), Fxd8ToFloat(_mm256_set1_epi32(Area)));
	__m256 AttrW1RowV = _mm256_div_ps(Fxd8ToFloat(W1RowV), Fxd8ToFloat(_mm256_set1_epi32(Area)));
	__m256 AttrW2RowV = _mm256_div_ps(Fxd8ToFloat(W2RowV), Fxd8ToFloat(_mm256_set1_epi32(Area)));


	__m256 AttrInc0V = _mm256_set1_ps(8.0f * AttrInc0);
	__m256 AttrInc1V = _mm256_set1_ps(8.0f * AttrInc1);
	__m256 AttrInc2V = _mm256_set1_ps(8.0f * AttrInc2);

	__m256 AttrWARowV[3], AttrWBRowV[3], AttrWCRowV[3];
	AttrWARowV[0] = _mm256_mul_ps(AttrAV[0], AttrW1RowV);
	AttrWARowV[1] = _mm256_mul_ps(AttrAV[1], AttrW1RowV);
	AttrWARowV[2] = _mm256_mul_ps(AttrAV[2], AttrW1RowV);

	AttrWBRowV[0] = _mm256_mul_ps(AttrBV[0], AttrW2RowV);
	AttrWBRowV[1] = _mm256_mul_ps(AttrBV[1], AttrW2RowV);
	AttrWBRowV[2] = _mm256_mul_ps(AttrBV[2], AttrW2RowV);

	AttrWCRowV[0] = _mm256_mul_ps(AttrCV[0], AttrW0RowV);
	AttrWCRowV[1] = _mm256_mul_ps(AttrCV[1], AttrW0RowV);
	AttrWCRowV[2] = _mm256_mul_ps(AttrCV[2], AttrW0RowV);


	__m256 AttrAIncV[3], AttrBIncV[3], AttrCIncV[3];
	AttrAIncV[0] = _mm256_mul_ps(AttrAV[0], AttrInc1V);
	AttrAIncV[1] = _mm256_mul_ps(AttrAV[1], AttrInc1V);
	AttrAIncV[2] = _mm256_mul_ps(AttrAV[2], AttrInc1V);

	AttrBIncV[0] = _mm256_mul_ps(AttrBV[0], AttrInc2V);
	AttrBIncV[1] = _mm256_mul_ps(AttrBV[1], AttrInc2V);
	AttrBIncV[2] = _mm256_mul_ps(AttrBV[2], AttrInc2V);

	AttrCIncV[0] = _mm256_mul_ps(AttrCV[0], AttrInc0V);
	AttrCIncV[1] = _mm256_mul_ps(AttrCV[1], AttrInc0V);
	AttrCIncV[2] = _mm256_mul_ps(AttrCV[2], AttrInc0V);

	__m256 AttrARowIncV[3], AttrBRowIncV[3], AttrCRowIncV[3];
	AttrARowIncV[0] = _mm256_mul_ps(AttrAV[0], AttrRowInc1V);
	AttrARowIncV[1] = _mm256_mul_ps(AttrAV[1], AttrRowInc1V);
	AttrARowIncV[2] = _mm256_mul_ps(AttrAV[2], AttrRowInc1V);

	AttrBRowIncV[0] = _mm256_mul_ps(AttrBV[0], AttrRowInc2V);
	AttrBRowIncV[1] = _mm256_mul_ps(AttrBV[1], AttrRowInc2V);
	AttrBRowIncV[2] = _mm256_mul_ps(AttrBV[2], AttrRowInc2V);

	AttrCRowIncV[0] = _mm256_mul_ps(AttrCV[0], AttrRowInc0V);
	AttrCRowIncV[1] = _mm256_mul_ps(AttrCV[1], AttrRowInc0V);
	AttrCRowIncV[2] = _mm256_mul_ps(AttrCV[2], AttrRowInc0V);

	//Add Top Left Rule Bias after Attribute Setup for correct attribute interpolation
	W0RowV = _mm256_add_epi32(W0RowV, Bias0);
	W1RowV = _mm256_add_epi32(W1RowV, Bias1);
	W2RowV = _mm256_add_epi32(W2RowV, Bias2);

	unsigned *Row = (Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX));
	fxd_24_8 ModuloMask = ~(8 * FXD_ONE - 1);

	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		__m256i W0V = W0RowV;
		__m256i W1V = W1RowV;
		__m256i W2V = W2RowV;

		__m256 AttrWAV[3];
		__m256 AttrWBV[3];
		__m256 AttrWCV[3];

		memcpy(AttrWAV, AttrWARowV, 3 * sizeof(*AttrWAV));
		memcpy(AttrWBV, AttrWBRowV, 3 * sizeof(*AttrWBV));
		memcpy(AttrWCV, AttrWCRowV, 3 * sizeof(*AttrWCV));

		__m256i *Pixels = (__m256i *)Row;
		fxd_24_8 X = MinX;
		int IterCount = 0;
		for(; X < (((MaxX - MinX) & ModuloMask) + MinX); X += 8 * FXD_ONE){

			__m256 fColorV[3];
			fColorV[0] = _mm256_add_ps(AttrWAV[0], AttrWBV[0]);
			fColorV[0] = _mm256_add_ps(fColorV[0], AttrWCV[0]);

			fColorV[1] = _mm256_add_ps(AttrWAV[1], AttrWBV[1]);
			fColorV[1] = _mm256_add_ps(fColorV[1], AttrWCV[1]);

			fColorV[2] = _mm256_add_ps(AttrWAV[2], AttrWBV[2]);
			fColorV[2] = _mm256_add_ps(fColorV[2], AttrWCV[2]);

			__m256i AttrColorV = Float3ToARGBAVX(fColorV);


			__m256i Mask = _mm256_or_si256(W0V, W1V);
			Mask = _mm256_or_si256(W2V, Mask);
			__m256i MaskGt = _mm256_cmpgt_epi32(Mask, _mm256_setzero_si256());
			__m256i MaskEq = _mm256_cmpeq_epi32(Mask, _mm256_setzero_si256());
			__m256i MaskGe = _mm256_or_epi32(MaskEq, MaskGt);

			__m256i PixelV = _mm256_loadu_si256(Pixels);
			//__m256i Tmp = _mm256_blendv_epi8(PixelV, AttrColorV, MaskGe);
			__m256i Tmp = _mm256_blendv_epi8(_mm256_set1_epi32(0xff000000), AttrColorV, MaskGe);
			_mm256_storeu_si256(Pixels, Tmp);
			Pixels++;

			W0V = _mm256_add_epi32(Inc0V, W0V);
			W1V = _mm256_add_epi32(Inc1V, W1V);
			W2V = _mm256_add_epi32(Inc2V, W2V);

			AttrWAV[0] = _mm256_add_ps(AttrWAV[0], AttrAIncV[0]);
			AttrWAV[1] = _mm256_add_ps(AttrWAV[1], AttrAIncV[1]);
			AttrWAV[2] = _mm256_add_ps(AttrWAV[2], AttrAIncV[2]);

			AttrWBV[0] = _mm256_add_ps(AttrWBV[0], AttrBIncV[0]);
			AttrWBV[1] = _mm256_add_ps(AttrWBV[1], AttrBIncV[1]);
			AttrWBV[2] = _mm256_add_ps(AttrWBV[2], AttrBIncV[2]);

			AttrWCV[0] = _mm256_add_ps(AttrWCV[0], AttrCIncV[0]);
			AttrWCV[1] = _mm256_add_ps(AttrWCV[1], AttrCIncV[1]);
			AttrWCV[2] = _mm256_add_ps(AttrWCV[2], AttrCIncV[2]);
			IterCount++;
		}

		unsigned *Pixel = (unsigned *)Pixels;
		fxd_48_16 W0 = _mm256_cvtsi256_si32(W0V);
		fxd_48_16 W1 = _mm256_cvtsi256_si32(W1V);
		fxd_48_16 W2 = _mm256_cvtsi256_si32(W2V);

		float AttrWA[3], AttrWB[3], AttrWC[3];
		AttrWA[0] = _mm256_cvtss_f32(AttrWAV[0]);
		AttrWA[1] = _mm256_cvtss_f32(AttrWAV[1]);
		AttrWA[2] = _mm256_cvtss_f32(AttrWAV[2]);

		AttrWB[0] = _mm256_cvtss_f32(AttrWBV[0]);
		AttrWB[1] = _mm256_cvtss_f32(AttrWBV[1]);
		AttrWB[2] = _mm256_cvtss_f32(AttrWBV[2]);

		AttrWC[0] = _mm256_cvtss_f32(AttrWCV[0]);
		AttrWC[1] = _mm256_cvtss_f32(AttrWCV[1]);
		AttrWC[2] = _mm256_cvtss_f32(AttrWCV[2]);

		float AttrAInc[3], AttrBInc[3], AttrCInc[3];
		AttrAInc[0] = _mm256_cvtss_f32(AttrAIncV[0]) / 8.0f;
		AttrAInc[1] = _mm256_cvtss_f32(AttrAIncV[1]) / 8.0f;
		AttrAInc[2] = _mm256_cvtss_f32(AttrAIncV[2]) / 8.0f;

		AttrBInc[0] = _mm256_cvtss_f32(AttrBIncV[0]) / 8.0f;
		AttrBInc[1] = _mm256_cvtss_f32(AttrBIncV[1]) / 8.0f;
		AttrBInc[2] = _mm256_cvtss_f32(AttrBIncV[2]) / 8.0f;

		AttrCInc[0] = _mm256_cvtss_f32(AttrCIncV[0]) / 8.0f;
		AttrCInc[1] = _mm256_cvtss_f32(AttrCIncV[1]) / 8.0f;
		AttrCInc[2] = _mm256_cvtss_f32(AttrCIncV[2]) / 8.0f;



		while(X <= MaxX){
			float fColor[3] = {0};
			fColor[0] = AttrWA[0] + AttrWB[0] + AttrWC[0];
			fColor[1] = AttrWA[1] + AttrWB[1] + AttrWC[1];
			fColor[2] = AttrWA[2] + AttrWB[2] + AttrWC[2];
			unsigned AttrColor = Float3ToARGB(fColor);

			unsigned tmp = *Pixel;
			tmp = ((W0 | W1 | W2) >= 0) ? AttrColor : tmp;
			*Pixel++ = tmp;
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;

			AttrWA[0] += AttrAInc[0];
			AttrWA[1] += AttrAInc[1];
			AttrWA[2] += AttrAInc[2];

			AttrWB[0] += AttrBInc[0];
			AttrWB[1] += AttrBInc[1];
			AttrWB[2] += AttrBInc[2];

			AttrWC[0] += AttrCInc[0];
			AttrWC[1] += AttrCInc[1];
			AttrWC[2] += AttrCInc[2];

			X += FXD_ONE;
		}


		Row += Buffer->Width;
		W0RowV = _mm256_add_epi32(RowInc0V, W0RowV);
		W1RowV = _mm256_add_epi32(RowInc1V, W1RowV);
		W2RowV = _mm256_add_epi32(RowInc2V, W2RowV);

		AttrWARowV[0] = _mm256_add_ps(AttrWARowV[0], AttrARowIncV[0]);
		AttrWARowV[1] = _mm256_add_ps(AttrWARowV[1], AttrARowIncV[1]);
		AttrWARowV[2] = _mm256_add_ps(AttrWARowV[2], AttrARowIncV[2]);

		AttrWBRowV[0] = _mm256_add_ps(AttrWBRowV[0], AttrBRowIncV[0]);
		AttrWBRowV[1] = _mm256_add_ps(AttrWBRowV[1], AttrBRowIncV[1]);
		AttrWBRowV[2] = _mm256_add_ps(AttrWBRowV[2], AttrBRowIncV[2]);

		AttrWCRowV[0] = _mm256_add_ps(AttrWCRowV[0], AttrCRowIncV[0]);
		AttrWCRowV[1] = _mm256_add_ps(AttrWCRowV[1], AttrCRowIncV[1]);
		AttrWCRowV[2] = _mm256_add_ps(AttrWCRowV[2], AttrCRowIncV[2]);
	}


}

//Unfortunately slow
struct RasterizerOutputElement{
	int Index;
	float BaryA, BaryB, BaryC;
};

struct RasterizerOutput{
	struct RasterizerOutputElement *Data;
	size_t ElementCount;
};

internal void fxd_RasterizeTri2(IScreenBuffer *Buffer,
	fxd_24_8 _AX, fxd_24_8 _AY,
	fxd_24_8 _BX, fxd_24_8 _BY,
	fxd_24_8 _CX, fxd_24_8 _CY,
	struct RasterizerOutput *Output
){
	//Based on https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
	//Input is Y-Down Counter-Clockwise tri. But the math is for Y-Up coordinate systems
	//Change the orientation by swapping the order of the second and third vertex
	//Then process this as a Y-Up Counter-Clockwise try
	fxd_24_8 AX = _AX; fxd_24_8 AY = _AY;
	fxd_24_8 BX = _CX; fxd_24_8 BY = _CY;
	fxd_24_8 CX = _BX; fxd_24_8 CY = _BY;


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

	//Setup Position Interpolation
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

	//Setup Attribute Interpolation
	float BaryARow = FxdToFloat(W1Row) / FxdToFloat(Area);
	float BaryBRow = FxdToFloat(W2Row) / FxdToFloat(Area);
	float BaryCRow = FxdToFloat(W0Row) / FxdToFloat(Area);

	float BaryAInc = FxdToFloat(Inc1) / FxdToFloat(Area);
	float BaryBInc = FxdToFloat(Inc2) / FxdToFloat(Area);
	float BaryCInc = FxdToFloat(Inc0) / FxdToFloat(Area);

	float BaryARowInc = FxdToFloat(RowInc1) / FxdToFloat(Area);
	float BaryBRowInc = FxdToFloat(RowInc2) / FxdToFloat(Area);
	float BaryCRowInc = FxdToFloat(RowInc0) / FxdToFloat(Area);

	//Add Bias after calculating weights for correct Attribute Interpolation
	W0Row += Bias0;
	W1Row += Bias1;
	W2Row += Bias2;
	struct RasterizerOutputElement *El = Output->Data;
	unsigned *Row = Buffer->Data + FxdToIntF(PY) * Buffer->Width + FxdToIntF(PX);
	Assert(FxdToIntF(MaxX) * FxdToIntF(MaxY) <= Output->ElementCount);
	for(fxd_24_8 Y = MinY; Y <= MaxY; Y += FXD_ONE){
		fxd_24_8 W0 = W0Row;
		fxd_24_8 W1 = W1Row;
		fxd_24_8 W2 = W2Row;

		float BaryA = BaryARow;
		float BaryB = BaryBRow;
		float BaryC = BaryCRow;



		unsigned *Pixels = Row;
		int Index = FxdToIntF(Y) * Buffer->Width + FxdToIntF(PX);
		for(fxd_24_8 X = MinX; X <= MaxX; X += FXD_ONE){
			El->Index = ((W0 | W1 | W2) >= 0) ? Index : -1;
			El->BaryA = ((W0 | W1 | W2) >= 0) ? BaryA : 0;
			El->BaryB = ((W0 | W1 | W2) >= 0) ? BaryC : 0;
			El->BaryC = ((W0 | W1 | W2) >= 0) ? BaryB : 0;
			El += ((W0 | W1 | W2) >= 0) ? 1 : 0;
			Index++;
			W0 += Inc0;
			W1 += Inc1;
			W2 += Inc2;
			BaryA += BaryAInc;
			BaryB += BaryBInc;
			BaryC += BaryCInc;
		}
		Row += Buffer->Width;
		W0Row += RowInc0;
		W1Row += RowInc1;
		W2Row += RowInc2;

		BaryARow += BaryARowInc;
		BaryBRow += BaryBRowInc;
		BaryCRow += BaryCRowInc;

	}
}



