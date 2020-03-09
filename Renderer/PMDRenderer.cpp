#include "PMDRenderer.h"
#include <assert.h>
#include <vector>
#include <d3dcompiler.h>
#include <string>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"d3dcompiler.lib ")

PMDRenderer::PMDRenderer(Microsoft::WRL::ComPtr<ID3D12Device>& _dev):_Dev(_dev)
{
}

Microsoft::WRL::ComPtr<ID3D12PipelineState>& PMDRenderer::GetPipeLineState()
{
	return _PipelineState;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature>& PMDRenderer::GetRootSignature()
{
	return _RootSigunature;
}


PMDRenderer::~PMDRenderer()
{
}

bool PMDRenderer::Init()
{
	if (!LoadShader())
	{
		return false;
	}

	if (!CreateRootSignature())
	{
		return false;
	}

	if (!CreatePipeLineState())
	{
		return false;
	}

	return true;
}



bool PMDRenderer::CreateRootSignature()
{
	HRESULT Result = S_OK;

	D3D12_DESCRIPTOR_RANGE Range[6] = {};

	//座標変換
	Range[0].BaseShaderRegister = 1;//1
	Range[0].NumDescriptors = 1;
	Range[0].OffsetInDescriptorsFromTableStart = 0;
	Range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//b
	Range[0].RegisterSpace = 0;

	//マテリアル用
	Range[1].BaseShaderRegister = 0;//0
	Range[1].NumDescriptors = 1;
	Range[1].OffsetInDescriptorsFromTableStart = 0;
	Range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//b
	Range[1].RegisterSpace = 0;

	//テクスチャ用
	Range[2].BaseShaderRegister = 0;//0
	Range[2].NumDescriptors = 4;
	Range[2].OffsetInDescriptorsFromTableStart = 1;
	Range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVはテクスチャ t
	Range[2].RegisterSpace = 0;

	////座標変換
	Range[3].BaseShaderRegister = 2;//2
	Range[3].NumDescriptors = 1;
	Range[3].OffsetInDescriptorsFromTableStart = 0;
	Range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//b
	Range[3].RegisterSpace = 0;

	//ボーン座標
	Range[4].BaseShaderRegister = 3;//2
	Range[4].NumDescriptors = 1;
	Range[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	Range[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//b
	Range[4].RegisterSpace = 0;

	//テクスチャ用
	Range[5].BaseShaderRegister = 4;//0
	Range[5].NumDescriptors = 2;
	Range[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	Range[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVはテクスチャ t
	Range[5].RegisterSpace = 0;

	D3D12_ROOT_PARAMETER Rp[5] = {};

	//マテリアル用
	Rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rp[0].DescriptorTable.NumDescriptorRanges = 2;
	Rp[0].DescriptorTable.pDescriptorRanges = &Range[1];

	//座標変換
	Rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	Rp[1].DescriptorTable.NumDescriptorRanges = 1;
	Rp[1].DescriptorTable.pDescriptorRanges = &Range[0];

	////座標変換
	Rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	Rp[2].DescriptorTable.NumDescriptorRanges = 1;
	Rp[2].DescriptorTable.pDescriptorRanges = &Range[3];

	////ボーン関連の行列
	Rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	Rp[3].DescriptorTable.NumDescriptorRanges = 1;
	Rp[3].DescriptorTable.pDescriptorRanges = &Range[4];

	Rp[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rp[4].DescriptorTable.NumDescriptorRanges = 1;
	Rp[4].DescriptorTable.pDescriptorRanges = &Range[5];

	D3D12_STATIC_SAMPLER_DESC Sampler[2];
	Sampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Sampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Sampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Sampler[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	Sampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	Sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	Sampler[0].MaxAnisotropy = 0;
	Sampler[0].MaxLOD = D3D12_FLOAT32_MAX;
	Sampler[0].MinLOD = 0;
	Sampler[0].ShaderRegister = 0;
	Sampler[0].RegisterSpace = 0;
	Sampler[0].MipLODBias = 0.0f;

	Sampler[1] = Sampler[0];
	Sampler[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	Sampler[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	Sampler[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	Sampler[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	Sampler[1].ShaderRegister = 1;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	RootSignatureDesc.NumParameters = 5;
	RootSignatureDesc.pParameters = Rp;
	RootSignatureDesc.NumStaticSamplers = 2;
	RootSignatureDesc.pStaticSamplers = Sampler;

	Result = D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &_Signature, &_Error);

	if (FAILED(Result))
	{
		return false;
	}

	Result = _Dev->CreateRootSignature(0, _Signature->GetBufferPointer(), _Signature->GetBufferSize(), IID_PPV_ARGS(_RootSigunature.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

bool PMDRenderer::CreatePipeLineState()
{
	HRESULT Result = S_OK;

	D3D12_INPUT_ELEMENT_DESC InputLayoutDescs[] = { { "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT	,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
													{ "NORMAL"	,0,DXGI_FORMAT_R32G32B32_FLOAT	,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
													{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT		,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
													{ "BONENO"	,0,DXGI_FORMAT_R16G16_UINT		,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
													{ "WEIGHT"	,0,DXGI_FORMAT_R8_UINT			,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 } };

	D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateDesc = {};
	//シェーダ系
	PipelineStateDesc.VS.pShaderBytecode = _VertexShader->GetBufferPointer();
	PipelineStateDesc.VS.BytecodeLength = _VertexShader->GetBufferSize();

	PipelineStateDesc.PS.pShaderBytecode = _PixelShader->GetBufferPointer();
	PipelineStateDesc.PS.BytecodeLength = _PixelShader->GetBufferSize();
	//ルートシグネチャ、頂点レイアウト
	PipelineStateDesc.pRootSignature = _RootSigunature.Get();
	PipelineStateDesc.InputLayout.pInputElementDescs = InputLayoutDescs;
	PipelineStateDesc.InputLayout.NumElements = _countof(InputLayoutDescs);

	//レンダーターゲット
	PipelineStateDesc.NumRenderTargets = 6;
	PipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PipelineStateDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PipelineStateDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PipelineStateDesc.RTVFormats[3] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PipelineStateDesc.RTVFormats[4] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PipelineStateDesc.RTVFormats[5] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//震度ステンシル
	PipelineStateDesc.DepthStencilState.DepthEnable = true;
	PipelineStateDesc.DepthStencilState.StencilEnable = false;
	PipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	PipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	PipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//ラスタライザ
	PipelineStateDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	PipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	PipelineStateDesc.RasterizerState.FrontCounterClockwise = false;
	PipelineStateDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	PipelineStateDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	PipelineStateDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	PipelineStateDesc.RasterizerState.DepthClipEnable = true;
	PipelineStateDesc.RasterizerState.MultisampleEnable = false;
	PipelineStateDesc.RasterizerState.AntialiasedLineEnable = false;
	PipelineStateDesc.RasterizerState.ForcedSampleCount = 0;
	PipelineStateDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	//BlendState
	D3D12_RENDER_TARGET_BLEND_DESC RenderTargetBlendDesc{};
	RenderTargetBlendDesc.BlendEnable = true;
	RenderTargetBlendDesc.LogicOpEnable = false;
	RenderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	RenderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ONE;
	RenderTargetBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	RenderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	RenderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	RenderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;

	RenderTargetBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	RenderTargetBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		PipelineStateDesc.BlendState.RenderTarget[i] = RenderTargetBlendDesc;
	}
	PipelineStateDesc.BlendState.AlphaToCoverageEnable = false;
	PipelineStateDesc.BlendState.IndependentBlendEnable = false;

	//その他
	PipelineStateDesc.NodeMask = 0;
	PipelineStateDesc.SampleDesc.Count = 1;
	PipelineStateDesc.SampleDesc.Quality = 0;
	PipelineStateDesc.SampleMask = 0xffffffff;
	PipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;


	Result = _Dev->CreateGraphicsPipelineState(&PipelineStateDesc, IID_PPV_ARGS(_PipelineState.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

bool PMDRenderer::LoadShader()
{
	HRESULT Result = S_OK;
	Result = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "BasicVS", "vs_5_0", D3DCOMPILE_DEBUG
		| D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_VertexShader, nullptr);
	if (FAILED(Result))
	{
		return false;
	}

	Result = D3DCompileFromFile(L"shader.hlsl", nullptr, nullptr, "BasicPS", "ps_5_0", D3DCOMPILE_DEBUG
		| D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_PixelShader, nullptr);

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}
