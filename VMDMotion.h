#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include <map>
#include <string>
class VMDMotion
{
public:
#pragma pack(1)
	struct VmdHeader
	{
		char VmdName[30];
		char VmdModelname[20];
	};

	struct VmdMotionType
	{
		char BoneName[15];
		unsigned int FrameNo;
		DirectX::XMFLOAT3 Location;
		DirectX::XMFLOAT4 Rotation;
		unsigned char Interpolation[64];
	};
#pragma pack()

public:
	VMDMotion();
	~VMDMotion();

	std::map <std::string, std::vector<VmdMotionType>>& GetMotionData();
	int GetDuration();
	bool LoadVMDData(std::string VMDPath);
private:
	//モーション格納
	std::map <std::string, std::vector<VmdMotionType>> VmdMotionData;
	//最終フレーム数
	int Duration;
};

