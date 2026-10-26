#pragma once
#define FRACTION_BITS 8
#define INTEGER_BITS 24
#define FXD_PRINT_DECIMALPOINTS 7
#define FXD_HALF (1 <<(FRACTION_BITS - 1))
#define FXD_ONE (1 << FRACTION_BITS)
#define FXD_FRAC_MIN 1
#define FXD_FRAC_MAX ((1 << FRACTION_BITS) - 1)
typedef int fxd_24_8;
typedef long long fxd_48_16;


global const float float_fxd_max = (float)(1 << (INTEGER_BITS - 1)) - (1.0f / (float)(1 << FRACTION_BITS));
global const float float_fxd_min = (float)-(1 << (INTEGER_BITS - 1));
global const fxd_24_8 fxd_max = (1LL << (INTEGER_BITS + FRACTION_BITS - 1)) - 1;
global const fxd_24_8 fxd_min = -(1 << (INTEGER_BITS + FRACTION_BITS - 1));

#define IntToFxd(x) (((x) & 0x00ffffff) << FRACTION_BITS)
internal int FxdToIntR(fxd_24_8 n); //Round
internal int FxdToIntF(fxd_24_8 n); //Floor
internal void fxd_FormatFxd(fxd_24_8 n, char* String, size_t Size);
internal fxd_24_8 fxd_Abs(fxd_24_8 n);
internal inline fxd_24_8 fxd_Round(fxd_24_8 n);
internal inline fxd_24_8 fxd_Floor(fxd_24_8 n);
internal inline fxd_24_8 FloatToFixed(float n);
internal fxd_48_16 FixedMul(fxd_24_8 _A, fxd_24_8 _B);
internal fxd_24_8 FixedMulS(fxd_24_8 _A, fxd_24_8 _B);
internal inline fxd_24_8 DToSFxd(fxd_48_16 N);
internal inline fxd_24_8 NegateFxd(fxd_24_8 N);
internal inline fxd_24_8 fxd_Min3(fxd_24_8 A, fxd_24_8 B, fxd_24_8 C);
internal inline fxd_24_8 fxd_Max3(fxd_24_8 A, fxd_24_8 B, fxd_24_8 C);





internal void fxd_FormatFxd(fxd_24_8 n, char *String, size_t Size){
	snprintf(String,Size,"%d.%u", (n >> FRACTION_BITS), (unsigned)(((n & 0xff) * powf(10.0f, FXD_PRINT_DECIMALPOINTS)) / 256));
}

internal inline fxd_24_8 FloatToFixed(float n){
	//Conversion according to https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.2.4.1%20FLOAT%20-%3E%20Fixed%20Point%20Integer
	if(n == NAN) return 0;
	int Exponent;
	(void)frexpf(n, &Exponent);
	if(Exponent < (1 - FRACTION_BITS)) return 0;
	if(n >= float_fxd_max) return fxd_max;
	if(n <= float_fxd_min) return fxd_min;
	return (fxd_24_8)(n * (float)(1 << FRACTION_BITS));
}

internal inline float FixedToFloat(fxd_24_8 n){
	//Conversion according to https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.2.4.1%20FLOAT%20-%3E%20Fixed%20Point%20Integer
	float Result = (float)(n >> FRACTION_BITS);
	Result += (float)((n & FXD_FRAC_MAX) / FXD_ONE);
	return Result;
}


internal fxd_48_16 FixedMul(fxd_24_8 _A, fxd_24_8 _B){
	fxd_48_16 A = (fxd_24_8)_A;
	fxd_48_16 B = (fxd_24_8)_B;
	fxd_48_16 Res = A * B;
	return Res;
}

internal fxd_24_8 FixedMulS(fxd_24_8 _A, fxd_24_8 _B){
	fxd_48_16 A = (fxd_24_8)_A;
	fxd_48_16 B = (fxd_24_8)_B;
	fxd_48_16 Res = A * B;
	return DToSFxd(Res);
}


internal inline fxd_24_8 DToSFxd(fxd_48_16 N){
	return (fxd_24_8)((N >> FRACTION_BITS) & 0xffffffff);
}

internal inline fxd_24_8 NegateFxd(fxd_24_8 N){
	return DToSFxd(FixedMul(IntToFxd(-1), N));
}

internal inline fxd_24_8 fxd_Abs(fxd_24_8 n){
	return (n + (n >> 31)) ^ (n >> 31);
}
internal inline fxd_24_8 fxd_Round(fxd_24_8 n){
	n += (n >= FXD_HALF) ?  FXD_HALF : 0;
	n &= ~((1 << FRACTION_BITS) - 1);
	return n;
}

internal inline fxd_24_8 fxd_Floor(fxd_24_8 n){
	n &= ~((1 << FRACTION_BITS) - 1);
	return n;
}

internal int FxdToIntR(fxd_24_8 n){
	int Res = n;
	Res += (n >= FXD_HALF) ? FXD_HALF : 0;
	Res = Res >> FRACTION_BITS;
	return Res;
}

internal int FxdToIntF(fxd_24_8 n){
	int Res = n;
	Res = Res >> FRACTION_BITS;
	return Res;
}

internal inline fxd_24_8 fxd_Min3(fxd_24_8 A, fxd_24_8 B, fxd_24_8 C){
	fxd_24_8 Min = A;
	Min = Min < B ? Min : B;
	Min = Min < C ? Min : C;
	return Min;
}

internal inline fxd_24_8 fxd_Max3(fxd_24_8 A, fxd_24_8 B, fxd_24_8 C){
	fxd_24_8 Max = A;
	Max = Max > B ? Max : B;
	Max = Max > C ? Max : C;
	return Max;
}

