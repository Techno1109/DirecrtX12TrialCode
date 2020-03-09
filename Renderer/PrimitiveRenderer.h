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

	//�p�C�v���C���X�e�[�g
	Microsoft::WRL::ComPtr <ID3D12PipelineState> _ShadowPipelineState;
	//���_�V�F�[�_
	Microsoft::WRL::ComPtr <ID3DBlob> _ShadowVertexShader;

	//�s�N�Z���V�F�[�_
	Microsoft::WRL::ComPtr <ID3DBlob> _ShadowPixelShader;
};

