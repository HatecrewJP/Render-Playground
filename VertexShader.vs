

#define STRINGIFY(x) #x

char *VSShaderText = STRINGIFY(

struct VS_Input{
	float2 Position : SV_Position;
	uint Radius : PSIZE;
};

struct VS_Output{
	float4 Position : SV_Position;
};


VS_Output main(VS_Input Input){
	VS_Output Output;
	Output.Position.xy = Input.Position.xy;
	Output.Position.zw = float2(0,1);
	return Output;
}

);
