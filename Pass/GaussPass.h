#pragma once
#include "PassRenderer.h"
#include<vector>
class GaussPass :
	public PassRenderer
{
public:
	GaussPass(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, float S, int SampleNum, int Priority);
	~GaussPass();

	void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) override;
	Microsoft::WRL::ComPtr<ID3D12Resource>& GetWeightBuffer();
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>&  GetWeightHeap();
private:
	int _S;
	int _SampleNum;

	bool CreateRootSignature() override;
	bool CreatePipeLineState() override;
	bool Create_Rtv_Srv() override;
	bool CreateWeight();
	void ImGuiDrawBase()override;
	std::vector<float> GetWeight();
	Microsoft::WRL::ComPtr<ID3D12Resource>_WeightBuffer;
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _WeightHeap;
};

