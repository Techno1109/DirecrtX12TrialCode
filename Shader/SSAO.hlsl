SamplerState Smp:register(s0);
Texture2D<float4> NormalTex : register(t1);
Texture2D<float4> ShrinkFovTex : register(t5);
//深度値実験用
Texture2D<float> Depth : register(t6);
Texture2D<float> LightDepth : register(t7);

cbuffer TransBuffer : register(b0)
{
	matrix Camera;
	float3 Eye;
	float3 LightPos;
	matrix Shadow;
	matrix LightCamera;
    matrix Projection;
    matrix InProjection;
};


struct Out
{
	float4 svpos : SV_POSITION;
	float4 normal : NORMAL;
	float2 uv : TEXCOORD;
};

float random(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

//SSAO(乗算用の明度のみ情報を返せればよい)
float BasicPS(Out o) : SV_Target
{
    float dp = Depth.Sample(Smp, o.uv); //現在の UV の深度
    float w, h, miplevels;
    Depth.GetDimensions(0, w, h, miplevels);
    float dx = 1.0f / w;
    float dy = 1.0f / h;
 //SSAO
 //元の座標を復元する
    float4 respos = mul(InProjection, float4(o.uv * float2(2, -2) + float2(-1, 1), dp, 1));
    respos.xyz = respos.xyz / respos.w;
    float div = 0.0f;
    float ao = 0.0f;
    float3 norm = normalize((NormalTex.Sample(Smp, o.uv).xyz * 2) - 1);
    const int trycnt = 256;
    const float radius = 0.8f;
    if (dp < 1.0f)
    {
        for (int i = 0; i < trycnt; ++i)
        {
            float rnd1 = random(float2(i * dx, i * dy)) * 2 - 1;
            float rnd2 = random(float2(rnd1, i * dy)) * 2 - 1;
            float rnd3 = random(float2(rnd2, rnd1)) * 2 - 1;
            float3 omega = normalize(float3(rnd1, rnd2, rnd3));
            omega = normalize(omega);
            float dt = dot(norm, omega);
            float sgn = sign(dt);
            omega *= sign(dt);
            float4 rpos = mul(Projection, float4(respos.xyz + omega * radius, 1));
            rpos.xyz /= rpos.w;
            dt *= sgn;
            div += dt;
            ao += step(Depth.Sample(Smp,(rpos.xy + float2(1, -1)) * float2(0.5f, -0.5f)), rpos.z) * dt;
        }
        ao /= div;
    }

    return 1.0f - ao;
}


//頂点シェーダ 
Out BasicVS(float4 pos : POSITION,float2 uv : TEXCOORD)
{
	Out o;
	o.svpos = pos;
	o.uv = uv;
	return o;
}