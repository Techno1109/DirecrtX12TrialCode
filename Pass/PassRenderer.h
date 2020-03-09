#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include<string>
#include<array>

class PassRenderer
{
public:
	enum PassType
	{
		SHADER,
		SHADOW
	};

	enum BufferTag
	{
	  MainBuffer,
	  NormalBuffer,
	  BloomBuffer,
	  ShrinkBloomBuffer,
	  FovBuffer,
	  ShrinkFovBuffer,
	  MAX
	};

	PassRenderer(Microsoft::WRL::ComPtr<ID3D12Device>& Dev,std::string ShaderPath,int Priority, PassType Type);
	virtual ~PassRenderer();

	virtual void DrawSetting(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect);
	virtual void PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList);

	virtual void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort,D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) = 0;
	
	void ImGuiDraw();

	virtual void PassClearAndSetRendererTarget(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthStencilViewHeap, bool First);
	virtual void AddBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);

	bool& GetActiveFlag();

	Microsoft::WRL::ComPtr <ID3D12PipelineState>& GetPipeLineState();
	Microsoft::WRL::ComPtr <ID3D12RootSignature>& GetRootSignature();
	Microsoft::WRL::ComPtr <ID3D12Resource>& GetBuffer();
	D3D12_VERTEX_BUFFER_VIEW&					  GetVertexBufferView();
	Microsoft::WRL::ComPtr <ID3D12Resource>&	  GetVertexBuffer();
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> GetRtv_DescriptorHeap();
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> GetSrv_DescriptorHeap();

	std::array<bool, BufferTag::MAX>& GetUseBufferFlag();
	PassType& GetPassType();
	const int& GetPriority();

private:
	int _Priority;
	bool CreatePassPoly();

protected:
	bool ActiveFlag;
	std::string _ShaderPath;

	std::array<bool, BufferTag::MAX> BufferFlag;

	//デバイス--生成時に受け取り。
	Microsoft::WRL::ComPtr<ID3D12Device> _Dev;
	//ルートシグネチャ系
	//ルートシグネチャ
	Microsoft::WRL::ComPtr <ID3D12RootSignature> _RootSigunature;
	//シグネチャ
	Microsoft::WRL::ComPtr <ID3DBlob> _Signature;
	//エラー対処用
	Microsoft::WRL::ComPtr <ID3DBlob> _Error;

	//パイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> _PipelineState;

	//シェーダー関係-------
	//頂点シェーダ
	Microsoft::WRL::ComPtr <ID3DBlob> _VertexShader;

	//ピクセルシェーダ
	Microsoft::WRL::ComPtr <ID3DBlob> _PixelShader;

	Microsoft::WRL::ComPtr <ID3D12Resource> _Buffer;
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Rtv_DescriptorHeap;
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Srv_DescriptorHeap;

	//頂点バッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _VertexBuffer;
	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW _VertexBufferView;

	PassType ThisPassType;

	bool Init(std::string ShaderPath);

	virtual bool CreateRootSignature()=0;
	virtual bool CreatePipeLineState()=0;
	virtual bool Create_Rtv_Srv()=0;
	virtual bool LoadShader(std::string ShaderPath);
	virtual void ImGuiDrawBase();
};

