#include "PMDActor.h"
#include "Application.h"
#include "Tool.h"
#include "VMDMotion.h"
#include "TexLoader.h"
#include <assert.h>
#include <d3dcompiler.h>
#include <d3dx12.h>
#include <DirectXTex.h>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib ")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

PMDActor::PMDActor(Microsoft::WRL::ComPtr<ID3D12Device>& dev, Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& CmdList):_Dev(dev),_CmdList(CmdList), YAngle(XM_PIDIV4)
{
	Position.x = 0;
	Position.y = 0;
	Position.z = 0;
}


PMDActor::~PMDActor()
{
}

const std::vector<uint8_t>& 
PMDActor::GetVertices()
{
	return PMDVertices;
}

const std::vector<uint16_t>& 
PMDActor::GetVertexIndex()
{
	return PMDVertexIndex;
}

const std::vector<PMDActor::PMDMaterial>& 
PMDActor::GetMaterials()
{
	return PMDMaterials;
}

const std::vector<PMDActor::TexturePaths>& 
PMDActor::GetTexutrePathVec()
{
	return PMDTexutrePathVec;
}

void PMDActor::Update()
{
	XMMATRIX Identity = XMMatrixIdentity();
	XMMATRIX TransMatrix = Identity;
	*Transform=XMMatrixRotationY(YAngle)*XMMatrixTranslation(Position.x,Position.y,Position.z)*TransMatrix;

	static auto LastTime = GetTickCount();
	if (GetTickCount() - LastTime > _MotionClass->GetDuration()*33.33333f)
	{
		LastTime = GetTickCount();
	}
	MotionUpDate(static_cast<float>(GetTickCount()-LastTime)/33.3333f);
}

void PMDActor::MotionUpDate(int FrameNum)
{
	XMMATRIX Identity = XMMatrixIdentity();

	std::fill(_BoneMatrices.begin(), _BoneMatrices.end(), Identity);


	auto& MotionData = _MotionClass->GetMotionData();

	for (auto& BoneAnim : MotionData)
	{
		auto& KeyFrame = BoneAnim.second;
		auto TargetFrameItr = std::find_if(KeyFrame.rbegin(), KeyFrame.rend(),
			[FrameNum](const VMDMotion::VmdMotionType& TargetKey) 
			{return TargetKey.FrameNo <= FrameNum; });
		if (TargetFrameItr==KeyFrame.rend())
		{
			continue;
		}


		DirectX::XMVECTOR Rotation;
		DirectX::XMVECTOR Position;
		auto NextKeyFrame = TargetFrameItr.base();
		if (NextKeyFrame != KeyFrame.end())
		{
			float WaitTime = NextKeyFrame->FrameNo - TargetFrameItr->FrameNo;
			float NowTime = (FrameNum-TargetFrameItr->FrameNo) / WaitTime;
			DirectX::XMFLOAT2 Point1 = DirectX::XMFLOAT2(TargetFrameItr->Interpolation[18] / 127.0f, TargetFrameItr->Interpolation[22] / 127.0f);
			DirectX::XMFLOAT2 Point2 = DirectX::XMFLOAT2(TargetFrameItr->Interpolation[26] / 127.0f, TargetFrameItr->Interpolation[30] / 127.0f);
			NowTime = GetYFromXOnBezier(NowTime,Point1,Point2,12);
			Rotation=XMQuaternionSlerp(XMLoadFloat4(&TargetFrameItr->Rotation), XMLoadFloat4(&NextKeyFrame->Rotation), NowTime);
			Position =(1-NowTime)*XMLoadFloat3(&TargetFrameItr->Location)+XMLoadFloat3(&NextKeyFrame->Location)* NowTime;
		}
		else
		{
			Rotation= XMLoadFloat4(&TargetFrameItr->Rotation);
			Position = XMLoadFloat3(&TargetFrameItr->Location);
		}

		BoneBend(BoneAnim.first.c_str(),Rotation,Position, false);
	}
	//ポーズ更新
	RecursiveMatrixMultiply(_BoneMap["センター"], Identity);
	//ポーズ転送
	std::copy(_BoneMatrices.begin(), _BoneMatrices.end(), _MappedBone);
}

void PMDActor::Draw(bool DrawShadow)
{
	//インデックスバッファをセットfdd
	_CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_CmdList->IASetIndexBuffer(&_IndexBufferView);

	//座標
	_CmdList->SetDescriptorHeaps(1, _TransformHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(2, _TransformHeap->GetGPUDescriptorHandleForHeapStart());

	//ボーン行列をセット
	_CmdList->SetDescriptorHeaps(1, _BoneHeap.GetAddressOf());
	_CmdList->SetGraphicsRootDescriptorTable(3, _BoneHeap->GetGPUDescriptorHandleForHeapStart());

	//頂点バッファをセット
	_CmdList->IASetVertexBuffers(0, 1, &_VertexBufferView);


	//マテリアル
	auto MaterialHandle = _MaterialHeap->GetGPUDescriptorHandleForHeapStart();

	ID3D12DescriptorHeap* MaterialHeaps[] = { _MaterialHeap.Get() };
	_CmdList->SetDescriptorHeaps(1, MaterialHeaps);
	_CmdList->SetGraphicsRootDescriptorTable(0, MaterialHandle);

	int RastDrawIndex = 0;
	if (DrawShadow)
	{
		int Sum = 0;

		for (auto& Material : PMDMaterials)
		{
			Sum += Material.FaceVertexCount;
		}
		_CmdList->DrawIndexedInstanced(Sum, 1,0, 0, 0);
		return;
	}
	for (auto& Material : PMDMaterials)
	{
		_CmdList->SetGraphicsRootDescriptorTable(0, MaterialHandle);
		MaterialHandle.ptr += _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;

		//本体と影、2つ描画
		_CmdList->DrawIndexedInstanced(Material.FaceVertexCount, 1, RastDrawIndex, 0, 0);

		RastDrawIndex += Material.FaceVertexCount;
	}
}


const unsigned int PMDActor::GetMaterialSize()
{
	return PMDMaterials.size();
}

void PMDActor::MoveWorldAngle(float Angle)
{
	YAngle += Angle;
}

void PMDActor::SetWorldAngle(float Angle)
{
	YAngle = Angle;
}

void PMDActor::SetPosition(float x, float y, float z)
{
	Position.x = x;
	Position.y = y;
	Position.z = z;
}

void PMDActor::MovePosition(float x, float y, float z)
{
	Position.x += x;
	Position.y += y;
	Position.z += z;
}


bool PMDActor::Init(std::string LoadModelpath, std::string LoadVmdpath)
{
	if (!LoadPMDData(LoadModelpath))
	{
		return false;
	}

	_MotionClass = std::make_unique<VMDMotion>();
	if (!_MotionClass->LoadVMDData(LoadVmdpath))
	{
		return false;
	}

	if (!CreateVertexBuffer())
	{
		return false;
	}

	if (!CreateIndexBuffer())
	{
		return false;
	}

	if (!CreateTransformConstantBuffer())
	{
		return false;
	}

	if (!CreateBoneBuffer())
	{
		return false;
	}

	if (!CreateBaseTex())
	{
		return false;
	}

	if (!CreateMaterialBuffer())
	{
		return false;
	}
	return true;
}

bool PMDActor::CreateVertexBuffer()
{
	HRESULT Result = S_OK;


	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(PMDVertices.size());

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_VertexBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(Result))
	{
		return false;
	}

	//頂点Map作成
	uint8_t* _VertexMap = nullptr;
	Result = _VertexBuffer->Map(0, nullptr, (void**)&_VertexMap);
	std::copy(PMDVertices.begin(), PMDVertices.end(), _VertexMap);
	_VertexBuffer->Unmap(0, nullptr);

	//頂点バッファビュー作成
	_VertexBufferView.BufferLocation = _VertexBuffer->GetGPUVirtualAddress();

	_VertexBufferView.StrideInBytes = 38;
	_VertexBufferView.SizeInBytes = PMDVertices.size();

	return true;
}

bool PMDActor::CreateIndexBuffer()
{
	HRESULT Result = S_OK;

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(PMDVertexIndex.size() * sizeof(PMDVertexIndex[0]));

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(_IndexBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	//IndexMap作成
	unsigned short* _IndexMap = nullptr;

	Result = _IndexBuffer->Map(0, nullptr, (void**)&_IndexMap);

	if (FAILED(Result))
	{
		return false;
	}
	std::copy(PMDVertexIndex.begin(), PMDVertexIndex.end(), _IndexMap);

	_IndexBuffer->Unmap(0, nullptr);

	//インデクスバッファビュー設定
	_IndexBufferView.BufferLocation = _IndexBuffer->GetGPUVirtualAddress();
	_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;

	_IndexBufferView.SizeInBytes = sizeof(PMDVertexIndex[0])*PMDVertexIndex.size();
	return true;
}

bool PMDActor::CreateMaterialBuffer()
{
	Size WinSize = Application::Instance().GetWindowSize();

	HRESULT Result = S_OK;

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(MaterialForBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)*PMDMaterials.size());

	Result = _Dev->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &ResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_MaterialBuffer.ReleaseAndGetAddressOf()));


	if (FAILED(Result))
	{
		return false;
	}

	uint8_t* MappedMaterial = nullptr;

	Result = _MaterialBuffer->Map(0, nullptr, (void**)& MappedMaterial);

	if (FAILED(Result))
	{
		return false;
	}


	for (auto& Material : PMDMaterials)
	{
		((MaterialForBuffer*)MappedMaterial)->Diffuse = Material.Diffuse;
		((MaterialForBuffer*)MappedMaterial)->Power = Material.Specularity;
		((MaterialForBuffer*)MappedMaterial)->Specular = Material.SpecularColor;
		((MaterialForBuffer*)MappedMaterial)->Ambient = DirectX::XMFLOAT4(Material.MirrorColor.x, Material.MirrorColor.y, Material.MirrorColor.z, 1);
		MappedMaterial += AligmentedValue(sizeof(MaterialForBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	_MaterialBuffer->Unmap(0, nullptr);

	D3D12_DESCRIPTOR_HEAP_DESC MaterialHeapDesc = {};
	MaterialHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	MaterialHeapDesc.NodeMask = 0;
	MaterialHeapDesc.NumDescriptors = PMDMaterials.size() * 5;
	MaterialHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Result = _Dev->CreateDescriptorHeap(&MaterialHeapDesc, IID_PPV_ARGS(_MaterialHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}


	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC TexDesc = {};

	TexDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	TexDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	TexDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	TexDesc.Texture2D.MipLevels = 1;

	ViewDesc.BufferLocation = _MaterialBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes = _MaterialBuffer->GetDesc().Width / PMDMaterials.size();
	auto Handle = _MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	auto HeapStride = _Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	int MaterialNum = 0;

	for (auto& Material : PMDMaterials)
	{
		_Dev->CreateConstantBufferView(&ViewDesc, Handle);
		ViewDesc.BufferLocation += AligmentedValue(sizeof(MaterialForBuffer), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		Handle.ptr += HeapStride;

		if (PMDTextureBuffers[MaterialNum] == nullptr)
		{
			TexDesc.Format = _WhiteTex->GetDesc().Format;
			_Dev->CreateShaderResourceView(_WhiteTex.Get(), &TexDesc, Handle);
		}
		else
		{
			TexDesc.Format = PMDTextureBuffers[MaterialNum]->GetDesc().Format;
			_Dev->CreateShaderResourceView(PMDTextureBuffers[MaterialNum].Get(), &TexDesc, Handle);
		}
		Handle.ptr += HeapStride;

		if (PMDSphTextureBuffers[MaterialNum] == nullptr)
		{
			TexDesc.Format = _WhiteTex->GetDesc().Format;
			_Dev->CreateShaderResourceView(_WhiteTex.Get(), &TexDesc, Handle);
		}
		else
		{
			TexDesc.Format = PMDSphTextureBuffers[MaterialNum]->GetDesc().Format;
			_Dev->CreateShaderResourceView(PMDSphTextureBuffers[MaterialNum].Get(), &TexDesc, Handle);
		}
		Handle.ptr += HeapStride;

		if (PMDSpaTextureBuffers[MaterialNum] == nullptr)
		{
			TexDesc.Format = _BlackTex->GetDesc().Format;
			_Dev->CreateShaderResourceView(_BlackTex.Get(), &TexDesc, Handle);
		}
		else
		{
			TexDesc.Format = PMDSpaTextureBuffers[MaterialNum]->GetDesc().Format;
			_Dev->CreateShaderResourceView(PMDSpaTextureBuffers[MaterialNum].Get(), &TexDesc, Handle);
		}
		Handle.ptr += HeapStride;

		if (PMDToonTextureBuffers[MaterialNum] == nullptr)
		{
			TexDesc.Format = _GradationTex->GetDesc().Format;
			_Dev->CreateShaderResourceView(_GradationTex.Get(), &TexDesc, Handle);
		}
		else
		{
			TexDesc.Format = PMDToonTextureBuffers[MaterialNum]->GetDesc().Format;
			_Dev->CreateShaderResourceView(PMDToonTextureBuffers[MaterialNum].Get(), &TexDesc, Handle);
		}
		Handle.ptr += HeapStride;

		MaterialNum++;
	}


	return true;
}

bool PMDActor::CreateTransformConstantBuffer()
{
	Size WinSize = Application::Instance().GetWindowSize();

	HRESULT Result = S_OK;

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(XMMATRIX), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));


	Result = _Dev->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &ResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_TransformCB.ReleaseAndGetAddressOf()));


	if (FAILED(Result))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC TransHeapDesc = {};
	TransHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	TransHeapDesc.NodeMask = 0;
	TransHeapDesc.NumDescriptors = 2;
	TransHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Result = _Dev->CreateDescriptorHeap(&TransHeapDesc, IID_PPV_ARGS(_TransformHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}


	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc = {};
	ViewDesc.BufferLocation = _TransformCB->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes = _TransformCB->GetDesc().Width;

	_Dev->CreateConstantBufferView(&ViewDesc, _TransformHeap->GetCPUDescriptorHandleForHeapStart());


	//XMMATRIXは行列。
	//初期化はXMMatrixIdentityで初期化する
	_TransformCB->Map(0, nullptr, (void**)&Transform);

	return true;
}

bool 
PMDActor::LoadPMDData(std::string LoadModelpath)
{
	//まずFileを開く
	FILE *Fp = nullptr;
	std::string ModelPath = LoadModelpath;
	fopen_s(&Fp, ModelPath.c_str(), "rb");

	if (Fp == nullptr)
	{
		return false;
	}

	//ヘッダ分シークさせる

	//まず頭のMagic
	fseek(Fp, 3, SEEK_SET);
	//残りのHeaderのByte数
	fseek(Fp, sizeof(PMDHeader), SEEK_CUR);

	//頂点数読み込み
	unsigned long VertexCount;
	fread(&VertexCount, sizeof(VertexCount), 1, Fp);


	PMDVertices.resize(sizeof(PMDVertex)*VertexCount);
	fread(PMDVertices.data(), sizeof(PMDVertex), VertexCount, Fp);

	//頂点インデックスを読み込み
	unsigned int IndexCount = 0;

	fread(&IndexCount, sizeof(IndexCount), 1, Fp);

	PMDVertexIndex.resize(IndexCount);

	fread(PMDVertexIndex.data(), sizeof(unsigned short), IndexCount, Fp);

	//マテリアル数読み込み
	unsigned int MaterialCount = 0;
	fread(&MaterialCount, sizeof(unsigned int), 1, Fp);

	PMDMaterials.resize(MaterialCount);
	fread(PMDMaterials.data(), sizeof(PMDMaterial), MaterialCount, Fp);

	//ボーン読み込み
	unsigned short BoneCount = 0;
	fread(&BoneCount, sizeof(unsigned short), 1, Fp);

	std::vector<PMDBone> BoneDatas(BoneCount);
	fread(BoneDatas.data(), sizeof(PMDBone), BoneDatas.size(), Fp);

	//Fileを閉じる
	fclose(Fp);

	//ボーンの設定
	BoneSetUp(BoneCount, BoneDatas);

	//マテリアルの設定
	MaterialsSetUp(MaterialCount, ModelPath);

	return true;
}

void PMDActor::BoneSetUp(unsigned short BoneCount, std::vector<PMDActor::PMDBone> &BoneDatas)
{
	_BoneMatrices.resize(BoneCount);
	std::fill(_BoneMatrices.begin(), _BoneMatrices.end(), XMMatrixIdentity());

	int Idx = 0;
	for (auto& BoneData : BoneDatas)
	{
		auto& TargetMap = _BoneMap[BoneData.BoneName];
		TargetMap.BoneIndex = Idx;
		TargetMap.StartPos = BoneData.BoneHeadPos;
		TargetMap.EndPos = BoneDatas[BoneData.TailPosBoneIndex].BoneHeadPos;
		Idx++;
	}

	for (auto& Bone : _BoneMap)
	{
		int ParentBoneIndex = BoneDatas[Bone.second.BoneIndex].ParentBoneIndex;
		if (ParentBoneIndex >= BoneDatas.size())
		{
			continue;
		}
		_BoneMap[BoneDatas[ParentBoneIndex].BoneName].Children.emplace_back(&Bone.second);
	}
}

bool PMDActor::CreateBoneBuffer()
{

	HRESULT Result = S_OK;

	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_RESOURCE_DESC ResDesc = CD3DX12_RESOURCE_DESC::Buffer(AligmentedValue(sizeof(XMMATRIX)*_BoneMatrices.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));


	Result = _Dev->CreateCommittedResource(&HeapProp, D3D12_HEAP_FLAG_NONE, &ResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(_BoneBuffer.ReleaseAndGetAddressOf()));


	if (FAILED(Result))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC TransHeapDesc = {};
	TransHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	TransHeapDesc.NodeMask = 0;
	TransHeapDesc.NumDescriptors = 1;
	TransHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Result = _Dev->CreateDescriptorHeap(&TransHeapDesc, IID_PPV_ARGS(_BoneHeap.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}


	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc = {};
	ViewDesc.BufferLocation = _BoneBuffer->GetGPUVirtualAddress();
	ViewDesc.SizeInBytes = _BoneBuffer->GetDesc().Width;

	_Dev->CreateConstantBufferView(&ViewDesc, _BoneHeap->GetCPUDescriptorHandleForHeapStart());


	//XMMATRIXは行列。
	//初期化はXMMatrixIdentityで初期化する
	_BoneBuffer->Map(0, nullptr, (void**)&_MappedBone);

	return true;
}

bool PMDActor::RecursiveMatrixMultiply(PMDBoneNode & Node, DirectX::XMMATRIX & PivotMatrix)
{
	_BoneMatrices[Node.BoneIndex] *= PivotMatrix;
	
	for (auto& ChildNode : Node.Children)
	{
		RecursiveMatrixMultiply(*ChildNode,_BoneMatrices[Node.BoneIndex]);
	}

	return true;
}


bool PMDActor::BoneBend(std::string RootNodeName, const DirectX::XMVECTOR & AngleMatrix,const DirectX::XMVECTOR & PositionMatrix,bool AffectChild)
{
	if (_BoneMap.find(RootNodeName)==_BoneMap.end())
	{
		return false;
	}
	auto& RootBoneNode = _BoneMap[RootNodeName];

	auto Vec = XMLoadFloat3(&RootBoneNode.StartPos);
	auto ReverseBasePosMatrix = XMMatrixTranslationFromVector(XMVectorScale(Vec, -1));
	auto RotationMatrix = XMMatrixRotationQuaternion(AngleMatrix);
	auto Tmp = XMMatrixTranslationFromVector(PositionMatrix);
	_BoneMatrices[RootBoneNode.BoneIndex] = ReverseBasePosMatrix   * RotationMatrix *XMMatrixTranslationFromVector(Vec);
	_BoneMatrices[RootBoneNode.BoneIndex] *= Tmp;
	if (AffectChild)
	{
		for (auto& ChildNode : RootBoneNode.Children)
		{
			RecursiveMatrixMultiply(*ChildNode, _BoneMatrices[RootBoneNode.BoneIndex]);
		}
	}
	return true;
}

float PMDActor::GetYFromXOnBezier(float x, const DirectX::XMFLOAT2 & a, const DirectX::XMFLOAT2 & b, uint8_t n)
{
	if (a.x==a.y&&b.x==b.y)
	{
		//両方のXとYが同じである場合、変化がない(線形補間なので計算不要)
		return x;
	}

	float T = x;
	//t^3の係数
	float ConstantExp_1 = 1 + 3 * a.x - 3 * b.x;
	//t^2の係数
	float ConstantExp_2 = 3*b.x-6*a.x;
	//t^1の係数
	float ConstantExp_3 = 3 * a.x;

	//誤差の範囲内と考えられる定数
	const float Tolerance = 0.0005f;

	for (int i=0;i<n;i++)
	{
		//f(t)を求める
		float Ft = (ConstantExp_1 * T*T*T) + (ConstantExp_2*T*T) + ConstantExp_3 * T - x;

		if (Ft<=Tolerance&&Ft>=-Tolerance)
		{
			//許容範囲内であればBreak
			break;
		}

		//f'(t)を求める
		float Fdt = (3 * ConstantExp_1*T*T) + (2 * ConstantExp_2*T) + ConstantExp_3;

		if (Fdt==0)
		{
			//Fdtが0の時、割れないのでBreak
			break;
		}

		T -= Ft / Fdt;
	}

	float R = 1 - T;
	return (T*T*T) + (3 * T*T*R*b.y) + (3 * T*R*R*a.y);
}

void PMDActor::MaterialsSetUp(unsigned int MaterialCount, std::string &ModelPath)
{
	//各種マテリアル関連Vectorリサイズ
	PMDTexutrePathVec.resize(MaterialCount);

	PMDTextureBuffers.resize(MaterialCount);
	PMDSphTextureBuffers.resize(MaterialCount);
	PMDSpaTextureBuffers.resize(MaterialCount);
	PMDToonTextureBuffers.resize(MaterialCount);

	int Counter = 0;
	//各種マテリアル関連Vectorリサイズ

	std::string ToonRootPath = "toon/";
	for (auto& Material : PMDMaterials)
	{
		std::string TexturePath = Material.TextureFileName;
		std::string Extension = "";

		//テクスチャのパスをロード
		//スプリッタが含まれているかどうかチェック
		auto PathVector = SplitFilePath(TexturePath, '*');

		for (int i = 0; i < PathVector.size(); i++)
		{
			auto Extension = GetExtension(PathVector[i]);

			if (Extension == "sph")
			{
				PMDTexutrePathVec[Counter].Sph = GetTexturePathFromModelAndTexturePath(ModelPath, PathVector[i].c_str());
				continue;
			}

			if (Extension == "spa")
			{
				PMDTexutrePathVec[Counter].Spa = GetTexturePathFromModelAndTexturePath(ModelPath, PathVector[i].c_str());
				continue;
			}

			PMDTexutrePathVec[Counter].Tex = GetTexturePathFromModelAndTexturePath(ModelPath, PathVector[i].c_str());
		}

		//Toonのパスをロード、作成
		char ToonFilePath[16];
		sprintf_s(ToonFilePath, "toon%02d.bmp", Material.ToonIndex + 1);
		PMDTexutrePathVec[Counter].Toon = ToonRootPath + ToonFilePath;
		Counter++;
	}

	std::fill(PMDTextureBuffers.begin(), PMDTextureBuffers.end(), Microsoft::WRL::ComPtr <ID3D12Resource>());
	std::fill(PMDSphTextureBuffers.begin(), PMDSphTextureBuffers.end(), Microsoft::WRL::ComPtr <ID3D12Resource>());
	std::fill(PMDSpaTextureBuffers.begin(), PMDSpaTextureBuffers.end(), Microsoft::WRL::ComPtr <ID3D12Resource>());

	auto& _TexLoader = TexLoader::Instance();
	//マテリアル読み込み
	for (int i = 0; i < PMDTexutrePathVec.size(); i++)
	{
		if (PMDTexutrePathVec[i].Tex != "")
		{
			PMDTextureBuffers[i] = _TexLoader.LoadTextureFromFile(_Dev,PMDTexutrePathVec[i].Tex);
		}

		if (PMDTexutrePathVec[i].Sph != "")
		{
			PMDSphTextureBuffers[i] = _TexLoader.LoadTextureFromFile(_Dev, PMDTexutrePathVec[i].Sph);
		}

		if (PMDTexutrePathVec[i].Spa != "")
		{
			PMDSpaTextureBuffers[i] = _TexLoader.LoadTextureFromFile(_Dev, PMDTexutrePathVec[i].Spa);
		}


		if (PMDTexutrePathVec[i].Toon != "")
		{
			PMDToonTextureBuffers[i] = _TexLoader.LoadTextureFromFile(_Dev, PMDTexutrePathVec[i].Toon);
		}
	}
}


bool PMDActor::CreateBaseTex()
{
	if (!CreateWhiteTex())
	{
		return false;
	}

	if (!CreateBlackTex())
	{
		return false;
	}

	if (!CreateGradationTex())
	{
		return false;
	}

	return true;
}

bool PMDActor::CreateWhiteTex()
{
	HRESULT Result = S_OK;

	//WriteToSubresource方式
	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(_WhiteTex.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	std::vector<unsigned char> ColData(4 * 4 * 4);
	std::fill(ColData.begin(), ColData.end(), 0xff);

	Result = _WhiteTex->WriteToSubresource(0, nullptr, ColData.data(), 4 * 4, ColData.size());

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

bool PMDActor::CreateBlackTex()
{
	HRESULT Result = S_OK;

	//WriteToSubresource方式
	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 4);

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(_BlackTex.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	std::vector<unsigned char> ColData(4 * 4 * 4);
	std::fill(ColData.begin(), ColData.end(), 0x0);

	Result = _BlackTex->WriteToSubresource(0, nullptr, ColData.data(), 4 * 4, ColData.size());

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

bool PMDActor::CreateGradationTex()
{
	HRESULT Result = S_OK;

	//WriteToSubresource方式
	D3D12_HEAP_PROPERTIES HeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);

	D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 4, 256);

	Result = _Dev->CreateCommittedResource(&HeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(_GradationTex.ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return false;
	}

	struct Color
	{
		uint8_t r, g, b, a;
		Color(uint8_t inr, uint8_t ing, uint8_t inb, uint8_t ina) :
			r(inr), g(ing), b(inb), a(ina) {}
		Color() {};
	};

	std::vector<Color> ColData(4 * 256);

	uint8_t BaseCol = 256;

	for (auto It = ColData.begin(); It != ColData.end(); It = It + 4)
	{
		std::fill(It, It + 4, Color(BaseCol, BaseCol, BaseCol, 255));
		BaseCol--;
	}

	Result = _GradationTex->WriteToSubresource(0, nullptr, ColData.data(), sizeof(Color) * 4, sizeof(Color)*ColData.size());

	if (FAILED(Result))
	{
		return false;
	}

	return true;
}

//相対パスを作成します
std::string 
PMDActor::GetTexturePathFromModelAndTexturePath(const std::string & ModelPath, const char * TexturePath)
{
	//"\"または"/"のどちらかがフォルダセパレータなので、どっちも検索
	int PathIndex_1 = ModelPath.rfind("/");
	int PathIndex_2 = ModelPath.rfind("\\");
	//rFindで見つからなかったら-1が帰るため、大きいほうに絞れば良い。
	int PathIndex = max(PathIndex_1, PathIndex_2);

	if (PathIndex_1 == -1 && PathIndex_2 == -1)
	{
		return "";
	}

	auto FolderPath = ModelPath.substr(0, PathIndex + 1);

	return FolderPath + TexturePath;
}