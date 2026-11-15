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