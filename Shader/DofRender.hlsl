
SamplerState Smp:register(s0);
Texture2D<float4> Tex : register(t0);
Texture2D<float4> NormalTex : register(t1);
Texture2D<float4> BloomTex : register(t2);
Texture2D<float4> ShrinkBloomTex : register(t3);
Texture2D<float4> FovTex : register(t4);
Texture2D<float4> ShrinkFovTex : register(t5);
//深度値実験用
Texture2D<float> Depth : register(t6);
Texture2D<float> LightDepth : register(t7);
struct Out
{
	float4 svpos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct PixelStruct
{
    float4 Color : SV_TARGET0;
};

PixelStruct BasicPS(Out o) : SV_Target
{
	PixelStruct PixelReturn;
	
    float DepthDiff = abs(Depth.Sample(Smp, float2(0.5, 0.5)) - Depth.Sample(Smp, o.uv));
    DepthDiff = pow(DepthDiff, 0.5f);
    float2 FovUvSize = float2(0.5, 0.5);
    float2 FovUvOfst = float2(0, 0);
    float T = DepthDiff * 4;
    float No = 0;
    T = modf(T, No);
    float4 RetCol[2];
    
    RetCol[0] = Tex.Sample(Smp, o.uv);
    
    if (No == 0)
    {
        RetCol[1] = ShrinkFovTex.Sample(Smp, o.uv * FovUvSize + FovUvOfst);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            if (i - No < 0)
            {
                continue;
            }
            RetCol[i - No] = ShrinkFovTex.Sample(Smp, o.uv * FovUvSize + FovUvOfst);
            FovUvOfst.y += FovUvSize.y;
            FovUvSize *= 0.5f;
            if (i - No > 1)
            {
                break;
            }
        }
    }
    PixelReturn.Color = lerp(RetCol[0], RetCol[1], float4(T, T, T, 1));
    return PixelReturn;
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION,float2 uv : TEXCOORD)
{
	Out o;
	o.svpos = pos;
	o.uv = uv;
	return o;
}