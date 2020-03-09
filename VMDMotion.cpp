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
	//�ꎞ�I�Ƀ��[�V�����f�[�^���i�[����ϐ�
	std::vector<VmdMotionType>VmdData;

	//�܂�File���J��
	FILE *Fp = nullptr;
	std::string ModelPath = VMDPath;
	fopen_s(&Fp, ModelPath.c_str(), "rb");

	if (Fp == nullptr)
	{
		return false;
	}

	//�w�b�_���V�[�N������

	//Header��Byte���i�߂�
	fseek(Fp, sizeof(VmdHeader), SEEK_CUR);

	//���_���ǂݍ���
	unsigned int MotionCount;
	fread(&MotionCount, sizeof(MotionCount), 1, Fp);

	VmdData.resize(MotionCount);
	fread(VmdData.data(), sizeof(VmdMotionType), MotionCount, Fp);

	//File�����
	fclose(Fp);

	Duration = 0;

	//�}�b�v�Ƃ��Đ�������
	for (auto& KeyFrame:VmdData)
	{
		VmdMotionData[KeyFrame.BoneName].emplace_back(KeyFrame);
		if (Duration<KeyFrame.FrameNo)
		{
			Duration = KeyFrame.FrameNo;
		}
	}
	
	//���ꂼ���Vector���t���[���ԍ��Ń\�[�g���Ă���
	for (auto& MotionData : VmdMotionData)
	{
		auto& AnimationVector = MotionData.second;
		std::sort(AnimationVector.begin(), AnimationVector.end(), [](VmdMotionType& a, VmdMotionType& b) {return a.FrameNo < b.FrameNo; });
	}

	return true;
}
