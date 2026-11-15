#pragma once
#define DEBUG 1
#include "Ix64.h"

#include "IDebug.h"

#define global static
#define internal static
#define Assert(x) if(!(x)) __debugbreak();
typedef unsigned char byte;



#define APP 1

internal inline unsigned Float3ToARGB(float ArrRGB[3]){
	unsigned Color = 0xff000000;
	Color |= (unsigned)ArrRGB[0] << 16;
	Color |= (unsigned)ArrRGB[1] << 8;
	Color |= (unsigned)ArrRGB[2] << 0;
	return Color;
}


#include <emmintrin.h>
internal inline __m128i Float3ToARGBSSE(__m128 ArrRGB[3]){
	__m128i iColor = _mm_set1_epi32(0xff000000);
	iColor = _mm_or_si128(iColor, _mm_slli_epi32(_mm_cvtps_epi32(ArrRGB[0]), 16));
	iColor = _mm_or_si128(iColor, _mm_slli_epi32(_mm_cvtps_epi32(ArrRGB[1]), 8));
	iColor = _mm_or_si128(iColor, _mm_cvtps_epi32(ArrRGB[2]));
	return iColor;
}

#include <immintrin.h>

internal inline __m256i Float3ToARGBAVX(__m256 ArrRGB[3]){
	__m256i iColor = _mm256_set1_epi32(0xff000000);
	iColor = _mm256_or_si256(iColor, _mm256_slli_epi32(_mm256_cvtps_epi32(ArrRGB[0]), 16));
	iColor = _mm256_or_si256(iColor, _mm256_slli_epi32(_mm256_cvtps_epi32(ArrRGB[1]), 8));
	iColor = _mm256_or_si256(iColor, _mm256_cvtps_epi32(ArrRGB[2]));
	return iColor;
}