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
    uint Counter;
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
    float4 Fov : SV_TARGET4;
};

float SDFCircle2D(float2 xy, float2 center, float r)
{
    return length(fmod(xy, r) - r / 2) - r / 2;
}

float SDFCircle3D(float3 pos, float divider,float r)
{
    return length(fmod(pos, divider) - divider / 2) - r;
}

float opSubtraction(float d1, float d2)
{
    return max(-d1, d2);
}

float SDOctahedron(float3 p,float divider, float s)
{
    p = fmod(p, divider) - divider / 2;
    p = abs(p);
    float m = p.x + p.y + p.z - s;
    float3 q;
    if (3.0 * p.x < m)
        q = p.xyz;
    else if (3.0 * p.y < m)
        q = p.yzx;
    else if (3.0 * p.z < m)
        q = p.zxy;
    else
        return m * 0.57735027;
    
    float k = clamp(0.5 * (q.z - q.y + s), 0.0, s);
    return length(float3(q.x, q.y - s + k, q.z - k));
}

float opSmoothUnion(float d1, float d2, float k)
{
    float h = max(k - abs(d1 - d2), 0.0);
    return min(d1, d2) - h * h * 0.25 / k;
}

float opSmoothSubtraction(float d1, float d2, float k)
{
    float h = max(k - abs(-d1 - d2), 0.0);
    return max(-d1, d2) + h * h * 0.25 / k;
}
PixelStruct BasicPS(Out o) : SV_Target
{

	PixelStruct PixelReturn;
    PixelReturn.Color =PixelReturn.Fov= Tex.Sample(Smp, o.uv);
    
	float w, h, miplevel;
	Tex.GetDimensions(0, w, h, miplevel);

	float dx = 1 / w;
	float dy = 1 / h;
  
    if (NormalTex.Sample(Smp, o.uv).a == 0)
    {
        float2 aspect = float2(w / h, 1);
        float3 Eye = float3(0, 0, -2.5);
        float3 Tpos = float3(o.pos.xy * aspect, 0);
        float3 Ray = normalize(Tpos - Eye);
        float Rsph = 3;
        
        float4 RayMarchOutColor = float4(0.0, 0.0, 0.0, 1);
       
        for (int i = 0; i < 64; ++i)
        {
            float Octlen = SDOctahedron(abs(Eye), Rsph * 2, (Rsph + 0.5) / 2);
            float Spherelen = SDFCircle3D(abs(Eye), Rsph * 2, Rsph / 2);
            float len = opSmoothSubtraction(Octlen, Spherelen, 0.5);
            Eye += Ray * len;
            if (len < 0.001f)
            {
                RayMarchOutColor.rgb = (float) (64 - i) / 64.0f;
                break;
            }
        }
        float3 Color = float3(o.uv.x, 0.1, o.uv.y);
        PixelReturn.Color = PixelReturn.Fov = float4(RayMarchOutColor.rgb * Color, 1);
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