SamplerState Smp : register(s0);
Texture2D<float4> Tex : register(t0);
Texture2D<float4> NormalTex : register(t1);
Texture2D<float4> BloomTex : register(t2);
Texture2D<float4> ShrinkBloomTex : register(t3);
Texture2D<float4> FovTex : register(t4);
Texture2D<float4> ShrinkFovTex : register(t5);
//深度値実験用
Texture2D<float> Depth : register(t6);
Texture2D<float> LightDepth : register(t7);

struct PixelStruct
{
    float4 ShlinkLum : SV_TARGET3;
    float4 ShlinkFov : SV_TARGET5;
};

struct Out
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

PixelStruct BasicPS(Out o) : SV_Target
{
	PixelStruct PixelReturn;
	float UvBase = 0.5f;
	
	//1/2
	if (o.uv.x <= (1.0 * UvBase) && o.uv.y <= (1.0 * UvBase))
	{
		PixelReturn.ShlinkLum = BloomTex.Sample(Smp, o.uv * (1.0/UvBase));
        PixelReturn.ShlinkFov = FovTex.Sample(Smp, o.uv * (1.0 / UvBase));
		return PixelReturn;
	}
	
	UvBase *= 0.5f;
	//1/4(UvBase * (1.0 / UvBase))
	if (o.uv.x <= (1.0 * UvBase) && o.uv.y <= 0.75 && o.uv.y > 0.5)
	{
		PixelReturn.ShlinkLum = BloomTex.Sample(Smp, (o.uv - float2(0.0f, 0.5)) * (1.0 / UvBase));
        PixelReturn.ShlinkFov = FovTex.Sample(Smp, (o.uv - float2(0.0f, 0.5)) * (1.0 / UvBase));
		return PixelReturn;
	}
	
	UvBase *= 0.5f;
	//1/8
	if (o.uv.x <= (1.0 * UvBase) && o.uv.y <= (0.875) && o.uv.y > 0.75)
	{
		PixelReturn.ShlinkLum = BloomTex.Sample(Smp, (o.uv - float2(0.0f, 0.75)) * (1.0 / UvBase));
        PixelReturn.ShlinkFov = FovTex.Sample(Smp, (o.uv - float2(0.0f, 0.75)) * (1.0 / UvBase));
		return PixelReturn;
	}
	
	UvBase *= 0.5f;
	//1/16
	if (o.uv.x <= (1.0 * UvBase) && o.uv.y <= 0.9375 && o.uv.y > 0.875)
	{
		PixelReturn.ShlinkLum = BloomTex.Sample(Smp, (o.uv - float2(0.0f, 0.875)) * (1.0 / UvBase));
        PixelReturn.ShlinkFov = FovTex.Sample(Smp, (o.uv - float2(0.0f, 0.875)) * (1.0 / UvBase));
		return PixelReturn;
	}
    
	return PixelReturn;
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Out o;
	o.svpos = pos;
	o.uv = uv;
	return o;
}