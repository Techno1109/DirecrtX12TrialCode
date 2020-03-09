#pragma once
#include "PassRenderer.h"
class NormalMappingPass :
	public PassRenderer
{
public:
	NormalMappingPass(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, int Priority);
	~NormalMappingPass();

	void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) override;
	Microsoft::WRL::ComPtr<ID3D12Resource>& GetNormalBuffer();
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>& GetNormalHeap();
private:
	bool CreateRootSignature() override;
	bool CreatePipeLineState() override;
	bool Create_Rtv_Srv() override;
	bool LoadNormalMap();
	float Intensity;
	void ImGuiDrawBase()override;
	Microsoft::WRL::ComPtr<ID3D12Resource>_NormalBuffer;
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _NormalHeap;
};

