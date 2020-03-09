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
	float4 svpos : SV_POSITION;
	float4 pos : POSITION;
};

float4 BasicPS(Out o) : SV_Target
{
	return 	float4(1,1,1,1);
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint weight : WEIGHT, uint instNo : SV_InstanceID)
{
	Out o;

	float w = weight / 100.0f;
	matrix m = BoneMats[boneno.x] * w + BoneMats[boneno.y] * (1 - w);

	pos = mul(m, pos);

	o.pos = mul(World, pos);

	o.svpos = mul(LightCamera, o.pos);

	return o;
}
