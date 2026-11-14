#pragma once
#include "fxd_tri.h"

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
