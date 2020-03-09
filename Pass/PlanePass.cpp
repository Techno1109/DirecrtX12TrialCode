#include "PlanePass.h"
#include "Application.h"
#include "GaussPass.h"
#include "Imgui/imgui.h"
#include <assert.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "Tool.h"
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib ")


PlanePass::PlanePass(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, int Priority):PassRenderer(Dev,ShaderPath,Priority, PassRenderer::PassType::SHADER)
{
	Init(ShaderPath);
	CreateSignalBuffer();

	Signals.OnDebug = 0;
	Signals.NormalWindow=0;
	Signals.BloomWindow=0;
	Signals.ShrinkBloomWindow=0;
	Signals.FovWindow=0;
	Signals.ShrinkFovWindow=0;
	Signals.SSAOWindow = 0;
	Signals.Depth=0;
	Signals.LightDepth=0;
	Signals.OutLine=1;
	Signals.FXAA=1;
	Signals.Bloom=0;
	Signals.SSAO = 0;
	BufferFlag[BufferTag::MainBuffer] = true;
}


PlanePass::~PlanePass()
{
}

void PlanePass::DrawSetting(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect)
{
	//ルートシグネチャをセット
	_CmdList->SetGraphicsRootSignature(_RootSigunature.Get());

	//ビューポートをセット
	_CmdList->RSSetViewports(1, &_ViewPort);

	//シザーをセット
	_CmdList->RSSetScissorRects(1, &_ScissorRect);

	//パイプラインをセット
	_CmdList->SetPipelineState(_PipelineState.Get());
}

void PlanePass::PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList)
{
	SetSignal();
	_CmdList->SetDescriptorHeaps(1, _SignalHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(2, _SignalHeap->GetGPUDescriptorHandleForHeapStart());

	_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	////頂点バッファをセット
	_CmdList->IASetVertexBuffers(0, 1, &_VertexBufferView);

	_CmdList->DrawInstanced(4, 1, 0, 0);
}

void PlanePass::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap)
{
	//ルートシグネチャをセット
	_CmdList->SetGraphicsRootSignature(_RootSigunature.Get());

	//ビューポートをセット
	_CmdList->RSSetViewports(1, &_ViewPort);

	//シザーをセット
	_CmdList->RSSetScissorRects(1, &_ScissorRect);

	//パイプラインをセット
	_CmdList->SetPipelineState(_PipelineState.Get());

	_CmdList->SetDescriptorHeaps(1, _Srv_DescriptorHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(0, _Srv_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	_CmdList->SetDescriptorHeaps(1, _DepthBufferViewHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(1, _DepthBufferViewHeap->GetGPUDescriptorHandleForHeapStart());

	_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	////頂点バッファをセット
	_CmdList->IASetVertexBuffers(0, 1, &_VertexBufferView);

	_CmdList->DrawInstanced(4, 1, 0, 0);

}

void PlanePass::PassClearAndSetRendererTarget(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthStencilViewHeap, bool First)
{
	auto HeapPointer = _Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto Multi = HeapPointer.ptr + _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto Bloom = HeapPointer.ptr + _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)*2;

	D3D12_CPU_DESCRIPTOR_HANDLE Rtvs[3] = {HeapPointer,Multi,Bloom};

	//レンダリング
	AddBarrier(_CmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (First == true)
	{
		_CmdList->OMSetRenderTargets(3, Rtvs, false, &_DepthStencilViewHeap->GetCPUDescriptorHandleForHeapStart());
	}
	else
	{
		_CmdList->OMSetRenderTargets(3, Rtvs, false, nullptr);
	}
	FLOAT ClsColor[4] = { 0,0, 0,1.0 };
	for (auto& Rt:Rtvs)
	{
		_CmdList->ClearRenderTargetView(Rt, ClsColor, 0, nullptr);
	}
}

void PlanePass::ImGuiDrawBase()
{
	
	ImGui::Checkbox("Debug", (bool*)&Signals.OnDebug);
	ImGui::Checkbox("OutLine", (bool*)&Signals.OutLine);
	ImGui::Checkbox("FXAA", (bool*)&Signals.FXAA);
	ImGui::Checkbox("Bloom", (bool*)&Signals.Bloom);
	ImGui::Checkbox("SSAO", (bool*)&Signals.SSAO);
	if (ImGui::TreeNode("DebugMenu"))
	{
		ImGui::Checkbox("NormalTex", (bool*)&Signals.NormalWindow);
		ImGui::Checkbox("BloomTex", (bool*)&Signals.BloomWindow);
		ImGui::Checkbox("ShrinkBloomTex", (bool*)&Signals.ShrinkBloomWindow);
		ImGui::Checkbox("FovTex", (bool*)&Signals.FovWindow);
		ImGui::Checkbox("ShrinkFovTex", (bool*)&Signals.ShrinkFovWindow);
		ImGui::Checkbox("SSAOWindow", (bool*)&Signals.SSAOWindow);
		ImGui::Checkbox("Depth", (bool*)&Signals.Depth);
		ImGui::Checkbox("LightDepth", (bool*)&Signals.LightDepth);

		ImGui::TreePop();

	}
}


bool PlanePass::LoadShader(std::string ShaderPath)
{
	HRESULT Result = S_OK;
	Microsoft::WRL::ComPtr <ID3DBlob> E;
	Result = D3DCompileFromFile(GetWideString(ShaderPath).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BasicVS", "vs_5_0", D3DCOMPILE_DEBUG
		| D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_VertexShader, &E);
	if (FAILED(Result))
	{
		return false;
	}

	Result = D3DCompileFromFile(GetWideString(ShaderPath).c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "BasicPS", "ps_5_0", D3DCOMPILE_DEBUG
		| D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_PixelShader, &E);

	if (FAILED(Result))
	{
		std::string CheckString;
		CheckString.resize(E.Get()->GetBufferSize());
		std::copy_n((char*)E.Get()->GetBufferPointer(), E.Get()->GetBufferSize(), CheckString.begin());
		OutputDebugString(CheckString.c_str());
		return false;
	}

	return true;
}
void PlanePass::SetSignal()
{
	MappedSignal->OnDebug = Signals.OnDebug;
	MappedSignal->NormalWindow =Signals.NormalWindow ;
	MappedSignal->BloomWindow =Signals.BloomWindow;
	MappedSignal->ShrinkBloomWindow =Signals.ShrinkBloomWindow;
	MappedSignal->FovWindow =Signals.FovWindow;
	MappedSignal->ShrinkFovWindow =Signals.ShrinkFovWindow;
	MappedSignal->Depth =Signals.Depth;
	MappedSignal->LightDepth =Signals.LightDepth;
	MappedSignal->SSAOWindow = Signals.SSAOWindow;
	MappedSignal->OutLine= Signals.OutLine;
	MappedSignal->FXAA=Signals.FXAA;
	MappedSignal->Bloom=Signals.Bloom;
	MappedSignal->SSAO = Signals.SSAO;
}
bool PlanePass::CreateSignalBuffer()
{
	HRESULT Result = S_OK;

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(ShaderSignal), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

	Result = _Dev->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &ResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_SignalBuffer.ReleaseAndGetAddressOf()));


	if (FAILED(Result))
	{
		return false;
	}

	 MappedSignal=nullptr;

	Result = _SignalBuffer->Map(0, nullptr, (void**)& MappedSignal);

	if (FAILED(Result))
	{
		return false;
	}

	SetSignal();

	D3D12_DESCRIPTOR_HEAP_DESC SignalHeapDesc = {};
	SignalHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SignalHeapDesc.NodeMask = 0;
	SignalHeapDesc.NumDescriptors = 1;
	SignalHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Result = _Dev->CreateDescriptorHeap(&SignalHeapDesc, IID_PPV_ARGS(_SignalHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc = {};

	ViewDesc.BufferLocation = _SignalBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes = _SignalBuffer->GetDesc().Width;
	auto Handle = _SignalHeap->GetCPUDescriptorHandleForHeapStart();
	auto HeapStride = _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_Dev->CreateConstantBufferView(&ViewDesc, Handle);

	return true;
}

bool PlanePass::CreateRootSignature()
{
	HRESULT Result = S_OK;

	D3D12_DESCRIPTOR_RANGE Range[4] = {};

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

	Range[2].BaseShaderRegister = 0;//0
	Range[2].NumDescriptors = 1;
	Range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	Range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	Range[2].RegisterSpace = 0;

	Range[3].BaseShaderRegister = 8;//0
	Range[3].NumDescriptors = 1;
	Range[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	Range[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//SRVはテクスチャ t
	Range[3].RegisterSpace = 0;

	D3D12_ROOT_PARAMETER Rp[4] = {};

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

	Rp[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Rp[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	Rp[3].DescriptorTable.NumDescriptorRanges = 1;
	Rp[3].DescriptorTable.pDescriptorRanges = &Range[3];

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
	RootSignatureDesc.NumParameters = 4;
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

bool PlanePass::CreatePipeLineState()
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
	PipelineStateDesc.NumRenderTargets = 1;
	PipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

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
	RenderTargetBlendDesc.BlendEnable = false;
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

bool PlanePass::Create_Rtv_Srv()
{
	return true;
}
void PlanePass::AddBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{

}
