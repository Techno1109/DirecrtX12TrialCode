#pragma once
#include "PassRenderer.h"
class RayMarchingPass :
	public PassRenderer
{
public:
	RayMarchingPass(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, int Priority);
	~RayMarchingPass();

	void DrawSetting(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect) override;
	void PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList)override;
	void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) override;
	void PassClearAndSetRendererTarget(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthStencilViewHeap, bool First) override;
private:
	struct RayMarchShaderSignal
	{
		uint32_t OnDebug;
	};

	RayMarchShaderSignal Signals;
	RayMarchShaderSignal* MappedSignal;
	Microsoft::WRL::ComPtr<ID3D12Resource>_SignalBuffer;
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _SignalHeap;
	bool CreateSignalBuffer();

	void SetSignal();
	bool CreateRootSignature() override;
	bool CreatePipeLineState() override;
	bool Create_Rtv_Srv() override;
	bool LoadShader(std::string ShaderPath) override;
	void AddBarrier(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter) override;
protected:
	virtual void ImGuiDrawBase()override;
};

