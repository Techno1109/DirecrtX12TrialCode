#pragma once
#include "PassRenderer.h"
class DofPass :
	public PassRenderer
{
public:
	DofPass(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, int Priority);
	~DofPass();
	void DrawSetting(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect) override;
	void PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList)override;
	void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) override;
	void PassClearAndSetRendererTarget(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthStencilViewHeap, bool First) override;
private:
	bool CreateRootSignature() override;
	bool CreatePipeLineState() override;
	bool Create_Rtv_Srv() override;
};

