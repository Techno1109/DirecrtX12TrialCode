#define FXAA_PC 1
#define FXAA_HLSL_5 1
#include"FXAA.hlsl"


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

//AO
Texture2D<float4> SSAOTex : register(t8);

cbuffer Signals : register(b0)
{
    uint OnDebug;
    uint NormalWindow;
    uint BloomWindow;
    uint ShrinkBloomWindow;
    uint FovWindow;
    uint ShrinkFovWindow;
    uint SSAOWindow;
    uint DepthWindow;
    uint LightDepthWindow;
    uint OutLineFlag;
    uint FXAAFlag;
    uint BloomFlag;
    uint SSAOFlag;
}

struct Out
{
	float4 svpos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
    float4 pos : POSITION0;
};

struct PixelStruct
{
    float4 Color : SV_TARGET0;
};

    PixelStruct BasicPS(Out o) : SV_Target
{

	PixelStruct PixelReturn;
    PixelReturn.Color = Tex.Sample(Smp, o.uv);

	float w, h, miplevel;
	Tex.GetDimensions(0, w, h, miplevel);

	float dx = 1 / w;
	float dy = 1 / h;

	float4 OutColor = Tex.Sample(Smp, o.uv);

    if (FXAAFlag)
    {
        FxaaTex InputFXAATex = { Smp, Tex };
        OutColor = float4(FxaaPixelShader(
		o.uv, // FxaaFloat2 pos,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsolePosPos,
		InputFXAATex, // FxaaTex tex,
		InputFXAATex, // FxaaTex fxaaConsole360TexExpBiasNegOne,
		InputFXAATex, // FxaaTex fxaaConsole360TexExpBiasNegTwo,
		float2(dx, dy), // FxaaFloat2 fxaaQualityRcpFrame,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsoleRcpFrameOpt,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsoleRcpFrameOpt2,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f), // FxaaFloat4 fxaaConsole360RcpFrameOpt2,
		0.75f, // FxaaFloat fxaaQualitySubpix,
		0.166f, // FxaaFloat fxaaQualityEdgeThreshold,
		0.0833f, // FxaaFloat fxaaQualityEdgeThresholdMin,
		0.0f, // FxaaFloat fxaaConsoleEdgeSharpness,
		0.125f, // FxaaFloat fxaaConsoleEdgeThreshold,
		0.05f, // FxaaFloat fxaaConsoleEdgeThresholdMin,
		FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f) // FxaaFloat fxaaConsole360ConstDir,
	    ).rgb, OutColor.a);
    }
    
    if (SSAOFlag)
    {
        OutColor *= float4(SSAOTex.Sample(Smp, o.uv).rrr, 1);
    }
    
    if (OutLineFlag)
    {
        float OutLine = Depth.Sample(Smp, o.uv) * 4 -
		Depth.Sample(Smp, o.uv + float2(dx, 0)) -
		Depth.Sample(Smp, o.uv + float2(-dx, 0)) -
		Depth.Sample(Smp, o.uv + float2(0, dy)) -
		Depth.Sample(Smp, o.uv + float2(0, -dy));
        OutLine = 1 - step(0.001, OutLine);
        OutColor *= OutLine;
    }
       
	
    if (BloomFlag)
    {
        float4 BloomAccum = float4(0, 0, 0, 0);
        float2 UvSize = float2(0.5, 0.5);
        float2 UvOfst = float2(0, 0);
	
        for (int i = 0; i < 4; i++)
        {
            BloomAccum += ShrinkBloomTex.Sample(Smp, o.uv * UvSize + UvOfst);
            UvOfst.y += UvSize.y;
            UvSize *= 0.5f;
        }
        
        OutColor += BloomTex.Sample(Smp, o.uv) * 0.1f + saturate(BloomAccum) * 0.1f;
    }

    
    PixelReturn.Color = OutColor;
    
    if (!OnDebug)
    {
        return PixelReturn;
    }
        if (o.uv.x <= 0.2f && o.uv.y <= 0.2f && DepthWindow)
        {
            float4 OutColor = float4(0, 0, 0, 0);
            OutColor = Depth.Sample(Smp, o.uv * 5);
            OutColor = 1 - pow(OutColor, 10);
            PixelReturn.Color = float4(OutColor.rgb, 1);
        }
        else if (o.uv.x <= 0.2f && (o.uv.y > 0.2f && o.uv.y <= 0.4f) && LightDepthWindow)
        {
            float4 OutColor = float4(0, 0, 0, 0);
            OutColor = LightDepth.Sample(Smp, (o.uv - float2(0.0f, 0.2f)) * 5);
            OutColor = 1 - OutColor;
            PixelReturn.Color = float4(OutColor.rgb, 1);
        }
        else if (o.uv.x < 0.2 && (o.uv.y > 0.4f && o.uv.y <= 0.6f) && NormalWindow)
        { //法線出力 
            PixelReturn.Color = NormalTex.Sample(Smp, (o.uv - float2(0, 0.4)) * 5);
        }
        else if (o.uv.x < 0.2 && (o.uv.y > 0.6f && o.uv.y <= 0.8f) && BloomWindow)
        { //ブルーム
            PixelReturn.Color = BloomTex.Sample(Smp, (o.uv - float2(0, 0.6)) * 5);
        }
        else if (o.uv.x < 0.2 && (o.uv.y > 0.8f && o.uv.y <= 1.0f) && ShrinkBloomWindow)
        { //縮小バッファ
            PixelReturn.Color = ShrinkBloomTex.Sample(Smp, (o.uv - float2(0, 0.8)) * 5);
        }
        else if (o.uv.x > 0.2 && o.uv.x < 0.4 && o.uv.y <= 0.2f && FovWindow)
        {
            PixelReturn.Color = FovTex.Sample(Smp, (o.uv - float2(0.2, 0.0)) * 5);
        }
        else if (o.uv.x > 0.2 && o.uv.x < 0.4 && (o.uv.y > 0.2f && o.uv.y <= 0.4f) && ShrinkFovWindow)
        {
            PixelReturn.Color = ShrinkFovTex.Sample(Smp, (o.uv - float2(0.2, 0.2)) * 5);
        }
        if (o.uv.x > 0.2 && o.uv.x < 0.4 && (o.uv.y > 0.4f && o.uv.y <= 0.6f) && SSAOWindow)
        {
            float Tmp = SSAOTex.Sample(Smp, (o.uv - float2(0.2, 0.4)) * 5);
            PixelReturn.Color = float4(Tmp, Tmp, Tmp, 1);
        }
    
    return PixelReturn;
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION,float2 uv : TEXCOORD)
{
	Out o;
	o.svpos = pos;
	o.uv = uv;
    o.pos = pos;
	return o;
}