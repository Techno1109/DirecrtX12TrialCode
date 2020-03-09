#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class PMDRenderer
{
public:
	PMDRenderer(Microsoft::WRL::ComPtr<ID3D12Device>& _dev);
	~PMDRenderer();

	bool Init();

	Microsoft::WRL::ComPtr <ID3D12PipelineState>& GetPipeLineState();
	Microsoft::WRL::ComPtr <ID3D12RootSignature>& GetRootSignature();

private:
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

	bool CreateRootSignature();
	bool CreatePipeLineState();
	bool LoadShader();

};

