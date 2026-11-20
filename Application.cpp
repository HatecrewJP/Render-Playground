#include <stdio.h>
#include <memory.h>


#include "Application.h"

global struct IDebugContext *GlobalDebugCtx;



internal inline void SetPixel(IScreenBuffer *Buffer, int X, int Y, unsigned Color){
	//Assumes that the point is valid
	//Pixel center is at 0,0
	unsigned *Pixel = Buffer->Data + Y * Buffer->Width + X;
	*Pixel = Color;
}

#include "fxd_tri.h"
#include "line.h"


internal void SetBackgroundColor(IScreenBuffer *Buffer, unsigned BackgroundColor){
	unsigned *Pixels = Buffer->Data;
	for(int i = 0; i < (Buffer->Width * Buffer->Height); i++){
		*Pixels = BackgroundColor;
		Pixels++;
	}
}

internal void SetBackgroundColorSSE(IScreenBuffer *Buffer, unsigned BackgroundColor){
	__m128i *PixelsV = (__m128i*)Buffer->Data;
	__m128i BackgroundColorV = _mm_set1_epi32(BackgroundColor);

	int IterationCount = (Buffer->Width * Buffer->Height);
	int i = 0;
	for(; i < (IterationCount & ~3); i+=4){
		memcpy(PixelsV, &BackgroundColorV, sizeof(BackgroundColorV));
		PixelsV++;
	}
	unsigned *Pixels = Buffer->Data;
	while(i < IterationCount){
		*Pixels = BackgroundColor;
		i++;
	}
}

internal void SetBackgroundColorAVX(IScreenBuffer *Buffer, unsigned BackgroundColor){
	__m256i *PixelsV = (__m256i *)Buffer->Data;
	__m256i BackgroundColorV = _mm256_set1_epi32(BackgroundColor);

	int IterationCount = (Buffer->Width * Buffer->Height);
	int i = 0;
	for(; i < (IterationCount & ~7); i+=8){
		memcpy(PixelsV, &BackgroundColorV, sizeof(BackgroundColorV));
		PixelsV++;
	}
	unsigned *Pixels = Buffer->Data;
	while(i < IterationCount){
		*Pixels = BackgroundColor;
		i++;
	}
}

internal void fxd_RasterizeRectangle(IScreenBuffer *Buffer,
	fxd_24_8 Left, fxd_24_8 Top,
	fxd_24_8 Right, fxd_24_8 Bottom,
	unsigned Color){
	fxd_24_8 MinX = fxd_Round(Left - FXD_HALF) + FXD_HALF;
	fxd_24_8 MinY = fxd_Round(Top - FXD_HALF) + FXD_HALF;
	fxd_24_8 MaxX = fxd_Round(Right - FXD_HALF) + FXD_HALF;
	fxd_24_8 MaxY = fxd_Round(Bottom - FXD_HALF) + FXD_HALF;

	if(MinX < FXD_HALF){
		MinX = FXD_HALF;
	}
	if(MinY < FXD_HALF){
		MinY = FXD_HALF;
	}
	if(MaxX >= IntToFxd(Buffer->Width)){
		MaxX = IntToFxd(Buffer->Width);
	}
	if(MaxY >= IntToFxd(Buffer->Height)){
		MaxY = IntToFxd(Buffer->Height);
	}
	unsigned *Row = Buffer->Data + FxdToIntF(MinY) * Buffer->Width + FxdToIntR(MinX);
	for(fxd_24_8 Y = MinY; Y < MaxY; Y += FXD_ONE){
		unsigned *Pixels = Row;
		for(fxd_24_8 X = MinX; X < MaxX; X += FXD_ONE){
			*Pixels = Color;
			Pixels++;
		}
		Row += Buffer->Width;
	}
}




internal void fxd_DrawQuad(IScreenBuffer *Buffer,
	fxd_24_8 X1, fxd_24_8 Y1, float *Attr1,
	fxd_24_8 X2, fxd_24_8 Y2, float *Attr2,
	fxd_24_8 X3, fxd_24_8 Y3, float *Attr3,
	fxd_24_8 X4, fxd_24_8 Y4, float *Attr4
){
	//Diagonal is 2-4
	fxd_RasterizeTri(Buffer, X1, Y1, Attr1, X2, Y2, Attr2, X4, Y4, Attr4);
	fxd_RasterizeTri(Buffer, X3, Y3, Attr3, X4, Y4, Attr4, X2, Y2, Attr2);
}

internal void fxd_DrawQuadSSE(IScreenBuffer *Buffer,
	fxd_24_8 X1, fxd_24_8 Y1, float *Attr1,
	fxd_24_8 X2, fxd_24_8 Y2, float *Attr2,
	fxd_24_8 X3, fxd_24_8 Y3, float *Attr3,
	fxd_24_8 X4, fxd_24_8 Y4, float *Attr4
){
	//Diagonal is 2-4
	fxd_RasterizeTriSSE(Buffer, X1, Y1, Attr1, X2, Y2, Attr2, X4, Y4, Attr4);
	fxd_RasterizeTriSSE(Buffer, X2, Y2, Attr2, X3, Y3, Attr3, X4, Y4, Attr4);
}

internal void fxd_DrawQuadAVX(IScreenBuffer *Buffer,
	fxd_24_8 X1, fxd_24_8 Y1, float *Attr1,
	fxd_24_8 X2, fxd_24_8 Y2, float *Attr2,
	fxd_24_8 X3, fxd_24_8 Y3, float *Attr3,
	fxd_24_8 X4, fxd_24_8 Y4, float *Attr4
){
	//Diagonal is 2-4
	fxd_RasterizeTriAVX(Buffer, X1, Y1,Attr1, X2, Y2,Attr2, X4, Y4,Attr4);
	fxd_RasterizeTriAVX(Buffer, X2, Y2,Attr2, X3, Y3,Attr3, X4, Y4,Attr4);
}



internal void fxd_DrawQuadOutline(IScreenBuffer *Buffer,
	fxd_24_8 X1, fxd_24_8 Y1,
	fxd_24_8 X2, fxd_24_8 Y2,
	fxd_24_8 X3, fxd_24_8 Y3,
	fxd_24_8 X4, fxd_24_8 Y4,
	unsigned Color)
{
	//Diagonal is 2-4
	fxd_BresenhamLine2(Buffer, X1, Y1, X2, Y2, Color);
	fxd_BresenhamLine2(Buffer, X2, Y2, X3, Y3, Color);
	fxd_BresenhamLine2(Buffer, X3, Y3, X4, Y4, Color);
	fxd_BresenhamLine2(Buffer, X4, Y4, X1, Y1, Color);
}


internal void RasterizeTriOutline(IScreenBuffer *Buffer, 
	int AX, int AY,
	int BX, int BY,
	int CX, int CY, 
	unsigned Color)
{
	BresenhamLine2(Buffer, AX,AY,BX,BY, Color);
	BresenhamLine2(Buffer, BX,BY,CX,CY, Color);
	BresenhamLine2(Buffer, CX,CY,AX,AY, Color);
}

internal void RasterizePoint(IScreenBuffer *Buffer,
	int X, int Y,
	unsigned Color)
{
	if(X >= 0 && X <= Buffer->Width && Y >= 0 && Y <= Buffer->Height){
		*(Buffer->Data + Y * Buffer->Width + X) = Color;
	}
}

internal void fxd_RasterizePoint(IScreenBuffer *Buffer,
	fxd_24_8 _X, fxd_24_8 _Y, float *Attr)
{
	fxd_24_8 X = FxdToIntF(_X - FXD_FRAC_MIN);
	fxd_24_8 Y = FxdToIntF(_Y - FXD_FRAC_MIN);
	if(X >= 0 && X <= Buffer->Width && Y >= 0 && Y <= Buffer->Height){
		*(Buffer->Data + Y * Buffer->Width + X) = Float4ToARGB(Attr);
	}
}

internal void fxd_RasterizePoint2(IScreenBuffer *Buffer,
	fxd_24_8 X, fxd_24_8 Y,float *Attr){
	fxd_24_8 TmpX = fxd_Floor(X - FXD_FRAC_MIN) + FXD_HALF;
	fxd_24_8 TmpY = fxd_Floor(Y - FXD_FRAC_MIN) + FXD_HALF;
	if(TmpX >= 0 && TmpX <= (IntToFxd(Buffer->Width) - FXD_HALF) && TmpY >= 0 && TmpY <= (IntToFxd(Buffer->Height) - FXD_HALF)){
		fxd_SetPixel(Buffer, TmpX, TmpY, Float4ToARGB(Attr));
	}
}

internal void fxd_RasterizePoint3(IScreenBuffer *Buffer,
	fxd_24_8 X, fxd_24_8 Y,
	float *fColor)
{
	fxd_RasterizeTri(Buffer, X - FXD_HALF, Y - FXD_HALF, fColor, X - FXD_HALF, Y + FXD_HALF, fColor, X + FXD_HALF, Y - FXD_HALF, fColor);
	fxd_RasterizeTri(Buffer, X - FXD_HALF, Y + FXD_HALF, fColor, X + FXD_HALF, Y + FXD_HALF, fColor, X + FXD_HALF, Y - FXD_HALF, fColor);
}



internal void NaiveRasterizeCircleOutline(IScreenBuffer *Buffer, 
	float X,float Y,
	float Radius, unsigned Color)
{
	Assert(Buffer);
	float Resolution = 0.0001f;
	unsigned *Pixels = Buffer->Data;

	for(float a = 0; a < 2 * PI; a += Resolution){
		
		float CurrentPosX = X + (float)RoundFloat((float)cos(a) * Radius);
		float CurrentPosY = Y + (float)RoundFloat((float)sin(a) * Radius);

		if(CurrentPosX >= Buffer->Width || CurrentPosY >= Buffer->Height){
			continue;
		}
		if(CurrentPosX < 0 || CurrentPosY < 0){
			continue;
		}
		int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
		Pixels[Index] = Color;
	}
}

internal void NaiveRasterizeQuadraticBezierCurve(IScreenBuffer *Buffer, 
	float SX, float SY, //Start
	float EX, float EY, //End
	float CX, float CY, //Control Point
	unsigned Color)
{
	unsigned *Pixels = Buffer->Data;
	float Resolution = 0.0001f;
	
	for(float t = 0; t <= 1; t += Resolution){
		
		float CurrentPosX = RoundFloat((1 - t) * (1 - t) * SX + 2 * (1 - t) * t * CX + t * t * EX);
		float CurrentPosY = RoundFloat((1 - t) * (1 - t) * SY + 2 * (1 - t) * t * CY + t * t * EY);


		if(CurrentPosX >= Buffer->Width || CurrentPosY >= Buffer->Height){
			continue;
		}
		if(CurrentPosX < 0 || CurrentPosY < 0){
			continue;
		}

		int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
		Pixels[Index] = Color;
	}
}


internal void NaiveRasterizeCubicBezierCurve(IScreenBuffer *Buffer, 
	float P0X, float P0Y,
	float P1X, float P1Y,
	float P2X, float P2Y,
	float P3X, float P3Y,
	unsigned Color)
{
	unsigned *Pixels = Buffer->Data;
	float Resolution = 0.0001f;
	for(float t = 0; t <= 1; t += Resolution){
		
		float CurrentPosX = RoundFloat(P0X + t * (-3 * P0X + 3 * P1X) + t * t * (3 * P0X - 6 * P1X + 3 * P2X) + t * t * t * (-P0X + 3 * P1X - 3 * P2X + P3X));
		float CurrentPosY = RoundFloat(P0Y + t * (-3 * P0Y + 3 * P1Y) + t * t * (3 * P0Y - 6 * P1Y + 3 * P2Y) + t * t * t * (-P0Y + 3 * P1Y - 3 * P2Y + P3Y));

		if(CurrentPosX >= Buffer->Width || CurrentPosY >= Buffer->Height){
			continue;
		}
		if(CurrentPosX < 0 || CurrentPosY < 0){
			continue;
		}

		int Index = (int)CurrentPosX + (int)CurrentPosY * Buffer->Width;
		Pixels[Index] = Color;
	}
}
struct Vertex{
	float X, Y;
	union{
		struct{
			float R, G, B, A;
		}Color;
		float R, G, B, A;
	};
};

struct fxd_Vertex{
	fxd_24_8 X, Y;
	union{
		struct{
			float R, G, B, A;
		}Color;
		float ColorArr[4];
		float R, G, B, A;
	};
	
};

internal void DrawTriangleList(IScreenBuffer *Buffer, Vertex *VertexList, int VertexCount){
	for(int i = 0; i < (VertexCount & ~2); i+=3){
		struct fxd_Vertex V1, V2, V3;
		memcpy(&V1.Color,&VertexList[i+0].Color,sizeof(V1.Color));
		memcpy(&V2.Color,&VertexList[i+1].Color,sizeof(V2.Color));
		memcpy(&V3.Color,&VertexList[i+2].Color,sizeof(V3.Color));

		V1.X = FloatToFxd(VertexList[i + 0].X);
		V2.X = FloatToFxd(VertexList[i + 1].X);
		V3.X = FloatToFxd(VertexList[i + 2].X);

		V1.Y = FloatToFxd(VertexList[i + 0].Y);
		V2.Y = FloatToFxd(VertexList[i + 1].Y);
		V3.Y = FloatToFxd(VertexList[i + 2].Y);
		fxd_RasterizeTri(Buffer,
			V1.X, V1.Y, V1.ColorArr,
			V2.X, V2.Y, V2.ColorArr,
			V3.X, V3.Y, V3.ColorArr);
	}
}

internal void DrawIndexedTriangleList(IScreenBuffer *Buffer, Vertex *VertexList, int VertexCount, int *Indices, int IndexCount){
	for(int i = 0; i < (IndexCount & ~2); i += 3){
		int I0, I1, I2;
		I0 = Indices[i + 0];
		I1 = Indices[i + 1];
		I2 = Indices[i + 2];

		struct fxd_Vertex V1, V2, V3;
		memcpy(&V1.Color, &VertexList[I0].Color, sizeof(V1.Color));
		memcpy(&V2.Color, &VertexList[I1].Color, sizeof(V2.Color));
		memcpy(&V3.Color, &VertexList[I2].Color, sizeof(V3.Color));

		V1.X = FloatToFxd(VertexList[I0].X);
		V2.X = FloatToFxd(VertexList[I1].X);
		V3.X = FloatToFxd(VertexList[I2].X);

		V1.Y = FloatToFxd(VertexList[I0].Y);
		V2.Y = FloatToFxd(VertexList[I1].Y);
		V3.Y = FloatToFxd(VertexList[I2].Y);
		fxd_RasterizeTri(Buffer,
			V1.X, V1.Y, V1.ColorArr,
			V2.X, V2.Y, V2.ColorArr,
			V3.X, V3.Y, V3.ColorArr);
	}
}
internal void DrawIndexedTriangleListSSE(IScreenBuffer *Buffer, Vertex *VertexList, int VertexCount, int *Indices, int IndexCount){
	for(int i = 0; i < (IndexCount & ~2); i += 3){
		int I0, I1, I2;
		I0 = Indices[i + 0];
		I1 = Indices[i + 1];
		I2 = Indices[i + 2];

		struct fxd_Vertex V1, V2, V3;
		memcpy(&V1.Color, &VertexList[I0].Color, sizeof(V1.Color));
		memcpy(&V2.Color, &VertexList[I1].Color, sizeof(V2.Color));
		memcpy(&V3.Color, &VertexList[I2].Color, sizeof(V3.Color));

		V1.X = FloatToFxd(VertexList[I0].X);
		V2.X = FloatToFxd(VertexList[I1].X);
		V3.X = FloatToFxd(VertexList[I2].X);

		V1.Y = FloatToFxd(VertexList[I0].Y);
		V2.Y = FloatToFxd(VertexList[I1].Y);
		V3.Y = FloatToFxd(VertexList[I2].Y);
		fxd_RasterizeTriSSE(Buffer,
			V1.X, V1.Y, V1.ColorArr,
			V2.X, V2.Y, V2.ColorArr,
			V3.X, V3.Y, V3.ColorArr);
	}
}
internal void DrawIndexedTriangleListAVX(IScreenBuffer *Buffer, Vertex *VertexList, int VertexCount, int *Indices, int IndexCount){
	for(int i = 0; i < (IndexCount & ~2); i += 3){
		int I0, I1, I2;
		I0 = Indices[i + 0];
		I1 = Indices[i + 1];
		I2 = Indices[i + 2];

		struct fxd_Vertex V1, V2, V3;
		memcpy(&V1.Color, &VertexList[I0].Color, sizeof(V1.Color));
		memcpy(&V2.Color, &VertexList[I1].Color, sizeof(V2.Color));
		memcpy(&V3.Color, &VertexList[I2].Color, sizeof(V3.Color));

		V1.X = FloatToFxd(VertexList[I0].X);
		V2.X = FloatToFxd(VertexList[I1].X);
		V3.X = FloatToFxd(VertexList[I2].X);

		V1.Y = FloatToFxd(VertexList[I0].Y);
		V2.Y = FloatToFxd(VertexList[I1].Y);
		V3.Y = FloatToFxd(VertexList[I2].Y);
		fxd_RasterizeTriAVX(Buffer,
			V1.X, V1.Y, V1.ColorArr,
			V2.X, V2.Y, V2.ColorArr,
			V3.X, V3.Y, V3.ColorArr);
	}
}

internal void ScaleVertexList(struct Vertex *Vertices, int VertexCount, float ScaleX, float ScaleY){
	for(int i = 0; i < VertexCount; i++){
		Vertices[i].X *= ScaleX;
		Vertices[i].Y *= ScaleY;
	}
}

RENDER_FRAME(RenderFrame){
	int &Init = *(int*)MemBuffer;
	*EnableFrameTime = 1;
	GlobalDebugCtx = DebugCTX;
	Assert(Buffer);
	Assert(Buffer->Data);
	if(Input->KeyR){
		Init = 0;
	}
	
	
	if(!Init){

		Init++;
	}

	unsigned BackgroundColor = 0xffa0a0a0;
	SetBackgroundColorAVX(Buffer, BackgroundColor);
	
	struct Vertex Vertices[12] = {
		  0.0f,   0.0f, {255.0f,0.0f, 0.0f, 255.f},
		  0.0f, 100.0f, {255.0f,0.0f, 0.0f, 255.f},
		100.0f,   0.0f, {255.0f,0.0f, 0.0f, 255.f},
		
		  0.0f, 100.0f, {0.0f,255.0f, 0.0f, 255.f},
		100.0f, 100.0f, {0.0f,255.0f, 0.0f, 255.f},
		100.0f,   0.0f, {0.0f,255.0f, 0.0f, 255.f},
		
		100.0f,   0.0f, {0,0.0f, 255.0f, 255.f},
		100.0f, 100.0f, {0,0.0f, 255.0f, 255.f},
		200.0f,   0.0f, {0,0.0f, 255.0f, 255.f},
		
		100.0f, 100.0f, {255.0f,0.0f, 0.0f, 255.f},
		200.0f, 100.0f, {255.0f,0.0f, 0.0f, 255.f},
		200.0f,   0.0f, {255.0f,0.0f, 0.0f, 255.f},
	};
	struct Vertex Vertices2[12] = {
		  0.0f,   0.0f, {255.0f,    0.0f,   0.0f, 255.f},
		  0.0f, 100.0f, {255.0f,    0.0f,   0.0f, 255.f},
		100.0f,   0.0f, {255.0f,    0.0f,   0.0f, 255.f},
		100.0f, 100.0f, {   0.0f, 255.0f,   0.0f, 255.f},
		200.0f,   0.0f, {   0.0f,   0.0f, 255.0f, 255.f},
		200.0f, 100.0f, {255.0f,    0.0f,   0.0f, 255.f},

		  0.0f, 200.0f, {  0.0f,   0.0f, 255.0f, 255.f},
		100.0f, 200.0f, {255.0f,   0.0f,   0.0f, 255.f},
		200.0f, 200.0f, {255.0f,   0.0f,   0.0f, 255.f},
	};

	int Indices[24] = {
		0,1,2,
		1,3,2,
		2,3,4,
		3,5,4,

		1,6,3,
		6,7,3,
		3,7,5,
		7,8,5
	};
	


	ScaleVertexList(Vertices2, ArrayCount(Vertices2), 4, 4);

	DrawIndexedTriangleListAVX(Buffer, Vertices2, ArrayCount(Vertices2), Indices, ArrayCount(Indices));


	




}