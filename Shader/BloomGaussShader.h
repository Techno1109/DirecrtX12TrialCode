#pragma once
#include "PassRenderer.h"
#include <vector>
class BloomGaussShader :
	public PassRenderer
{
public:
	BloomGaussShader(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, float S, int SampleNum, int Priority);
	~BloomGaussShader();

	void PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList)override;
	void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) override;

private:
	bool CreateRootSignature() override;
	bool CreatePipeLineState() override;
	bool Create_Rtv_Srv() override;

	int _S;
	int _SampleNum;

	bool CreateWeight();

	std::vector<float> GetWeight();
	Microsoft::WRL::ComPtr<ID3D12Resource>_WeightBuffer;
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _WeightHeap;

};

