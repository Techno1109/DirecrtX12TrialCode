#include "Dx12Wrapper.h"
#include "Application.h"
#include "Tool.h"
#include "TexLoader.h"

#include "PassRenderer.h"
#include "ShadowMapPass.h"
#include "BloomGaussShader.h"
#include "PrimitiveRenderer.h"
#include "PrimitiveBase.h"
#include "PrimitiveCone.h"
#include "PrimitivePlate.h"

#include "PlanePass.h"
#include "SSAOPass.h"
#include "BloomGaussShader.h"
#include "DofPass.h"
#include "ShlinkRenderer.h"
#include "RayMarchingPass.h"

#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx12.h"
#include "Imgui/imgui_impl_win32.h"

#include <assert.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib ")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"LLGI.lib ")
#pragma comment(lib,"Effekseer.lib ")
#pragma comment(lib,"EffekseerRendererDX12.lib ")

using namespace DirectX;


Dx12Wrapper::Dx12Wrapper(HWND windowH) :_WindowH(windowH),Eye(0, 30, -40),Target(0, 20, 0),UpperVec(0, 1, 0)
{

	//DXGI
	_DxgiFactory=nullptr;
	_DxgiSwapChain = nullptr;

	//デバイス
	_Dev = nullptr;

	//コマンド系
	//コマンドリスト確保用オブジェクト
	_CmdAlloc = nullptr;
	//コマンドリスト本体
	_CmdList = nullptr;
	//コマンドリストのキュー
	_CmdQueue = nullptr;

	//フェンスオブジェクト
	_Fence = nullptr;

	//フェンス値
	_FenceValue=0;

	//ディスクリプタヒープ
	_Rtv_DescriptorHeap = nullptr;

	 //座標変換定数バッファ
	 _TransformCB=nullptr;

	 //座標変換ヒープ
	 _TransformHeap=nullptr;


	//ルートシグネチャ
	_RootSigunature=nullptr;
	//パイプラインステート
	_PipelineState = nullptr;

	//ビューポート
	SetViewPort();

	Offset.x = 0;
	Offset.y = 0;
	Offset.z = 0;

	TotalActivePass = 0;
}


Dx12Wrapper::~Dx12Wrapper()
{
	_EffekseerRenderer->Release();
	_EffekseerManager->Release();
	_EffekseerMemoryPool->Release();
	_EffekseerCommandList->Release();
	_Effect->Release();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool Dx12Wrapper::Init()
{
	ID3D12Debug* Debug;
	D3D12GetDebugInterface(IID_PPV_ARGS(&Debug));
	Debug->EnableDebugLayer();
	Debug->Release();

	HRESULT Result = S_OK;

	D3D_FEATURE_LEVEL Levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	for (auto level : Levels)
	{
		if (SUCCEEDED( D3D12CreateDevice( nullptr,level,IID_PPV_ARGS(_Dev.ReleaseAndGetAddressOf()) ) )	)
		{
			break;
		}
	}


	if (_Dev == nullptr)
	{
		return false;
	}

	if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(_DxgiFactory.ReleaseAndGetAddressOf()))))
	{
		return false;
	}

	//コマンドキューを作成

	D3D12_COMMAND_QUEUE_DESC CmdQueueDesc{};
	CmdQueueDesc.NodeMask = 0;
	CmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	CmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	CmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	

	if (FAILED( _Dev->CreateCommandQueue(&CmdQueueDesc,IID_PPV_ARGS(_CmdQueue.ReleaseAndGetAddressOf()))))
	{
		return false;
	}


	//スワップチェインを作成
	Size WindowSize = Application::Instance().GetWindowSize();

	DXGI_SWAP_CHAIN_DESC1 SwDesc = {};
	SwDesc.BufferCount = 2;
	SwDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	SwDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwDesc.Flags = 0;
	SwDesc.Width = WindowSize.Width;
	SwDesc.Height = WindowSize.Height;
	SwDesc.SampleDesc.Count = 1;
	SwDesc.SampleDesc.Quality = 0;
	SwDesc.Stereo = false;
	SwDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwDesc.Scaling = DXGI_SCALING_STRETCH;

	Result = _DxgiFactory->CreateSwapChainForHwnd(_CmdQueue.Get(),_WindowH,&SwDesc,nullptr,nullptr,(IDXGISwapChain1**)(_DxgiSwapChain.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}


	//フェンスを作成
	Result = _Dev->CreateFence(_FenceValue,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(_Fence.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}


	Result = CoInitializeEx(0, COINIT_MULTITHREADED);

	if (FAILED(Result))
	{
		return false;
	}

	if (!CreateCommandList())
	{
		return false;
	}

	if (!CreateRenderTargetView())
	{
		return false;
	}


	if (!CreateDepthBuffer())
	{
		return false;
	}


	if (!CreateTransformConstantBuffer())
	{
		return false;
	}

	if (!CreatePrimitives())
	{
		return false;
	}

	if (!Create_Rtv_Srvs())
	{
		return false;
	}

	if (!Create_SSAO_Rtv_Srv())
	{
		return false;
	}

	_ShadowPass = std::make_unique<ShadowMapPass>(_Dev, "ShadowShader.hlsl", 0);
	_PrimitiveRender= std::make_unique<PrimitiveRenderer>(_Dev, "PrimitiveShader.hlsl", 0);
	_SSAORender = std::make_unique<SSAOPass>(_Dev, "SSAO.hlsl", 0);

	//ここから下で画面効果用のレンダリングパスを設定する

	RendererPasses.emplace_back(new RayMarchingPass(_Dev, "RayMarching.hlsl", 0));
	RendererPasses.emplace_back(new BloomGaussShader(_Dev, "BloomGaussX.hlsl", 8.0f, 8.0f, 2));
	RendererPasses.emplace_back(new BloomGaussShader(_Dev, "BloomGaussY.hlsl", 8.0f, 8.0f, 2));

	RendererPasses.emplace_back(new ShlinkRenderer(_Dev, "ShrinkBuffer.hlsl", 3)); 
	RendererPasses.emplace_back(new DofPass(_Dev, "DofRender.hlsl", 4));

	if (RendererPasses.size()>0)
	{
		std::sort(RendererPasses.begin(), RendererPasses.end(),
			[](std::shared_ptr<PassRenderer> a, std::shared_ptr<PassRenderer> b) {return a->GetPriority() < b->GetPriority(); });
	}
	RendererPasses.emplace_back(new PlanePass(_Dev, "Plane.hlsl", 1000));


	//Effekseer初期化
	EffekseerInit();


	//Imgui
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	auto GUIResult = ImGui_ImplWin32_Init(_WindowH);
	if (!GUIResult)
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	_Dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(_ImGuiHeap.ReleaseAndGetAddressOf()));

	GUIResult = ImGui_ImplDX12_Init(_Dev.Get(),2,DXGI_FORMAT_R8G8B8A8_UNORM,
								 _ImGuiHeap.Get(),_ImGuiHeap->GetCPUDescriptorHandleForHeapStart(),
								 _ImGuiHeap->GetGPUDescriptorHandleForHeapStart());

	if (!GUIResult)
	{
		return false;
	}

	//DXTK
	_Gmemory = std::make_shared <DirectX::GraphicsMemory>(_Dev.Get());

	//SpriteBatch 初期化
	DirectX::ResourceUploadBatch resUploadBatch(_Dev.Get());
	resUploadBatch.Begin();
	DirectX::RenderTargetState rtState(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
	DirectX::SpriteBatchPipelineStateDescription pd(rtState);
	_SpriteBatch = std::make_shared<DirectX::SpriteBatch>(_Dev.Get(), resUploadBatch, pd);

	_Dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(_DxtkHeap.ReleaseAndGetAddressOf()));

	//ここでSpriteFontを指定します
	//各自で用意してください。
	_SpriteFont = std::make_shared < DirectX::SpriteFont>(_Dev.Get(),
		resUploadBatch,
		L"Font/meiryo.spritefont",
		_DxtkHeap->GetCPUDescriptorHandleForHeapStart(),
		_DxtkHeap->GetGPUDescriptorHandleForHeapStart());

	auto future = resUploadBatch.End(_CmdQueue.Get());
	WaitWithFence();
	future.wait();
	_SpriteBatch->SetViewport(_ViewPort);
	return true;
}

bool Dx12Wrapper::Create_Rtv_Srvs()
{
	HRESULT Result = S_OK;
	D3D12_DESCRIPTOR_HEAP_DESC DescripterHeapDesc{};
	DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescripterHeapDesc.NodeMask = 0;
	DescripterHeapDesc.NumDescriptors = 6;

	auto WindowSize = Application::Instance().GetWindowSize();
	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, WindowSize.Width, WindowSize.Height);
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	FLOAT Color[4] = { 0.0,0.0, 0.0,1.0 };
	auto _1stPathClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, Color);

	for (int Num=0;Num<2;Num++)
	{
		DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		Result = _Dev->CreateDescriptorHeap(&DescripterHeapDesc, IID_PPV_ARGS(BufferHeaps[Num]._Rtv_DescriptorHeap.ReleaseAndGetAddressOf()));

		if (FAILED(Result))
		{
			return false;
		}

		DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		Result = _Dev->CreateDescriptorHeap(&DescripterHeapDesc, IID_PPV_ARGS(BufferHeaps[Num]._Srv_DescriptorHeap.ReleaseAndGetAddressOf()));

		if (FAILED(Result))
		{
			return false;
		}


		for (int i=0;i<PassRenderer::BufferTag::MAX;i++)
		{
			Result = _Dev->CreateCommittedResource(&HeapProp,
				D3D12_HEAP_FLAG_NONE,
				&ResourceDesc,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				&_1stPathClearValue,
				IID_PPV_ARGS(Buffers[Num][i].ReleaseAndGetAddressOf()));

			if (FAILED(Result))
			{
				return false;
			}
		}

		auto RtvHandle = BufferHeaps[Num]._Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		auto Offset = _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (int i = 0; i < PassRenderer::BufferTag::MAX; i++)
		{
			_Dev->CreateRenderTargetView(Buffers[Num][i].Get(), nullptr, RtvHandle);
			RtvHandle.ptr += Offset;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Format = ResourceDesc.Format;
		SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SrvDesc.Texture2D.MipLevels = 1;

		auto SrvHandle = BufferHeaps[Num]._Srv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		auto SrvOffset = _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (int i = 0; i < PassRenderer::BufferTag::MAX; i++)
		{
			_Dev->CreateShaderResourceView(Buffers[Num][i].Get(), &SrvDesc, SrvHandle);
			SrvHandle.ptr += SrvOffset;
		}
	}
	return true;
}

bool Dx12Wrapper::Create_SSAO_Rtv_Srv()
{
	HRESULT Result = S_OK;
	D3D12_DESCRIPTOR_HEAP_DESC DescripterHeapDesc{};
	DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescripterHeapDesc.NodeMask = 0;
	DescripterHeapDesc.NumDescriptors = 1;

	auto WindowSize = Application::Instance().GetWindowSize();
	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, WindowSize.Width, WindowSize.Height);
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	FLOAT Color[4] = { 1.0,1.0, 1.0,1.0 };
	auto _1stPathClearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R32_FLOAT, Color);

	DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	Result = _Dev->CreateDescriptorHeap(&DescripterHeapDesc, IID_PPV_ARGS(SSAODatas._Rtv_DescriptorHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Result = _Dev->CreateDescriptorHeap(&DescripterHeapDesc, IID_PPV_ARGS(SSAODatas._Srv_DescriptorHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}


	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&_1stPathClearValue,
		IID_PPV_ARGS(SSAODatas._Buffer.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	auto RtvHandle = SSAODatas._Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	_Dev->CreateRenderTargetView(SSAODatas._Buffer.Get(), nullptr, RtvHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Format = ResourceDesc.Format;
	SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels = 1;

	auto SrvHandle = SSAODatas._Srv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	_Dev->CreateShaderResourceView(SSAODatas._Buffer.Get(), &SrvDesc, SrvHandle);
	return true;
}

void Dx12Wrapper::DrawSSAO()
{
	auto HeapPointer = SSAODatas._Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//レンダリング
	AddBarrier(SSAODatas._Buffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	//リソースバリアを設定
	_CmdList->OMSetRenderTargets(1, &HeapPointer, false, nullptr);

	float ClsColor[4] = { 1,1,1,1 };
	_CmdList->ClearRenderTargetView(HeapPointer, ClsColor, 0, nullptr);

	_SSAORender->DrawSetting(_CmdList, _ViewPort, _ScissorRect);

	_CmdList->SetDescriptorHeaps(1, BufferHeaps[0]._Srv_DescriptorHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(0, BufferHeaps[0]._Srv_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	_CmdList->SetDescriptorHeaps(1, _DepthBufferHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(1, _DepthBufferHeap->GetGPUDescriptorHandleForHeapStart());

	_CmdList->SetDescriptorHeaps(1, _TransformHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(2, _TransformHeap->GetGPUDescriptorHandleForHeapStart());

	_SSAORender->PassDraw(_CmdList);

	AddBarrier(SSAODatas._Buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
}

void Dx12Wrapper::AddBarrier(int PassNum, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.StateBefore = StateBefore;
	BarrierDesc.Transition.StateAfter = StateAfter;
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	for (int i=0;i<PassRenderer::BufferTag::MAX;i++)
	{
		BarrierDesc.Transition.pResource = Buffers[PassNum][i].Get();
		//レンダリング前にバリアを追加
		_CmdList->ResourceBarrier(1, &BarrierDesc);
	}
}

void Dx12Wrapper::PassClearAndSetRendererTarget(int Num, bool First)
{
	auto HeapPointer = BufferHeaps[Num]._Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto Multi = HeapPointer.ptr + _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto Bloom = HeapPointer.ptr + _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 2;
	auto ShrinkBloom = HeapPointer.ptr + _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 3;
	auto Fov = HeapPointer.ptr + _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 4;
	auto ShrinkFov = HeapPointer.ptr + _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * 5;
	D3D12_CPU_DESCRIPTOR_HANDLE Rtvs[6] = { HeapPointer,Multi,Bloom,ShrinkBloom,Fov,ShrinkFov};

	//レンダリング
	AddBarrier(Num, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	//リソースバリアを設定
	if (First)
	{
		_CmdList->OMSetRenderTargets(6, Rtvs, false, &_DepthStencilViewHeap->GetCPUDescriptorHandleForHeapStart());
	}
	else
	{
		_CmdList->OMSetRenderTargets(6, Rtvs, false, nullptr);
	}

	float BlackColor[4] = { 0.0,0.0,0.0,1};
	float ClsColor[4]	= { 0.0,0.0,0.0,0 };
	int Counter = 0;
	for (auto& Rt : Rtvs)
	{
		if (Counter==2 || Counter==3)
		{
			_CmdList->ClearRenderTargetView(Rt, BlackColor, 0, nullptr);
		}
		else
		{
			_CmdList->ClearRenderTargetView(Rt, ClsColor, 0, nullptr);
		}
		Counter++;
	}
}

void Dx12Wrapper::EffekseerInit()
{
	auto WinSize = Application::Instance().GetWindowSize();
	auto Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	_EffekseerRenderer = EffekseerRendererDX12::Create(_Dev.Get(), _CmdQueue.Get(), 2, &Format, 1, true, true, 2000);
	_EffekseerManager = Effekseer::Manager::Create(2000);

	//描画用インスタンスから各種描画機能を設定
	_EffekseerManager->SetSpriteRenderer(_EffekseerRenderer->CreateSpriteRenderer());
	_EffekseerManager->SetRibbonRenderer(_EffekseerRenderer->CreateRibbonRenderer());
	_EffekseerManager->SetRingRenderer(_EffekseerRenderer->CreateRingRenderer());
	_EffekseerManager->SetTrackRenderer(_EffekseerRenderer->CreateTrackRenderer());
	_EffekseerManager->SetModelRenderer(_EffekseerRenderer->CreateModelRenderer());

	//後から独自拡張してもいいかも。
	//描画用インスタンスからテクスチャ読み込み機能を指定
	_EffekseerManager->SetTextureLoader(_EffekseerRenderer->CreateTextureLoader());
	_EffekseerManager->SetModelLoader(_EffekseerRenderer->CreateModelLoader());

	//メモリプール
	_EffekseerMemoryPool = EffekseerRendererDX12::CreateSingleFrameMemoryPool(_EffekseerRenderer.Get());

	//コマンドリスト作成
	_EffekseerCommandList = EffekseerRendererDX12::CreateCommandList(_EffekseerRenderer.Get(), _EffekseerMemoryPool.Get());

	//コマンドリストを設定
	_EffekseerRenderer->SetCommandList(_EffekseerCommandList.Get());

	_EffekseerManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);


	//投影行列設定
	_EffekseerRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovLH(XM_PIDIV4, static_cast<float>(WinSize.Width) / static_cast<float>(WinSize.Height), 1.0f, 1000.0f));

	//カメラ行列指定
	_EffekseerRenderer->SetCameraMatrix(Effekseer::Matrix44().LookAtLH(Effekseer::Vector3D(Eye.x+Offset.x, Eye.y+Offset.y, Eye.z+Offset.z),
																		Effekseer::Vector3D(Target.x + Offset.x, Target.y + Offset.y, Target.z + Offset.z),
																		Effekseer::Vector3D(0, 1, 0)));

	//エフェクト読み込み
	_Effect=Effekseer::Effect::Create(_EffekseerManager.Get(), (const EFK_CHAR*)L"Effect/Laser/laser.efk");


}

void Dx12Wrapper::EffekseerDraw()
{
	_EffekseerManager->Update();

	_EffekseerMemoryPool->NewFrame();

	EffekseerRendererDX12::BeginCommandList(_EffekseerCommandList.Get(), _CmdList.Get());
	_EffekseerRenderer->BeginRendering();
	_EffekseerManager->Draw();
	_EffekseerRenderer->EndRendering();
	EffekseerRendererDX12::EndCommandList(_EffekseerCommandList.Get());
}


bool Dx12Wrapper::CreatePrimitives()
{
	Primitives.emplace_back(new PrimitiveCone(_Dev, 6, 10, 5, XMFLOAT3(0, 0, 0)));
	Primitives.emplace_back(new PrimitivePlate(_Dev,50,50,XMFLOAT3(0,0,0)));
	return true;
}

bool Dx12Wrapper::DrawPrimitiveShapes()
{
	_PrimitiveRender->Draw(_CmdList, _ViewPort, _ScissorRect, _DepthBufferHeap);

	_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& PrimitiveData:Primitives)
	{
		_CmdList->IASetIndexBuffer(&PrimitiveData->GetPrimitiveIndexBV());

		//頂点バッファをセット
		_CmdList->IASetVertexBuffers(0, 1, &PrimitiveData->GetPrimitiveVertexBV());

		//座標
		_CmdList->SetDescriptorHeaps(1, _TransformHeap.GetAddressOf());
		_CmdList->SetGraphicsRootDescriptorTable(1, _TransformHeap->GetGPUDescriptorHandleForHeapStart());

		_CmdList->SetDescriptorHeaps(1, _DepthBufferHeap.GetAddressOf());
		_CmdList->SetGraphicsRootDescriptorTable(4, _DepthBufferHeap->GetGPUDescriptorHandleForHeapStart());

		_CmdList->DrawIndexedInstanced(PrimitiveData->GetPrimitiveIndexBV().SizeInBytes / sizeof(uint16_t), 1, 0, 0, 0);
	}
	return true;
}

bool Dx12Wrapper::CreateRenderTargetView()
{
	HRESULT Result = S_OK;

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc ;

	Result = _DxgiSwapChain->GetDesc1(&SwapChainDesc);

	if (FAILED(Result))
	{
		return false;
	}

	//表示画面メモリ確保(デスクリプタヒープ作成)
	D3D12_DESCRIPTOR_HEAP_DESC DescripterHeapDesc{};
	DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DescripterHeapDesc.NodeMask = 0;
	DescripterHeapDesc.NumDescriptors = SwapChainDesc.BufferCount;

	Result = _Dev->CreateDescriptorHeap(&DescripterHeapDesc, IID_PPV_ARGS(_Rtv_DescriptorHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	_BackBuffers.resize(SwapChainDesc.BufferCount);

	int DescripterSize = _Dev->GetDescriptorHandleIncrementSize(DescripterHeapDesc.Type);

	D3D12_CPU_DESCRIPTOR_HANDLE DescripterHandle = _Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC RtvDesc = {};
	RtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	RtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//レンダーターゲットビューをヒープに紐づけ
	for (int i = 0;i<_BackBuffers.size();i++)
	{
		Result = _DxgiSwapChain->GetBuffer(i,IID_PPV_ARGS(&_BackBuffers[i]));

		if (FAILED(Result))
		{
			break;
		}

		_Dev->CreateRenderTargetView(_BackBuffers[i].Get(),nullptr,DescripterHandle);
		DescripterHandle.ptr += DescripterSize;
	}

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

bool Dx12Wrapper::CreateCommandList()
{
	HRESULT Result = S_OK;

	//コマンドアロケータ作成
	Result = _Dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_CmdAlloc.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	//コマンドリスト作成
	Result = _Dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _CmdAlloc.Get(), nullptr, IID_PPV_ARGS(_CmdList.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

bool Dx12Wrapper::CreateDepthBuffer()
{
	HRESULT Result = S_OK;

	auto WindowSize = Application::Instance().GetWindowSize();

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, WindowSize.Width, WindowSize.Height);
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//クリアバリュー
	D3D12_CLEAR_VALUE _DepthClearValue;
	_DepthClearValue.DepthStencil.Depth = 1.0f;
	_DepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	Result = _Dev->CreateCommittedResource(	&HeapProp,
											D3D12_HEAP_FLAG_NONE,
											&ResourceDesc,
											D3D12_RESOURCE_STATE_DEPTH_WRITE,
											&_DepthClearValue,
											IID_PPV_ARGS(_DepthBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	ResourceDesc.Width = 1024;
	ResourceDesc.Height = 1024;

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&_DepthClearValue,
		IID_PPV_ARGS(_LightDepthBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC _DSVHeapDesc = {};
	_DSVHeapDesc.NumDescriptors = 2;
	_DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	_DSVHeapDesc.NodeMask = 0;


	Result = _Dev->CreateDescriptorHeap(&_DSVHeapDesc, IID_PPV_ARGS(_DepthStencilViewHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
	DSVDesc.ViewDimension= D3D12_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DSVDesc.Flags = D3D12_DSV_FLAG_NONE;

	auto Handle = _DepthStencilViewHeap->GetCPUDescriptorHandleForHeapStart();
	_Dev->CreateDepthStencilView(_DepthBuffer.Get(),&DSVDesc,Handle);

	Handle.ptr += _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	_Dev->CreateDepthStencilView(_LightDepthBuffer.Get(), &DSVDesc, Handle);

	//テスト用
	D3D12_DESCRIPTOR_HEAP_DESC DescripterHeapDesc{};
	DescripterHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	DescripterHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DescripterHeapDesc.NodeMask = 0;
	DescripterHeapDesc.NumDescriptors = 2;

	Result = _Dev->CreateDescriptorHeap(&DescripterHeapDesc, IID_PPV_ARGS(_DepthBufferHeap.ReleaseAndGetAddressOf()));
	if (FAILED(Result))
	{
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Texture2D.MipLevels = 1;
	auto SrvHandle = _DepthBufferHeap->GetCPUDescriptorHandleForHeapStart();
	_Dev->CreateShaderResourceView(_DepthBuffer.Get(), &SrvDesc, SrvHandle);

	SrvHandle.ptr += _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	_Dev->CreateShaderResourceView(_LightDepthBuffer.Get(), &SrvDesc, SrvHandle);
	return true;
}


bool Dx12Wrapper::SetViewPort()
{
	Size WindowSize = Application::Instance().GetWindowSize();

	_ViewPort.TopLeftX = 0; 
	_ViewPort.TopLeftY = 0; 
	_ViewPort.Width = WindowSize.Width; 
	_ViewPort.Height = WindowSize.Height; 
	_ViewPort.MaxDepth = 1.0f;
	_ViewPort.MinDepth = 0.0f;

	_ScissorRect.left = 0;
	_ScissorRect.top = 0;
	_ScissorRect.right = WindowSize.Width;
	_ScissorRect.bottom = WindowSize.Height;

	return true;
}

bool Dx12Wrapper::CreateTransformConstantBuffer()
{
	Size WinSize = Application::Instance().GetWindowSize();

	HRESULT Result = S_OK;

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(TransformMatrix), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));


	Result=_Dev->CreateCommittedResource(&HeapProp,D3D12_HEAP_FLAG_NONE,&ResDesc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(_TransformCB.ReleaseAndGetAddressOf()));


	if (FAILED(Result))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC TransHeapDesc = {};
	TransHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	TransHeapDesc.NodeMask = 0;
	TransHeapDesc.NumDescriptors = 2;
	TransHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Result=_Dev->CreateDescriptorHeap(&TransHeapDesc, IID_PPV_ARGS(_TransformHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}


	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc = {};
	ViewDesc.BufferLocation = _TransformCB->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes = _TransformCB->GetDesc().Width;
	
	_Dev->CreateConstantBufferView(&ViewDesc, _TransformHeap->GetCPUDescriptorHandleForHeapStart());


	//XMMATRIXは行列。
	//初期化はXMMatrixIdentityで初期化する

	MappedMatrix = {};
	_TransformCB->Map(0, nullptr, (void**)&MappedMatrix);

	return true;
}

DirectX::XMMATRIX Dx12Wrapper::GetCurrentCameraMatrix()
{
	Size WinSize = Application::Instance().GetWindowSize();

	//XMVECTORはSIMD対象であり、XMFLOAT等はSIMD対象外
	XMMATRIX ViewMat = XMMatrixLookAtLH(XMLoadFloat3(&Eye)+XMLoadFloat3(&Offset), XMLoadFloat3(&Target) + XMLoadFloat3(&Offset), XMLoadFloat3(&UpperVec));

	XMMATRIX ProjMat = XMMatrixPerspectiveFovLH(XM_PIDIV4, static_cast<float>(WinSize.Width) / static_cast<float>(WinSize.Height), 1.0f, 1000);
	MappedMatrix->Projection = ProjMat;
	XMVECTOR Det;
	MappedMatrix->InProjection = XMMatrixInverse(&Det, MappedMatrix->Projection);

	return  ViewMat*ProjMat;
}

void Dx12Wrapper::SetFov(float Angle)
{
	_Fov = Angle;
}

void Dx12Wrapper::SetEyePosition(float x, float y, float z)
{
	Eye.x = x;
	Eye.y = y;
	Eye.z = z;
}

void Dx12Wrapper::SetTargetPosition(float x, float y, float z)
{
	Target.x = x;
	Target.y = y;
	Target.z = z;
}

void Dx12Wrapper::MoveFov(float Angle)
{
	_Fov += Angle;
}


void Dx12Wrapper::MoveEyePosition(float x, float y, float z)
{
	Eye.x += x;
	Eye.y += y;
	Eye.z += z;
}

void Dx12Wrapper::MoveTargetPosition(float x, float y, float z)
{
	Target.x += x;
	Target.y += y;
	Target.z += z;
}

void Dx12Wrapper::MoveOffset(float x, float y, float z)
{
	Offset.x += x;
	Offset.y += y;
	Offset.z += z;
}

void Dx12Wrapper::ShadowDrawSetting()
{
	//MappedMatrixを計算
	MappedMatrix->Camera = GetCurrentCameraMatrix();
	MappedMatrix->Eye = XMFLOAT3(Eye.x +Offset.x,Eye.y +Offset.y,Eye.z + Offset.z);

	auto Plane = XMFLOAT4(0, 1, 0, 0);
	MappedMatrix->LightPos = XMFLOAT3(-1, 2, -3);

	auto EyeMatrix = XMLoadFloat3(&Eye);
	auto TargetMatrix = XMLoadFloat3(&Target);
	auto OffsetMatrix = XMLoadFloat3(&Offset);
	auto UpperVecMatrix = XMLoadFloat3(&UpperVec);

	auto CameraEyeMatrix = XMLoadFloat3(&Eye)*2;

	float CameraArmLengh = XMVector3Length(XMVectorSubtract(TargetMatrix+ OffsetMatrix, CameraEyeMatrix+ OffsetMatrix)).m128_f32[0];

	auto LightPos = TargetMatrix+ OffsetMatrix + XMVector3Normalize(XMLoadFloat3(&MappedMatrix->LightPos))*CameraArmLengh;

	//カメラ行列指定
	_EffekseerRenderer->SetCameraMatrix(Effekseer::Matrix44().LookAtLH(Effekseer::Vector3D(Eye.x+Offset.x, Eye.y + Offset.y, Eye.z+Offset.z),
		Effekseer::Vector3D(Target.x + Offset.x, Target.y + Offset.y, Target.z + Offset.z),
		Effekseer::Vector3D(0, 1, 0)));

	MappedMatrix->LightCamera = XMMatrixLookAtLH(LightPos, TargetMatrix+OffsetMatrix, UpperVecMatrix)*XMMatrixOrthographicLH(120,120, 1.0f, 1000.0f);

	MappedMatrix->Shadow = XMMatrixShadow(XMLoadFloat4(&Plane), XMLoadFloat3(&MappedMatrix->LightPos));

	_ShadowPass->PassClearAndSetRendererTarget(_CmdList, _DepthStencilViewHeap,false);
}

void Dx12Wrapper::ShadowDraw()
{
	D3D12_VIEWPORT ShadowViewPort;
	ShadowViewPort.TopLeftX = 0;
	ShadowViewPort.TopLeftY = 0;
	ShadowViewPort.Width = 1024;
	ShadowViewPort.Height = 1024;
	ShadowViewPort.MaxDepth = 1.0f;
	ShadowViewPort.MinDepth = 0.0f;
	D3D12_RECT ShadowScissor;
	ShadowScissor.left = 0;
	ShadowScissor.top = 0;
	ShadowScissor.right = 1024;
	ShadowScissor.bottom = 1024;
	

	_PrimitiveRender->ShadowDraw(_CmdList, ShadowViewPort, ShadowScissor, _DepthBufferHeap);

	_CmdList->SetDescriptorHeaps(1, _TransformHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(1, _TransformHeap->GetGPUDescriptorHandleForHeapStart());

	_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& PrimitiveData : Primitives)
	{
		_CmdList->IASetIndexBuffer(&PrimitiveData->GetPrimitiveIndexBV());

		//頂点バッファをセット
		_CmdList->IASetVertexBuffers(0, 1, &PrimitiveData->GetPrimitiveVertexBV());

		//座標
		_CmdList->SetDescriptorHeaps(1, _TransformHeap.GetAddressOf());
		_CmdList->SetGraphicsRootDescriptorTable(1, _TransformHeap->GetGPUDescriptorHandleForHeapStart());

		_CmdList->SetDescriptorHeaps(1, _DepthBufferHeap.GetAddressOf());
		_CmdList->SetGraphicsRootDescriptorTable(4, _DepthBufferHeap->GetGPUDescriptorHandleForHeapStart());

		_CmdList->DrawIndexedInstanced(PrimitiveData->GetPrimitiveIndexBV().SizeInBytes / sizeof(uint16_t), 1, 0, 0, 0);
	}


	_ShadowPass->Draw(_CmdList, ShadowViewPort, ShadowScissor, _DepthBufferHeap);

	_CmdList->SetDescriptorHeaps(1, _TransformHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(1, _TransformHeap->GetGPUDescriptorHandleForHeapStart());

}


void Dx12Wrapper::DrawSetting()
{

	PassClearAndSetRendererTarget(0,true);

	DrawPrimitiveShapes();


	//ルートシグネチャをセット
	_CmdList->SetGraphicsRootSignature(_RootSigunature.Get());

	//ビューポートをセット
	_CmdList->RSSetViewports(1, &_ViewPort);

	//シザーをセット
	_CmdList->RSSetScissorRects(1, &_ScissorRect);

	//パイプラインをセット
	_CmdList->SetPipelineState(_PipelineState.Get());

	_CmdList->SetDescriptorHeaps(1, _TransformHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(1, _TransformHeap->GetGPUDescriptorHandleForHeapStart());

	_CmdList->SetDescriptorHeaps(1, _DepthBufferHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(4, _DepthBufferHeap->GetGPUDescriptorHandleForHeapStart());
}


void Dx12Wrapper::ScreenFlip()
{
	HRESULT Result = S_OK;
	Result = _DxgiSwapChain->Present(1,0);
	assert(SUCCEEDED(Result));
}

void Dx12Wrapper::StartEffect()
{
	if (_EffekseerManager->Exists(_EffekseerHandle))
	{
		_EffekseerManager->StopEffect(_EffekseerHandle);
	}
	_EffekseerHandle = (_EffekseerManager->Play(_Effect.Get(), Effekseer::Vector3D(0, -5, 0)));
}

void Dx12Wrapper::SetRootSignature(Microsoft::WRL::ComPtr<ID3D12RootSignature>& RootSigunature)
{
	_RootSigunature = RootSigunature;
}

void Dx12Wrapper::SetPipeLineState(Microsoft::WRL::ComPtr<ID3D12PipelineState>& PipelineState)
{
	_PipelineState = PipelineState;
}

void Dx12Wrapper::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology)
{
	_CmdList->IASetPrimitiveTopology(PrimitiveTopology);
}

Microsoft::WRL::ComPtr<ID3D12Device>& Dx12Wrapper::GetDevice()
{
	return _Dev;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& Dx12Wrapper::GetCmdList()
{
	return _CmdList;
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue>& Dx12Wrapper::GetCmdQueue()
{
	return _CmdQueue;
}

void Dx12Wrapper::WaitWithFence()
{
	//ポーリング待機
	while (_Fence->GetCompletedValue() != _FenceValue)
	{
		auto Event = CreateEvent(nullptr, false, false, nullptr);
		_Fence->SetEventOnCompletion(_FenceValue, Event);
		WaitForSingleObject(Event, INFINITE);	//ここで待機。
		CloseHandle(Event);
	}
}

void Dx12Wrapper::EndDraw()
{

	AddBarrier(0,D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	DrawSSAO();
	unsigned int RTVOffset = _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	TotalActivePass = 0;

	for (int i=0;i<RendererPasses.size()-1;i++)
	{

		if (!RendererPasses[i]->GetActiveFlag())
		{
			continue;
		}

		TotalActivePass++;

		int Num = (TotalActivePass) % 2;

		//コピー先に指定
		AddBarrier(Num, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
		//コピー元に指定
		AddBarrier((TotalActivePass-1)%2, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>TargetHandle;

		//対象であるバッファはリストにセット、そうでない場合はコピー
		auto& Flags = RendererPasses[i]->GetUseBufferFlag();
		auto HeapPointer = BufferHeaps[Num]._Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		for (int FlagNum=0;FlagNum<PassRenderer::BufferTag::MAX;FlagNum++)
		{
			if (Flags[FlagNum])
			{
				TargetHandle.emplace_back(HeapPointer);
			}
			else
			{
				_CmdList->CopyTextureRegion(
					&CD3DX12_TEXTURE_COPY_LOCATION(Buffers[Num][FlagNum].Get(), 0),
					0,
					0,
					0,
					&CD3DX12_TEXTURE_COPY_LOCATION(Buffers[(TotalActivePass -1)%2][FlagNum].Get(), 0),
					nullptr
				);
			}

			HeapPointer.ptr += RTVOffset;
		}
		//コピー終了
		AddBarrier( (TotalActivePass-1) % 2, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		AddBarrier(Num, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);

		//リソースバリアを設定
		_CmdList->OMSetRenderTargets(TargetHandle.size(), TargetHandle.data(), false, nullptr);
		float ClsColor[4] = { 0,0,0,0 };
		int Counter = 0;
		//for (auto& Rt : Rtvs)
		for (auto& Rt : TargetHandle)
		{
			_CmdList->ClearRenderTargetView(Rt, ClsColor, 0, nullptr);
			Counter++;
		}


		RendererPasses[i]->DrawSetting(_CmdList, _ViewPort, _ScissorRect);

		_CmdList->SetDescriptorHeaps(1, BufferHeaps[(TotalActivePass-1) % 2]._Srv_DescriptorHeap.GetAddressOf());
		_CmdList->SetGraphicsRootDescriptorTable(0, BufferHeaps[(TotalActivePass-1) % 2]._Srv_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		_CmdList->SetDescriptorHeaps(1, _DepthBufferHeap.GetAddressOf());
		_CmdList->SetGraphicsRootDescriptorTable(1, _DepthBufferHeap->GetGPUDescriptorHandleForHeapStart());

		RendererPasses[i]->PassDraw(_CmdList);

		AddBarrier((TotalActivePass)%2, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
}

bool Dx12Wrapper::ScreenCrear()
{
	auto BackBufferIdx = _DxgiSwapChain->GetCurrentBackBufferIndex();
	auto HeapPointer = _Rtv_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	HeapPointer.ptr += BackBufferIdx * _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//バリア追加
	AddBarrier(_BackBuffers[_DxgiSwapChain->GetCurrentBackBufferIndex()], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	//レンダリング
	_CmdList->OMSetRenderTargets(1, &HeapPointer, false, nullptr);

	float ClsColor[4] = { 0,0, 0,0.0 };
	_CmdList->ClearRenderTargetView(HeapPointer, ClsColor, 0, nullptr);


	return true;
}

void Dx12Wrapper::Draw()
{

	int Num = RendererPasses.size() - 1;
	RendererPasses[Num]->DrawSetting(_CmdList, _ViewPort, _ScissorRect);
	_CmdList->SetDescriptorHeaps(1, BufferHeaps[(TotalActivePass)%2]._Srv_DescriptorHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(0, BufferHeaps[(TotalActivePass) % 2]._Srv_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	_CmdList->SetDescriptorHeaps(1, _DepthBufferHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(1, _DepthBufferHeap->GetGPUDescriptorHandleForHeapStart());


	_CmdList->SetDescriptorHeaps(1, SSAODatas._Srv_DescriptorHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(3, SSAODatas._Srv_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	RendererPasses[Num]->PassDraw(_CmdList);
	EffekseerDraw();
	//ここまでがレンダリング//

	//ここからDXTK
	_CmdList->SetDescriptorHeaps(1, _DxtkHeap.GetAddressOf());
	_SpriteBatch->Begin(_CmdList.Get());

	_SpriteFont->DrawString(&*_SpriteBatch, L"モデルの移動:W,A,S,D", DirectX::XMFLOAT2(12, 12), DirectX::Colors::Black);
	_SpriteFont->DrawString(&*_SpriteBatch, L"モデルの移動:W,A,S,D", DirectX::XMFLOAT2(10, 10), DirectX::Colors::Yellow);

	_SpriteFont->DrawString(&*_SpriteBatch, L"モデルの上昇下降:Q,E", DirectX::XMFLOAT2(12, 42), DirectX::Colors::Black);
	_SpriteFont->DrawString(&*_SpriteBatch, L"モデルの上昇下降:Q,E", DirectX::XMFLOAT2(10, 40), DirectX::Colors::Yellow);

	_SpriteFont->DrawString(&*_SpriteBatch, L"モデルの回転:Num4,Num6", DirectX::XMFLOAT2(12, 72), DirectX::Colors::Black);
	_SpriteFont->DrawString(&*_SpriteBatch, L"モデルの回転:Num4,Num6", DirectX::XMFLOAT2(10, 70), DirectX::Colors::Yellow);

	_SpriteFont->DrawString(&*_SpriteBatch, L"カメラの移動:矢印キー", DirectX::XMFLOAT2(12, 102), DirectX::Colors::Black);
	_SpriteFont->DrawString(&*_SpriteBatch, L"カメラの移動:矢印キー", DirectX::XMFLOAT2(10, 100), DirectX::Colors::Yellow);

	_SpriteFont->DrawString(&*_SpriteBatch, L"カメラのZ移動:Z,X", DirectX::XMFLOAT2(12, 132), DirectX::Colors::Black);
	_SpriteFont->DrawString(&*_SpriteBatch, L"カメラのZ移動:Z,X", DirectX::XMFLOAT2(10, 130), DirectX::Colors::Yellow);

	_SpriteBatch->End();

	//ここからImgi
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Information");
	ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	if (ImGui::TreeNode("EyePos"))
	{
		ImGui::DragFloat("X", &Eye.x);
		ImGui::DragFloat("Y", &Eye.y);
		ImGui::DragFloat("Z", &Eye.z);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("TargetPos"))
	{
		ImGui::DragFloat("X", &Target.x);
		ImGui::DragFloat("Y", &Target.y);
		ImGui::DragFloat("Z", &Target.z);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Offset"))
	{
		ImGui::DragFloat("X", &Offset.x);
		ImGui::DragFloat("Y", &Offset.y);
		ImGui::DragFloat("Z", &Offset.z);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("PassDatas"))
	{
		for (auto& Pass : RendererPasses)
		{
			Pass->ImGuiDraw();
			ImGui::Separator();
		}

		ImGui::TreePop();
	}


	ImGui::End();
	
	_CmdList->SetDescriptorHeaps(1, _ImGuiHeap.GetAddressOf());
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),_CmdList.Get());

	AddBarrier(_BackBuffers[_DxgiSwapChain->GetCurrentBackBufferIndex()], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	_CmdList->Close();
	ID3D12CommandList* Cmds[] = { _CmdList.Get() };
	_CmdQueue->ExecuteCommandLists(1, Cmds);
	++_FenceValue;

	_CmdQueue->Signal(_Fence.Get(), _FenceValue);
	WaitWithFence();

	_CmdAlloc->Reset();

	_CmdList->Reset(_CmdAlloc.Get(), _PipelineState.Get());
	_CmdList->ClearDepthStencilView(_DepthStencilViewHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}


void Dx12Wrapper::AddBarrier(Microsoft::WRL::ComPtr<ID3D12Resource>& Buffer,D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{
	//リソースバリアを設定
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.StateBefore = StateBefore;
	BarrierDesc.Transition.StateAfter = StateAfter;
	BarrierDesc.Transition.pResource = Buffer.Get();
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	//レンダリング前にバリアを追加
	_CmdList->ResourceBarrier(1, &BarrierDesc);
}
