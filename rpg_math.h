#pragma once
#include "Application.h"
#include "math.h"
#define PI 3.141592653589793238462643383279f

struct Vec2{
	float x;
	float y;
};

internal inline float DistVec2(Vec2 a, Vec2 b){
	return (float)sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
}
internal Vec2 NormalizeVec2(Vec2 v){
	float Magnitude = (float)sqrt(v.x * v.x + v.y * v.y);
	if(-0.00001f <= Magnitude && Magnitude <= 0.00001f){
		return v;
	}
	Vec2 Result;
	Result.x = v.x / Magnitude;
	Result.y = v.y / Magnitude;
	return Result;
}

internal inline int Abs32(int n){
	return (n + (n >> 31)) ^ (n >> 31);
}
internal inline float RoundFloat(float In){
	return (float)((int)(In + 0.5f));
}
internal inline int RoundFloatToInt(float In){
	return (int)(In + 0.5f);
}
internal inline unsigned  RoundFloatToUInt(float In){
	return (unsigned)(In + 0.5f);
}

internal inline float fMin3(float A, float B, float C){
	float Min = A;
	Min = Min < B ? Min : B;
	Min = Min < C ? Min : C;
	return Min;
}

internal inline float fMax3(float A, float B, float C){
	float Max = A;
	Max = Max > B ? Max : B;
	Max = Max > C ? Max : C;
	return Max;
}

internal inline int Min3(int A, int B, int C){
	int Min = A;
	Min = Min < B ? Min : B;
	Min = Min < C ? Min : C;
	return Min;
}

internal inline int Max3(int A, int B, int C){
	int Max = A;
	Max = Max > B ? Max : B;
	Max = Max > C ? Max : C;
	return Max;
}