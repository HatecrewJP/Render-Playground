#pragma once
typedef int fxd_24_8;
typedef long long fxd_48_16;
#define FRACTION_BITS 8
#define INTEGER_BITS 24
#define FXD_PRINT_DECIMALPOINTS 7
#define FXD_HALF (1 <<(FRACTION_BITS - 1))
#define FXD_ONE (1 << FRACTION_BITS)
#define FXD_FRAC_MIN 1
#define FXD_FRAC_MAX ((1 << FRACTION_BITS) - 1)

//Conversions
#define IntToFxd(x) (((x) & 0x00ffffff) << FRACTION_BITS)
internal int FxdToIntR(fxd_24_8 n); //Round
internal int FxdToIntF(fxd_24_8 n); //Floor
internal inline fxd_24_8 FloatToFxd(float n);
internal inline float FxdToFloat(fxd_24_8 n);
internal inline fxd_24_8 DToSFxd(fxd_48_16 N);
internal inline fxd_48_16 SToDFxd(fxd_24_8 N);

//Printing
internal void fxd_FormatFxd(fxd_24_8 n, char* String, size_t Size);

//Unary Operations
internal fxd_24_8 fxd_Abs(fxd_24_8 n);
internal inline fxd_24_8 fxd_Round(fxd_24_8 n);
internal inline fxd_48_16 fxd_RoundD(fxd_48_16 n);
internal inline fxd_24_8 fxd_Floor(fxd_24_8 n);
internal inline fxd_24_8 NegateFxd(fxd_24_8 N);

//Arithmetic
internal fxd_48_16 FixedMul(fxd_24_8 _A, fxd_24_8 _B);
internal fxd_24_8 FixedMulS(fxd_24_8 _A, fxd_24_8 _B);
internal fxd_24_8 FixedDivS(fxd_24_8 _A, fxd_24_8 _B);

//Min Max
global const float float_fxd_max = (float)(1 << (INTEGER_BITS - 1)) - (1.0f / (float)(1 << FRACTION_BITS));
global const float float_fxd_min = (float)-(1 << (INTEGER_BITS - 1));
global const fxd_24_8 fxd_max = (1LL << (INTEGER_BITS + FRACTION_BITS - 1)) - 1;
global const fxd_24_8 fxd_min = -(1 << (INTEGER_BITS + FRACTION_BITS - 1));
internal inline fxd_24_8 fxd_Min3(fxd_24_8 A, fxd_24_8 B, fxd_24_8 C);
internal inline fxd_24_8 fxd_Max3(fxd_24_8 A, fxd_24_8 B, fxd_24_8 C);


/*********************/
/*                   */
/*  Implementations  */
/*                   */
/*********************/


/***************/
/* Conversions */
/***************/

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

internal inline fxd_24_8 FloatToFxd(float n){
	//Conversion according to https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.2.4.1%20FLOAT%20-%3E%20Fixed%20Point%20Integer
	if(n == NAN) return 0;
	int Exponent;
	(void)frexpf(n, &Exponent);
	if(Exponent < (1 - FRACTION_BITS)) return 0;
	if(n >= float_fxd_max) return fxd_max;
	if(n <= float_fxd_min) return fxd_min;
	return (fxd_24_8)(n * (float)(1 << FRACTION_BITS));
}

internal inline float FxdToFloat(fxd_24_8 n){
	//Conversion according to https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.2.4.1%20FLOAT%20-%3E%20Fixed%20Point%20Integer
	float Result = (float)(n >> FRACTION_BITS);
	Result += (float)((n & FXD_FRAC_MAX) / FXD_ONE);
	return Result;
}
internal inline fxd_24_8 DToSFxd(fxd_48_16 N){
	return (fxd_24_8)((N >> FRACTION_BITS) & 0xffffffff);
}

internal inline fxd_48_16 SToDFxd(fxd_24_8 N){
	return ((fxd_48_16)N) << FRACTION_BITS;
}

/************/
/* Printing */
/************/
internal void fxd_FormatFxd(fxd_24_8 n, char *String, size_t Size){
	//Fraction part is unsigned. There its (int << FRACTION_BITS) + fraction.
	//Therefore, negative numbers have an integer part that is 1 less than what the fixed point number actually represents.
	//This also means, that the fraction part is stored as the inverse of the actual fraction
	// Example: -4.25: -5 + 0.75
	// Integer     -4: -5 + 1
	// Fraction: 0.25:  1 - 0.75
	//To get the actual integer representation of the number, add 1 to the integer part.
	if(n < 0 && ((n&0xff) != 0)){
		snprintf(String,Size,"%d.%u", (n >> FRACTION_BITS) + 1, (unsigned)((((FXD_ONE)-(n & 0xff)) * powf(10.0f, FXD_PRINT_DECIMALPOINTS)) / 256));
	}
	else{
		snprintf(String, Size, "%d.%u", (n >> FRACTION_BITS), (unsigned)(((n & 0xff) * powf(10.0f, FXD_PRINT_DECIMALPOINTS)) / 256));
	}
}

/********************/
/* Unary Operations */
/********************/


internal inline fxd_24_8 fxd_Abs(fxd_24_8 n){
	return (n + (n >> 31)) ^ (n >> 31);
}
internal inline fxd_24_8 fxd_Round(fxd_24_8 n){
	n += (n >= FXD_HALF) ? FXD_HALF : 0;
	n &= ~((1 << FRACTION_BITS) - 1);
	return n;
}
internal inline fxd_48_16 fxd_RoundD(fxd_48_16 n){
	n += (n >= (1 << (2 * FRACTION_BITS))) ? (1 << (2 * FRACTION_BITS)) : 0;
	n &= ~((1 << (2 * FRACTION_BITS)) - 1);
	return n;
}

internal inline fxd_24_8 fxd_Floor(fxd_24_8 n){
	//Floors towards -INF
	n &= ~((1 << FRACTION_BITS) - 1);
	return n;
}
internal inline fxd_24_8 NegateFxd(fxd_24_8 N){
	return DToSFxd(FixedMul(IntToFxd(-1), N));
}


/**************/
/* Arithmetic */
/**************/

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

internal fxd_24_8 FixedDivS(fxd_24_8 _A, fxd_24_8 _B){
	//This functions returns the next lower represenation of the result
	if(_B == 0) return 0;
	fxd_48_16 A = SToDFxd(_A);
	fxd_48_16 B = SToDFxd(_B);
	A = A << FRACTION_BITS;
	return DToSFxd((A/B) << FRACTION_BITS);

}

internal fxd_24_8 FixedDivS2(fxd_24_8 _A, fxd_24_8 _B){
	//This function returns the nearest represenation of the Result
	if(_B == 0) return 0;
	fxd_48_16 A = SToDFxd(_A);
	fxd_48_16 B = SToDFxd(_B);
	
	A = A << FRACTION_BITS;
	fxd_48_16 Tmp = (A / B) << (2*FRACTION_BITS);
	fxd_48_16 Res = fxd_RoundD(Tmp) >> FRACTION_BITS;
	return DToSFxd(Res) ;

}

/***********/
/* Min Max */
/***********/
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

