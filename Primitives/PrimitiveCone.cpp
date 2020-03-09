#include "PrimitiveCone.h"
#include <DirectXMath.h>
#include <d3dx12.h>
#include <assert.h>
using namespace DirectX;

PrimitiveCone::PrimitiveCone(Microsoft::WRL::ComPtr<ID3D12Device>& _dev, int divCount, float Height, float radius, DirectX::XMFLOAT3 center):PrimitiveBase(_dev),DivCount(divCount),Height(Height),Radius(radius),Center(center)
{
	CreatePrimitive();
}


PrimitiveCone::~PrimitiveCone()
{
}

void PrimitiveCone::CreatePrimitive()
{
	HRESULT Result = S_OK;
	float Rad = (XM_PI / 180.0f)*(360.0f / DivCount);
	std::vector<PrimitiveVertex> ConeVertex;
	for (int i = 0; i < DivCount; i++)
	{
		PrimitiveVertex Insert = { { Center.x ,Center.y, Center.z}, { 0,1,0 } };
		Insert.Pos.x += Radius * cos(Rad*i);
		Insert.Pos.z += Radius * sin(Rad*i);
		ConeVertex.emplace_back(Insert);
	}
	PrimitiveVertex Top = { { Center.x , Center.y + 10, Center.z}, { 0,1,0 } };
	ConeVertex.emplace_back(Top);

	std::vector<unsigned short> ConeIndex;

	for (int i = 0; i < DivCount; i++)
	{
		if (i == DivCount - 1)
		{
			ConeIndex.emplace_back(0);
			ConeIndex.emplace_back(DivCount);
			ConeIndex.emplace_back(DivCount - 1);
			continue;
		}
		ConeIndex.emplace_back(i + 1);
		ConeIndex.emplace_back(DivCount);
		ConeIndex.emplace_back(i);
	}


	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(PrimitiveVertex)*ConeVertex.size());

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
	_PrimitiveVBV.SizeInBytes = sizeof(PrimitiveVertex)*ConeVertex.size();


	//頂点Map作成
	PrimitiveVertex* _CVertexMap = nullptr;
	Result = _PrimitiveVB->Map(0, nullptr, (void**)&_CVertexMap);
	std::copy(ConeVertex.begin(), ConeVertex.end(), _CVertexMap);
	_PrimitiveVB->Unmap(0, nullptr);


	//インデックスバッファ作成


	D3D12_HEAP_PROPERTIES InHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC InResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(unsigned int)*ConeIndex.size());

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

	_PrimitiveIBV.SizeInBytes = sizeof(unsigned int)*ConeIndex.size();


	//IndexMap作成
	unsigned short* _CIndexMap = nullptr;

	Result = _PrimitiveIB->Map(0, nullptr, (void**)&_CIndexMap);

	if (FAILED(Result))
	{
		assert(false);
		return;
	}
	std::copy(ConeIndex.begin(), ConeIndex.end(), _CIndexMap);

	_PrimitiveIB->Unmap(0, nullptr);
}
