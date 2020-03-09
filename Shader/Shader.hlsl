SamplerState Smp : register(s0);
SamplerState SmpToon:register(s1);
cbuffer materialBuffer:register(b0)
{
	float4 Diffuse;
	float Power;
	float3 Specular;
	float4 Ambient;
};

Texture2D Tex	: register(t0);	//�ʏ�e�N�X�`��
Texture2D Sph	: register(t1);	//�X�t�B�A�}�b�v(��Z���悤)
Texture2D Spa	: register(t2);	//�X�t�B�A�}�b�v(���Z���悤)
Texture2D Toon	: register(t3);	//�g�D�[���e�N�X�`��

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
	float4 tpos : POSITION1;
	float4 normal:NORMAL;
	float2 uv :TEXCOORD;
	float w : WEIGHT;
	uint instNo:SV_InstanceID;
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

	float3 Light =  normalize(-1* float3(-1, 2, -3));
	float3 EyeRay = normalize(o.pos - Eye);

	float3 RLight = reflect(Light, o.normal);
	float3 REye = reflect(EyeRay, o.normal);

	float2 SphUV = (o.normal.xy*float2(1.0f, -1.0f) + float2(1.0f, 1.0f)) / 2.0f;
	float2 NormalUV = (o.normal.xy + float2(1, -1))*float2(0.5, -0.5);

	//�X�y�L�����v�Z
	float SpecularB = saturate(dot(RLight, -EyeRay));
	if(SpecularB>0&&Power>0)
	{
		SpecularB = pow(SpecularB, Power);
	}

	//�e��f�B�t���[�Y�擾
	float DiffuseB = saturate(dot(-Light,o.normal));
	float4 ToonDiffuse = Toon.Sample(SmpToon, float2(0, 1.0 - DiffuseB));

	//�������牺���e��e�N�X�`���ƃ}�e���A�����f
	float Brightness = saturate(dot(o.normal, -Light));
	float4 TexColor = Tex.Sample(Smp, o.uv);
	float4 MaterialCol = float4(Brightness, Brightness, Brightness, 1);
	float4 SphColor = Sph.Sample(Smp, SphUV);
	float4 SpaColor = Spa.Sample(Smp, SphUV);
	float4 Return= saturate(ToonDiffuse*Diffuse*TexColor*SphColor + saturate(Ambient)*0.3 + saturate(SpaColor*TexColor + float4(Specular, 1) *SpecularB));
	//�������牺���Z���t�V���h�E���f
	float3 PosFromLight = (o.tpos.xyz / o.tpos.w);
	float2 ShadowUv = (PosFromLight + float2(1, -1))*float2(0.5f, -0.5f);
	float ShadowZ = LightDepth.Sample(Smp, ShadowUv);
	float ShadowWeight = 1.0f;

	if (PosFromLight.z>ShadowZ+0.001f)
	{
		ShadowWeight = 0.5f;
	}

	//�o��
	PixelReturn.Normal.rgb = float3((o.normal.xyz + 1.0f) / 2.0f);
	PixelReturn.Normal.a = 1;
    
    PixelReturn.Color = PixelReturn.Fov = float4(float3(ShadowWeight, ShadowWeight, ShadowWeight) * Return.rgb, Return.a);
    
	float Y = dot(float3(0.3f, 0.6f, 0.1f), PixelReturn.Color);
	float Bo =Y> 0.99f ? PixelReturn.Color : 0.0f;
	PixelReturn.HighLum = float4(Bo,Bo,Bo,1);

	return 	PixelReturn;
}


//���_�V�F�[�_ 
Out BasicVS(float4 pos : POSITION, float4 normal: NORMAL,float2 uv:TEXCOORD, min16uint2 boneno : BONENO,min16uint weight:WEIGHT,uint instNo:SV_InstanceID)
{
	Out o;

	float w = weight / 100.0f;
	matrix m = BoneMats[boneno.x] * w + BoneMats[boneno.y] * (1 - w);

	pos = mul(m, pos);

	o.pos	= mul(World, pos);
	o.svpos = mul(Camera,o.pos);
	o.tpos = mul(LightCamera, o.pos);
	o.normal= mul(World, float4(normal.xyz,0));
	o.uv = uv;
	o.w = weight / 100.0f;
	o.instNo = instNo;
	return o;
}
