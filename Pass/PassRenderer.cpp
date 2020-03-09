#include "PassRenderer.h"
#include "Tool.h"
#include "Imgui/imgui.h"
#include <assert.h>
#include <vector>
#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"d3dcompiler.lib ")


PassRenderer::PassRenderer(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, int Priority, PassType Type):_Dev(Dev),_Priority(Priority)
{
	_ShaderPath=ShaderPath;
	ActiveFlag = true;
}


PassRenderer::~PassRenderer()
{
}

void PassRenderer::DrawSetting(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect)
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

void PassRenderer::PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList)
{
	_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	////頂点バッファをセット
	_CmdList->IASetVertexBuffers(0, 1, &_VertexBufferView);

	_CmdList->DrawInstanced(4, 1, 0, 0);
}

bool PassRenderer::Init(std::string ShaderPath)
{
	if (!LoadShader(ShaderPath))
	{
		assert(false);
		return false;
	}

	if (!CreateRootSignature())
	{
		assert(false);
		return false;
	}

	if (!CreatePipeLineState())
	{
		assert(false);
		return false;
	}

	if (!CreatePassPoly())
	{
		assert(false);
		return false;
	}

	for (int i=0;i<BufferTag::MAX;i++)
	{
		BufferFlag[i] = false;
	}

	return true;
}

void PassRenderer::ImGuiDraw()
{
	if (ImGui::TreeNode(_ShaderPath.c_str()))
	{
		ImGui::Checkbox("Active", (bool*)&ActiveFlag);

		ImGuiDrawBase();
		ImGui::TreePop();
	}
}

void PassRenderer::ImGuiDrawBase()
{

}

bool& PassRenderer::GetActiveFlag()
{
	return ActiveFlag;
}

void PassRenderer::PassClearAndSetRendererTarget(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthStencilViewHeap,bool First)
{
	auto HeapPointer = _Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//レンダリング
	AddBarrier(_CmdList,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (First==true)
	{
		_CmdList->OMSetRenderTargets(1, &HeapPointer, false, &_DepthStencilViewHeap->GetCPUDescriptorHandleForHeapStart());
	}
	else
	{
		_CmdList->OMSetRenderTargets(1, &HeapPointer, false,nullptr);
	}
	FLOAT ClsColor[4] = { 0,0, 0,1.0 };
	_CmdList->ClearRenderTargetView(HeapPointer, ClsColor, 0, nullptr);
}

Microsoft::WRL::ComPtr<ID3D12PipelineState>& PassRenderer::GetPipeLineState()
{
	return _PipelineState;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature>& PassRenderer::GetRootSignature()
{
	return _RootSigunature;
}

Microsoft::WRL::ComPtr<ID3D12Resource>& PassRenderer::GetBuffer()
{
	return _Buffer;
}

D3D12_VERTEX_BUFFER_VIEW& PassRenderer::GetVertexBufferView()
{
	return _VertexBufferView;
}

Microsoft::WRL::ComPtr<ID3D12Resource>& PassRenderer::GetVertexBuffer()
{
	return _VertexBuffer;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> PassRenderer::GetRtv_DescriptorHeap()
{
	return _Rtv_DescriptorHeap;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> PassRenderer::GetSrv_DescriptorHeap()
{
	return _Srv_DescriptorHeap;
}


std::array<bool, PassRenderer::BufferTag::MAX>& PassRenderer::GetUseBufferFlag()
{
	return BufferFlag;
}

PassRenderer::PassType & PassRenderer::GetPassType()
{
	return ThisPassType;
}

const int & PassRenderer::GetPriority()
{
	return _Priority;
}



bool PassRenderer::LoadShader(std::string ShaderPath)
{
	HRESULT Result = S_OK;
	Microsoft::WRL::ComPtr <ID3DBlob> E;
	Result = D3DCompileFromFile(GetWideString(ShaderPath).c_str(), nullptr, nullptr, "BasicVS", "vs_5_0", D3DCOMPILE_DEBUG
		| D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_VertexShader, &E);
	if (FAILED(Result))
	{
		std::string CheckString;
		CheckString.resize(E.Get()->GetBufferSize());
		std::copy_n((char*)E.Get()->GetBufferPointer(), E.Get()->GetBufferSize(), CheckString.begin());
		OutputDebugString(CheckString.c_str());
		return false;
	}

	Result = D3DCompileFromFile(GetWideString(ShaderPath).c_str(), nullptr, nullptr, "BasicPS", "ps_5_0", D3DCOMPILE_DEBUG
		| D3DCOMPILE_SKIP_OPTIMIZATION, 0, &_PixelShader, &E);

	if (FAILED(Result))
	{
		std::string CheckString;
		CheckString.resize(E.Get()->GetBufferSize());
		std::copy_n((char*)E.Get()->GetBufferPointer(), E.Get()->GetBufferSize(),CheckString.begin());
		OutputDebugString(CheckString.c_str());
		return false;
	}

	return true;
}

void PassRenderer::AddBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{
	//リソースバリアを設定
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.StateBefore = StateBefore;
	BarrierDesc.Transition.StateAfter = StateAfter;
	BarrierDesc.Transition.pResource = _Buffer.Get();
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	//レンダリング前にバリアを追加
	_CmdList->ResourceBarrier(1, &BarrierDesc);
}

bool PassRenderer::CreatePassPoly()
{
	HRESULT Result = S_OK;

	struct  Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};
	Vertex Vertices[4];

	Vertices[0] = { { -1,-1	,0.0f } , {0,1} };
	Vertices[1] = { { -1,1	,0.0f } , {0,0} };
	Vertices[2] = { { 1	,-1	,0.0f } , {1,1} };
	Vertices[3] = { { 1	,1	,0.0f } , {1,0} };


	D3D12_HEAP_PROPERTIES HeapProp = {};

	HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC ResourceDesc{};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = sizeof(Vertices);
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_VertexBuffer));
	if (FAILED(Result))
	{
		return false;
	}

	//頂点Map作成
	Vertex* _VertexMap = nullptr;

	Result = _VertexBuffer->Map(0, nullptr, (void**)&_VertexMap);
	std::copy(std::begin(Vertices), std::end(Vertices), _VertexMap);
	_VertexBuffer->Unmap(0, nullptr);

	//頂点バッファビュー作成
	_VertexBufferView.BufferLocation = _VertexBuffer->GetGPUVirtualAddress();

	_VertexBufferView.StrideInBytes = sizeof(Vertices[0]);
	_VertexBufferView.SizeInBytes = sizeof(Vertices);

	return true;
}
