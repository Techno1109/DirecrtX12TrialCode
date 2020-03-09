#include "BloomGaussShader.h"
#include "Tool.h"
#include "Application.h"
#include <assert.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib ")

BloomGaussShader::BloomGaussShader(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, float S, int SampleNum, int Priority): PassRenderer(Dev, ShaderPath, Priority, PassRenderer::PassType::SHADER),_S(S), _SampleNum(SampleNum)
{
	Init(ShaderPath);
	CreateWeight();
	BufferFlag[BufferTag::FovBuffer] = true;
	BufferFlag[BufferTag::BloomBuffer] = true;
}

BloomGaussShader::~BloomGaussShader()
{
}

void BloomGaussShader::PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList)
{
	_CmdList->SetDescriptorHeaps(1, _WeightHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(2, _WeightHeap->GetGPUDescriptorHandleForHeapStart());

	_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	////頂点バッファをセット
	_CmdList->IASetVertexBuffers(0, 1, &_VertexBufferView);

	_CmdList->DrawInstanced(4, 1, 0, 0);
}

void BloomGaussShader::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap)
{
}

bool BloomGaussShader::CreateRootSignature()
{
	HRESULT Result = S_OK;

	D3D12_DESCRIPTOR_RANGE Range[3] = {};

	//テクスチャ用
	Range[0].BaseShaderRegister = 0;//0
	Range[0].NumDescriptors = 6;
	Range[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	Range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVはテクスチャ t
	Range[0].RegisterSpace = 0;

	Range[1].BaseShaderRegister = 6;//0
	Range[1].NumDescriptors = 2;
	Range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	Range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVはテクスチャ t
	Range[1].RegisterSpace = 0;

	//ガウスぼかし
	Range[2].BaseShaderRegister = 0;//0
	Range[2].NumDescriptors = 1;
	Range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	Range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;//b
	Range[2].RegisterSpace = 0;

	D3D12_ROOT_PARAMETER Rp[3] = {};

	//マテリアル用
	Rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rp[0].DescriptorTable.NumDescriptorRanges = 1;
	Rp[0].DescriptorTable.pDescriptorRanges = &Range[0];

	Rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rp[1].DescriptorTable.NumDescriptorRanges = 1;
	Rp[1].DescriptorTable.pDescriptorRanges = &Range[1];

	Rp[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	Rp[2].DescriptorTable.NumDescriptorRanges = 1;
	Rp[2].DescriptorTable.pDescriptorRanges = &Range[2];

	D3D12_STATIC_SAMPLER_DESC Sampler[1];
	Sampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Sampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	Sampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	Sampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	Sampler[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	Sampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	Sampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	Sampler[0].MaxAnisotropy = 0;
	Sampler[0].MaxLOD = D3D12_FLOAT32_MAX;
	Sampler[0].MinLOD = 0;
	Sampler[0].ShaderRegister = 0;
	Sampler[0].RegisterSpace = 0;
	Sampler[0].MipLODBias = 0.0f;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
	RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	RootSignatureDesc.NumParameters = 3;
	RootSignatureDesc.pParameters = Rp;
	RootSignatureDesc.NumStaticSamplers = 1;
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

bool BloomGaussShader::CreatePipeLineState()
{
	HRESULT Result = S_OK;

	D3D12_INPUT_ELEMENT_DESC InputLayoutDescs[] = { { "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT	,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
													{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT		,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 } };

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
	PipelineStateDesc.NumRenderTargets = 2;
	PipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	PipelineStateDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//震度ステンシル
	PipelineStateDesc.DepthStencilState.DepthEnable = false;
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

bool BloomGaussShader::Create_Rtv_Srv()
{
	return false;
}


bool BloomGaussShader::CreateWeight()
{
	auto Weight = GetWeight();

	Size WinSize = Application::Instance().GetWindowSize();

	HRESULT Result = S_OK;

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(Weight[0])*Weight.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

	Result = _Dev->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &ResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_WeightBuffer.ReleaseAndGetAddressOf()));


	if (FAILED(Result))
	{
		return false;
	}

	float* MappedWeight = nullptr;

	Result = _WeightBuffer->Map(0, nullptr, (void**)& MappedWeight);

	if (FAILED(Result))
	{
		return false;
	}

	std::copy(Weight.begin(), Weight.end(), MappedWeight);
	_WeightBuffer->Unmap(0, nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC WeightHeapDesc = {};
	WeightHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	WeightHeapDesc.NodeMask = 0;
	WeightHeapDesc.NumDescriptors = 1;
	WeightHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Result = _Dev->CreateDescriptorHeap(&WeightHeapDesc, IID_PPV_ARGS(_WeightHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc = {};

	ViewDesc.BufferLocation = _WeightBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes = _WeightBuffer->GetDesc().Width;
	auto Handle = _WeightHeap->GetCPUDescriptorHandleForHeapStart();
	auto HeapStride = _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_Dev->CreateConstantBufferView(&ViewDesc, Handle);

	return true;
}

std::vector<float> BloomGaussShader::GetWeight()
{
	std::vector<float> ReturnWeight(_SampleNum);

	float TargetOffset = 0.0f;
	float Total = 0.0f;
	for (auto& Value : ReturnWeight) {
		Value = expf(-(TargetOffset*TargetOffset) / (2 * _S*_S));
		Total += Value;
		TargetOffset += 1;
	}

	Total = (Total * 2.0f) - ReturnWeight[0];

	for (auto& Value : ReturnWeight)
	{
		Value /= Total;
	}

	return ReturnWeight;
}
