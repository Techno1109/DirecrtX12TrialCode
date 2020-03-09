//ブルーム用のぼかし

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
    float4 HighLum : SV_TARGET2;
    float4 Fov : SV_TARGET4;
};

cbuffer Weight:register(b0)
{
	float4 Wgts[2];
};

struct Out
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

PixelStruct BasicPS(Out o) : SV_Target
{
	PixelStruct PixelReturn;
    
	float W,H,Level;
	BloomTex.GetDimensions(0, W, H, Level);
	float2 OffsetX = float2(14 / W,0);
	float4 LumRet = BloomTex.Sample(Smp, o.uv)*Wgts[0][0];
    float4 FovRet = FovTex.Sample(Smp, o.uv) * Wgts[0][0];

	for (int i = 1; i < 8; i++)
	{
		float2 Offset1 = float2(0,0);
		Offset1.x = (-1 + (i - 1)*-2) / W;
		float2 Offset2 = float2(0, 0);
		Offset2.x = (-1 + (8 - i)*-2) / W;
		LumRet += Wgts[i / 4][i % 4] * (BloomTex.Sample(Smp, o.uv + Offset1) + BloomTex.Sample(Smp, o.uv + Offset2 + OffsetX));
        FovRet += Wgts[i / 4][i % 4] * (FovTex.Sample(Smp, o.uv + Offset1) + FovTex.Sample(Smp, o.uv + Offset2 + OffsetX));
    };
    
	PixelReturn.HighLum = LumRet;
    PixelReturn.Fov = FovRet;
	return 	PixelReturn;
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Out o;
	o.svpos = pos;
	o.uv = uv;
	return o;
}