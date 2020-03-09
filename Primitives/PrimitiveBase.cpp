#include "PrimitiveBase.h"

PrimitiveBase::PrimitiveBase(Microsoft::WRL::ComPtr<ID3D12Device>& _dev):_Dev(_dev)
{
}

PrimitiveBase::~PrimitiveBase()
{
}

D3D12_VERTEX_BUFFER_VIEW & PrimitiveBase::GetPrimitiveVertexBV()
{
	return _PrimitiveVBV;
}

D3D12_INDEX_BUFFER_VIEW & PrimitiveBase::GetPrimitiveIndexBV()
{
	return _PrimitiveIBV;
}
