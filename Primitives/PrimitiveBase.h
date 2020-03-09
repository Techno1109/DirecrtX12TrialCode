#pragma once
#include<vector>
#include <wrl/client.h>
#include <d3d12.h>
#include <DirectXMath.h>

class PrimitiveBase
{
public:
	PrimitiveBase(Microsoft::WRL::ComPtr<ID3D12Device>& _dev);
	~PrimitiveBase();

	D3D12_VERTEX_BUFFER_VIEW& GetPrimitiveVertexBV();
	D3D12_INDEX_BUFFER_VIEW& GetPrimitiveIndexBV();
protected:

	struct PrimitiveVertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
	};

	//デバイス--生成時に受け取り。
	Microsoft::WRL::ComPtr<ID3D12Device> _Dev;

	virtual void CreatePrimitive()=0;

	Microsoft::WRL::ComPtr<ID3D12Resource> _PrimitiveVB;
	D3D12_VERTEX_BUFFER_VIEW _PrimitiveVBV;

	Microsoft::WRL::ComPtr<ID3D12Resource> _PrimitiveIB;
	D3D12_INDEX_BUFFER_VIEW _PrimitiveIBV;
};

