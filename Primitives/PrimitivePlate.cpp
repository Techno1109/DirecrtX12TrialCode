#include "PrimitivePlate.h"
#include <d3dx12.h>
#include <assert.h>

using namespace DirectX;

PrimitivePlate::PrimitivePlate(Microsoft::WRL::ComPtr<ID3D12Device>& _dev, float x_size, float y_size, DirectX::XMFLOAT3 center):PrimitiveBase(_dev),X_Size(x_size),Y_Size(y_size),Center(center)
{
	CreatePrimitive();
}


PrimitivePlate::~PrimitivePlate()
{
}

void PrimitivePlate::CreatePrimitive()
{

	//頂点バッファ作成

	HRESULT Result = S_OK;

	unsigned short Indexes[6] = { 0,1,2, 2,1,3 };

	PrimitiveVertex Plane[4];

	Plane[0] = { { Center.x - (X_Size / 2.0f), Center.y, Center.z - (Y_Size / 2.0f)}, { 0,1,0 } };
	Plane[1] = { { Center.x - (X_Size / 2.0f), Center.y, Center.z + (Y_Size / 2.0f)}, { 0,1,0 } };
	Plane[2] = { { Center.x + (X_Size / 2.0f), Center.y, Center.z - (Y_Size / 2.0f)}, { 0,1,0 } };
	Plane[3] = { { Center.x + (X_Size / 2.0f), Center.y, Center.z + (Y_Size / 2.0f)}, { 0,1,0 } };

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Plane));

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_PrimitiveVB));

	if (FAILED(Result))
	{
		assert(false);
		return;
	}


	_PrimitiveVBV = {};

	//頂点バッファビュー作成
	_PrimitiveVBV.BufferLocation = _PrimitiveVB->GetGPUVirtualAddress();

	_PrimitiveVBV.StrideInBytes = sizeof(PrimitiveVertex);
	_PrimitiveVBV.SizeInBytes = sizeof(Plane);


	//頂点Map作成
	PrimitiveVertex* _VertexMap = nullptr;
	Result = _PrimitiveVB->Map(0, nullptr, (void**)&_VertexMap);
	std::copy(std::begin(Plane), std::end(Plane), _VertexMap);
	_PrimitiveVB->Unmap(0, nullptr);


	//インデックスバッファ作成


	D3D12_HEAP_PROPERTIES InHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC InResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Indexes));


	Result = _Dev->CreateCommittedResource(&InHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&InResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_PrimitiveIB));

	if (FAILED(Result))
	{
		assert(false);
		return;
	}

	 _PrimitiveIBV = {};

	//インデクスバッファビュー設定
	_PrimitiveIBV.BufferLocation = _PrimitiveIB->GetGPUVirtualAddress();
	_PrimitiveIBV.Format = DXGI_FORMAT_R16_UINT;

	_PrimitiveIBV.SizeInBytes = sizeof(Indexes);


	//IndexMap作成
	unsigned short* _IndexMap = nullptr;

	Result = _PrimitiveIB->Map(0, nullptr, (void**)&_IndexMap);

	if (FAILED(Result))
	{
		assert(false);
		return;
	}
	std::copy(std::begin(Indexes), std::end(Indexes), _IndexMap);

	_PrimitiveIB->Unmap(0, nullptr);
}
