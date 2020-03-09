SamplerState Smp:register(s0);
Texture2D<float4> Tex : register(t0);
Texture2D<float4> NormalMap : register(t1);
struct Out
{
	float4 svpos : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 BasicPS(Out o) : SV_Target
{
	float4 Dist = NormalMap.Sample(Smp,o.uv);
	float2 Offset = Dist.rg*float2(1, -1) + float2(-0.5, 0.5);
	float4 Color = Tex.Sample(Smp, o.uv+Offset*0.3f);
	return 	Color;
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Out o;
	o.svpos = pos;
	o.uv = uv;
	return o;
}