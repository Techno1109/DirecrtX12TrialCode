#pragma once
#include "PassRenderer.h"
class PrimitiveRenderer :
	public PassRenderer
{
public:
	PrimitiveRenderer(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, int Priority);
	~PrimitiveRenderer();

	void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) override;
	void ShadowDraw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap);
private:
	bool CreateRootSignature() override;
	bool CreatePipeLineState() override;
	bool Create_Rtv_Srv() override;

	bool LoadShadowShader(std::string ShaderPath);
	bool CreateShadowPipeLineState();

	//パイプラインステート
	Microsoft::WRL::ComPtr <ID3D12PipelineState> _ShadowPipelineState;
	//頂点シェーダ
	Microsoft::WRL::ComPtr <ID3DBlob> _ShadowVertexShader;

	//ピクセルシェーダ
	Microsoft::WRL::ComPtr <ID3DBlob> _ShadowPixelShader;
};

