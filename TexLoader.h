#pragma once
#include <wrl/client.h>
#include <d3d12.h>
#include <string>
#include <map>
class TexLoader
{
public:
	//シングルトンパターン
	static TexLoader& Instance();

	Microsoft::WRL::ComPtr<ID3D12Resource> LoadTextureFromFile(Microsoft::WRL::ComPtr<ID3D12Device>& _Dev,std::string & TexPath);

	~TexLoader();
private:
	TexLoader();
	TexLoader(const TexLoader&);
	void operator=(const TexLoader&);

	std::map<std::string, Microsoft::WRL::ComPtr <ID3D12Resource>> _ResourceTable;
};

