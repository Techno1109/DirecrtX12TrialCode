#pragma once
#include "PassRenderer.h"
class GaussPass;
class PlanePass :
	public PassRenderer
{
public:
	PlanePass(Microsoft::WRL::ComPtr<ID3D12Device>& Dev, std::string ShaderPath, int Priority);
	~PlanePass();

	void DrawSetting(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT & _ViewPort, D3D12_RECT & _ScissorRect) override;
	void PassDraw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& _CmdList)override;
	void Draw(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, D3D12_VIEWPORT& _ViewPort, D3D12_RECT& _ScissorRect, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthBufferViewHeap) override;
	void PassClearAndSetRendererTarget(Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& _CmdList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& _DepthStencilViewHeap, bool First) override;
private:
	struct ShaderSignal
	{
		uint32_t OnDebug;
		uint32_t NormalWindow;
		uint32_t BloomWindow;
		uint32_t ShrinkBloomWindow;
		uint32_t FovWindow;
		uint32_t ShrinkFovWindow;
		uint32_t SSAOWindow;
		uint32_t Depth;
		uint32_t LightDepth;
		uint32_t OutLine;
		uint32_t FXAA;
		uint32_t Bloom;
		uint32_t SSAO;
	};

	ShaderSignal Signals;
	ShaderSignal* MappedSignal;
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

