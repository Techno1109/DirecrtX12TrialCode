#pragma once
#include "PrimitiveBase.h"
class PrimitiveCone :
	public PrimitiveBase
{
public:
	PrimitiveCone(Microsoft::WRL::ComPtr<ID3D12Device>& _dev,int divCount,float hegiht,float radius,DirectX::XMFLOAT3 center);
	~PrimitiveCone();
protected:
	int DivCount;
	float Height;
	float Radius;
	DirectX::XMFLOAT3 Center;
	void CreatePrimitive() override;
};

