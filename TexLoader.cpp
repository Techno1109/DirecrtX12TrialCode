#include "TexLoader.h"
#include <DirectXTex.h>
#include <d3dx12.h>
#include "Tool.h"
#pragma comment(lib,"DirectXTex.lib")


using namespace DirectX;

TexLoader::TexLoader()
{
}


TexLoader& TexLoader::Instance()
{
	static TexLoader instance;
	return instance;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TexLoader::LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device>& _Dev,std::string & TexPath)
{
	//読み込み済みかチェックする

	auto SearchIt = _ResourceTable.find(TexPath);

	//検索にヒットした場合は読み込み済みのリソースを返す
	if (SearchIt != _ResourceTable.end())
	{
		return SearchIt->second;
	}

	HRESULT Result = S_OK;

	TexMetadata metaData = {};
	ScratchImage scratchImage = {};

	std::map<std::string, std::function<HRESULT(const std::wstring&, TexMetadata*, ScratchImage&)> >LoadLambdaTable;

	LoadLambdaTable["sph"] = LoadLambdaTable["spa"] = LoadLambdaTable["bmp"] = LoadLambdaTable["png"] = LoadLambdaTable["jpg"] = [](const std::wstring& FilePath, TexMetadata* TargetMetaData, ScratchImage& TargetScratchImage)
		->HRESULT
	{
		return LoadFromWICFile(FilePath.c_str(), 0, TargetMetaData, TargetScratchImage);
	};

	LoadLambdaTable["tga"] = [](const std::wstring& FilePath, TexMetadata* TargetMetaData, ScratchImage& TargetScratchImage)
		->HRESULT
	{
		return LoadFromTGAFile(FilePath.c_str(), TargetMetaData, TargetScratchImage);
	};

	LoadLambdaTable["dds"] = [](const std::wstring& FilePath, TexMetadata* TargetMetaData, ScratchImage& TargetScratchImage)
		->HRESULT
	{
		return LoadFromDDSFile(FilePath.c_str(), 0, TargetMetaData, TargetScratchImage);
	};

	auto Extension = GetExtension(TexPath);

	if (Extension == "")
	{
		return nullptr;
	}

	Result = LoadLambdaTable[Extension](GetWideString(TexPath).c_str(), &metaData, scratchImage);


	if (FAILED(Result))
	{
		return nullptr;
	}

	auto Img = scratchImage.GetImage(0, 0, 0);

	D3D12_HEAP_PROPERTIES TexHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);


	D3D12_RESOURCE_DESC ResourceDesc{};
	ResourceDesc.Format = metaData.format;
	ResourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metaData.dimension);
	ResourceDesc.Width = metaData.width;
	ResourceDesc.Height = metaData.height;
	ResourceDesc.DepthOrArraySize = metaData.arraySize;
	ResourceDesc.MipLevels = metaData.mipLevels;
	ResourceDesc.Format = metaData.format;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	ResourceDesc.Alignment = 0;

	_ResourceTable[TexPath] = nullptr;

	Result = _Dev->CreateCommittedResource(&TexHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(_ResourceTable[TexPath].ReleaseAndGetAddressOf()));

	if (FAILED(Result))
	{
		return nullptr;
	}

	Result = _ResourceTable[TexPath]->WriteToSubresource(0, nullptr, Img->pixels, Img->rowPitch, Img->slicePitch);

	if (FAILED(Result))
	{
		return nullptr;
	}

	return _ResourceTable[TexPath];
}

TexLoader::~TexLoader()
{
}
