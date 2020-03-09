#pragma once
#include "PrimitiveBase.h"
#include <DirectXMath.h>

class PrimitivePlate :
	public PrimitiveBase
{
public:
	PrimitivePlate(Microsoft::WRL::ComPtr<ID3D12Device>& _dev,float x_size,float y_size ,DirectX::XMFLOAT3 center);
	~PrimitivePlate();
protected:
	float X_Size;
	float Y_Size;
	DirectX::XMFLOAT3 Center;
	void CreatePrimitive() override;
};

