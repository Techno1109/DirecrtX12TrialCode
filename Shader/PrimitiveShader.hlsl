SamplerState Smp : register(s0);
SamplerState SmpToon:register(s1);
cbuffer materialBuffer:register(b0)
{
	float4 Diffuse;
	float Power;
	float3 Specular;
	float4 Ambient;
};

Texture2D Tex	: register(t0);	//通常テクスチャ
Texture2D Sph	: register(t1);	//スフィアマップ(乗算しよう)
Texture2D Spa	: register(t2);	//スフィアマップ(加算しよう)
Texture2D Toon	: register(t3);	//トゥーンテクスチャ

Texture2D<float> Depth : register(t4);
Texture2D<float> LightDepth : register(t5);
cbuffer TransBuffer : register(b1)
{
	matrix Camera;
	float3 Eye;
	float3 LightPos;
	matrix Shadow;
	matrix LightCamera;
    matrix Projection;
    matrix InProjection;
};

matrix World : register(b2);

cbuffer bones : register(b3)
{
	matrix BoneMats[512];
}

struct Out
{
	float4 svpos :SV_POSITION;
	float4 pos	:POSITION;
	float4 tpos : POSITION1;
	float4 normal:NORMAL;
};

struct PixelStruct
{
    float4 Color : SV_TARGET0;
    float4 Normal : SV_TARGET;
    float4 HighLum : SV_TARGET2;
    float4 ShlinkLum : SV_TARGET3;
    float4 Fov : SV_TARGET4;
    float4 ShlinkFov : SV_TARGET5;
};

PixelStruct BasicPS(Out o) : SV_Target
{

    PixelStruct PixelReturn;
    PixelReturn.Normal = float4(o.normal.rgb, 1);

    PixelReturn.HighLum = float4(0, 0, 0, 0);
    PixelReturn.ShlinkLum = float4(0, 0, 0, 0);
    PixelReturn.ShlinkFov = float4(0, 0, 0, 0);

	float3 light = normalize(float3(1 ,-1,1));
	float bright = dot(o.normal, -light);

	float3 PosFromLight = (o.tpos.xyz / o.tpos.w);
	float2 ShadowUv = (PosFromLight + float2(1, -1))*float2(0.5f, -0.5f);
	float ShadowZ = LightDepth.Sample(Smp, ShadowUv);
	float ShadowWeight = 1.0f;

	if (PosFromLight.z > ShadowZ + 0.001f)
	{
		ShadowWeight = 0.5f;
	}
    PixelReturn.Color = PixelReturn.Fov = float4(0.5, 1, 0.5, 1) * float4(float3(ShadowWeight, ShadowWeight, ShadowWeight) * float3(bright, bright, bright), 1);
    return PixelReturn;
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION, float4 normal : NORMAL)
{
	Out o;
	o.svpos = mul(Camera, pos);
	o.normal = mul(Camera, float4(normal.xyz, 0));
	o.tpos = mul(LightCamera, pos);
	return o;
}

//影用
float4 ShadowPS(Out o) : SV_Target
{
	return 	float4(1,1,1,1);
}


//影用 
Out ShadowVS(float4 pos : POSITION, float4 normal : NORMAL)
{
	Out o;
	o.pos = pos;
	o.svpos = mul(LightCamera, o.pos);

	return o;
}