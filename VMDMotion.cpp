#include "VMDMotion.h"
#include <algorithm>

VMDMotion::VMDMotion()
{
}

VMDMotion::~VMDMotion()
{
}

std::map <std::string, std::vector<VMDMotion::VmdMotionType>>& VMDMotion::GetMotionData()
{
	return VmdMotionData;
}

int VMDMotion::GetDuration()
{
	return Duration;
}

bool VMDMotion::LoadVMDData(std::string VMDPath)
{
	//一時的にモーションデータを格納する変数
	std::vector<VmdMotionType>VmdData;

	//まずFileを開く
	FILE *Fp = nullptr;
	std::string ModelPath = VMDPath;
	fopen_s(&Fp, ModelPath.c_str(), "rb");

	if (Fp == nullptr)
	{
		return false;
	}

	//ヘッダ分シークさせる

	//HeaderのByte数進める
	fseek(Fp, sizeof(VmdHeader), SEEK_CUR);

	//頂点数読み込み
	unsigned int MotionCount;
	fread(&MotionCount, sizeof(MotionCount), 1, Fp);

	VmdData.resize(MotionCount);
	fread(VmdData.data(), sizeof(VmdMotionType), MotionCount, Fp);

	//Fileを閉じる
	fclose(Fp);

	Duration = 0;

	//マップとして整理する
	for (auto& KeyFrame:VmdData)
	{
		VmdMotionData[KeyFrame.BoneName].emplace_back(KeyFrame);
		if (Duration<KeyFrame.FrameNo)
		{
			Duration = KeyFrame.FrameNo;
		}
	}
	
	//それぞれのVectorをフレーム番号でソートしておく
	for (auto& MotionData : VmdMotionData)
	{
		auto& AnimationVector = MotionData.second;
		std::sort(AnimationVector.begin(), AnimationVector.end(), [](VmdMotionType& a, VmdMotionType& b) {return a.FrameNo < b.FrameNo; });
	}

	return true;
}
