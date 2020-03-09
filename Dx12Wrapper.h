#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <map>
#include <memory>
#include <array>
#include <EffekseerRendererDX12.h>
#include<SpriteFont.h>
#include<ResourceUploadBatch.h>
#pragma comment(lib,"DirectXTex.lib")

//DirectX12の初期化等の各要素等をまとめるためのクラス
class PassRenderer;
class NormalMappingPass;
class GaussPass;
class PlanePass;
class ShadowMapPass;
class PrimitiveRenderer;
class PrimitiveBase;
class SSAOPass;
class RayMarchingPass;
class Dx12Wrapper
{
public:
	Dx12Wrapper(HWND windowH);
	~Dx12Wrapper();

	bool Init();

	void SetRootSignature(Microsoft::WRL::ComPtr <ID3D12RootSignature>& RootSigunature);
	void SetPipeLineState(Microsoft::WRL::ComPtr <ID3D12PipelineState>& PipelineState);
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

	Microsoft::WRL::ComPtr<ID3D12Device>& GetDevice();
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& GetCmdList();
	Microsoft::WRL::ComPtr <ID3D12CommandQueue>& GetCmdQueue();

	void SetFov(float Angle);
	void SetEyePosition(float x, float y, float z);
	void SetTargetPosition(float x, float y, float z);

	void MoveFov(float Angle);
	void MoveEyePosition(float x, float y, float z);
	void MoveTargetPosition(float x, float y, float z);
	void MoveOffset(float x, float y, float z);


	//影用描画
	void ShadowDrawSetting();

	void ShadowDraw();

	bool DrawPrimitiveShapes();

	//画面のリセット
	bool ScreenCrear();
	//描画前設定
	void DrawSetting();
	//描画終了(コマンド実行)
	void EndDraw();
	//1stPath描画
	void Draw();
	//画面の更新
	void ScreenFlip();

	void StartEffect();
private:
	struct TransformMatrix
	{
		DirectX::XMMATRIX Camera;//View&Projection
		DirectX::XMFLOAT3 Eye;
		DirectX::XMFLOAT3 LightPos;
		DirectX::XMMATRIX Shadow;
		DirectX::XMMATRIX LightCamera;	//カメラから見たビュープロジェクション
		DirectX::XMMATRIX Projection;
		DirectX::XMMATRIX InProjection;
	};

//ここから下で大改修--------------------------------
	struct DrawBuffers
	{
		Microsoft::WRL::ComPtr <ID3D12Resource> _MainBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _NormalBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _BloomBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _ShrinkBloomBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _FovBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _ShrinkFovBuffer;
	};

	struct DrawBufferHeaps
	{
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Rtv_DescriptorHeap;
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Srv_DescriptorHeap;
	};
	
	struct SSAOBufferAndHeaps
	{
		Microsoft::WRL::ComPtr <ID3D12Resource> _Buffer;
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Rtv_DescriptorHeap;
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Srv_DescriptorHeap;
	};

	SSAOBufferAndHeaps SSAODatas;
	//描画バッファー2枚
	std::array<std::array<Microsoft::WRL::ComPtr <ID3D12Resource>, 6>, 2>Buffers;
	//描画バッファー用Heap2セット
	std::array<DrawBufferHeaps, 2> BufferHeaps;
	std::unique_ptr<SSAOPass> _SSAORender;
	bool Create_Rtv_Srvs();
	bool Create_SSAO_Rtv_Srv();
	void DrawSSAO();
	void AddBarrier(int PassNum, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
	void PassClearAndSetRendererTarget(int Num,bool First);

//ここまでで頑張る------------------------

	//影描画用
	std::unique_ptr<ShadowMapPass> _ShadowPass;
	//プリミティブ描画用
	std::unique_ptr<PrimitiveRenderer> _PrimitiveRender;

	//パス描画用
	std::vector<std::shared_ptr<PassRenderer>> RendererPasses;

	//ウィンドウハンドル-----------
	HWND _WindowH;

	//有効化されているパス(0スタート)
	int TotalActivePass;

	int _EffekseerHandle;
	Microsoft::WRL::ComPtr	< EffekseerRenderer::Renderer> _EffekseerRenderer;
	Microsoft::WRL::ComPtr	< Effekseer::Manager> _EffekseerManager;
	Microsoft::WRL::ComPtr	< EffekseerRenderer::SingleFrameMemoryPool> _EffekseerMemoryPool;
	Microsoft::WRL::ComPtr	< EffekseerRenderer::CommandList> _EffekseerCommandList;

	Microsoft::WRL::ComPtr<Effekseer::Effect> _Effect;

	void EffekseerInit();

	void EffekseerDraw();

	//IMGUI
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _ImGuiHeap;

	//DXTK
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _DxtkHeap;
	std::shared_ptr < DirectX::GraphicsMemory> _Gmemory = nullptr;//グラフィクスメモリオブジェクト 
	std::shared_ptr < DirectX::SpriteFont> _SpriteFont = nullptr;//フォント表示用オブジェクト 
	std::shared_ptr < DirectX::SpriteBatch> _SpriteBatch = nullptr;//スプライト表示用オブジェクト 

	//DXGI-----------
	Microsoft::WRL::ComPtr<IDXGIFactory6> _DxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _DxgiSwapChain;

	//デバイス-----------
	Microsoft::WRL::ComPtr<ID3D12Device> _Dev;

	//Primitive
	std::vector<std::unique_ptr<PrimitiveBase>> Primitives;
	bool CreatePrimitives();

	//ルートシグネチャ系
	//ルートシグネチャ
	Microsoft::WRL::ComPtr <ID3D12RootSignature> _RootSigunature;

	//パイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> _PipelineState;



	//レンダーターゲットView作成
	bool CreateRenderTargetView();
	//レンダーターゲット用デスクリプタヒープ
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Rtv_DescriptorHeap;
	//スワップチェインのリソースポインタ群
	std::vector< Microsoft::WRL::ComPtr<ID3D12Resource>> _BackBuffers;


	//コマンドリスト作成
	bool CreateCommandList();
	//コマンドリスト確保用オブジェクト
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator> _CmdAlloc;
	//コマンドリスト本体
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> _CmdList;
	//コマンドリストのキュー
	Microsoft::WRL::ComPtr <ID3D12CommandQueue> _CmdQueue;

	//震度バッファを作成
	bool CreateDepthBuffer();

	//深度テクスチャ
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _DepthBufferHeap;
	//深度バッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _DepthBuffer;
	//シャドウマップ用深度バッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _LightDepthBuffer;
	//深度ステンシルビューヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _DepthStencilViewHeap;

	//ビューポートを作成
	bool SetViewPort();
	//ビューポート
	D3D12_VIEWPORT _ViewPort;
	//シザー
	D3D12_RECT _ScissorRect;


	//座標変換用定数バッファ及び、バッファビューを作成
	bool CreateTransformConstantBuffer();
	//座標変換定数バッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _TransformCB;
	//座標変換ヒープ
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _TransformHeap;
	//描画マトリクス
	TransformMatrix* MappedMatrix;


	DirectX::XMMATRIX GetCurrentCameraMatrix();
	//視点
	DirectX::XMFLOAT3 Eye;
	//注視点
	DirectX::XMFLOAT3 Target;
	//視点
	DirectX::XMFLOAT3 Offset;
	//上ベクトルが必要
	DirectX::XMFLOAT3 UpperVec;
	//モデルの場所
	DirectX::XMFLOAT3 ActorPos = DirectX::XMFLOAT3(0,0,0);
	//画角
	float _Fov = DirectX::XM_PIDIV4;

	//ポーリング待機
	void WaitWithFence();

	//バリア追加
	void AddBarrier(Microsoft::WRL::ComPtr<ID3D12Resource>& Buffer,D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
	//フェンスオブジェクト-----------
	Microsoft::WRL::ComPtr <ID3D12Fence> _Fence;
	//フェンス値
	UINT64 _FenceValue;

};
