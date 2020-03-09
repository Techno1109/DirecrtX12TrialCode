#pragma once
#include <vector>
#include <wrl/client.h>
#include <map>
#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>

class VMDMotion;
class PMDActor
{

public:
	struct PMDHeader
	{
		float Version; //4byte
		char ModelName[20];//20byte = 4*5byte
		char Comment[256];//256byte =4*64byte;
	};

	struct TexturePaths
	{
		std::string Tex;
		std::string Sph;
		std::string Spa;
		std::string Toon;
	};

	struct MaterialForBuffer
	{
		DirectX::XMFLOAT4 Diffuse;
		float Power;
		DirectX::XMFLOAT3 Specular;
		DirectX::XMFLOAT4 Ambient;
	};

	struct PMDBoneNode
	{
		int BoneIndex;
		DirectX::XMFLOAT3 StartPos;
		DirectX::XMFLOAT3 EndPos;
		std::vector<PMDBoneNode*> Children;
	};
#pragma pack(1)
	//38�o�C�g
	struct PMDVertex
	{
		float Pos[3];
		float Normal_Vec[3];
		float Uv[2];
		WORD BoneNum[2];
		BYTE BoneWeight;
		BYTE EdgeFlag;
	};

	struct PMDMaterial
	{
		DirectX::XMFLOAT4 Diffuse;
		float Specularity;		//���� 4
		DirectX::XMFLOAT3 SpecularColor;//����F 12
		DirectX::XMFLOAT3 MirrorColor;  //���F 12
		byte ToonIndex;			//		1
		byte EdgeFlag;		   //�֊s	1
		unsigned FaceVertexCount;//�ʒ��_��(�}�e���A���Ŏg�����_���X�g�̃f�[�^��) 4
		char TextureFileName[20];//�e�N�X�`���A�}�e���A����20
	};

	struct PMDBone
	{
		char BoneName[20];
		unsigned short	ParentBoneIndex;	//�Ȃ��ꍇ��0xffff
		unsigned short	TailPosBoneIndex;	//��������0
		unsigned char	BoneType;
		unsigned short	IkParentBoneIndex;	//�Ȃ��ꍇ����0
		DirectX::XMFLOAT3 BoneHeadPos;//X,Y,Z �{�[���̃w�b�h�ʒu
	};
#pragma pack()

	PMDActor(Microsoft::WRL::ComPtr<ID3D12Device>& dev, Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& CmdList);
	~PMDActor();

	const std::vector<uint8_t>&		GetVertices();
	const std::vector<uint16_t>&	GetVertexIndex();
	const std::vector<PMDMaterial>&	GetMaterials();
	const std::vector<TexturePaths>&GetTexutrePathVec();

	//������
	bool Init(std::string LoadModelpath, std::string LoadVmdpath);

	void Update();

	void MotionUpDate(int FrameNum);

	void Draw(bool DrawShadow=false);

	//�}�e���A���̐��𒲂ׂ܂�
	const unsigned int GetMaterialSize();

	void MoveWorldAngle(float Angle);
	void SetWorldAngle(float Angle);

	void SetPosition(float x, float y,float z);
	void MovePosition(float x, float y, float z);

private:

	float YAngle;
	DirectX::XMFLOAT3 Position;
	std::unique_ptr<VMDMotion> _MotionClass;

	//�f�o�C�X
	Microsoft::WRL::ComPtr<ID3D12Device>& _Dev;
	//�R�}���h���X�g
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> _CmdList;

	//���_�o�b�t�@�쐬
	bool CreateVertexBuffer();
	//���_�o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _VertexBuffer;
	//���_�o�b�t�@�r���[
	D3D12_VERTEX_BUFFER_VIEW _VertexBufferView;


	//�C���f�N�X�o�b�t�@�쐬
	bool CreateIndexBuffer();
	//�C���f�b�N�X�o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _IndexBuffer;
	//�C���f�b�N�X�o�b�t�@�r���[
	D3D12_INDEX_BUFFER_VIEW _IndexBufferView;
	//���_�ԍ�
	std::vector<unsigned short> Indices;

	//�}�e���A���o�b�t�@�̍쐬
	bool CreateMaterialBuffer();
	//�}�e���A���o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _MaterialBuffer;
	//�}�e���A���q�[�v
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _MaterialHeap;


	//�g�����X�t�H�[��
	DirectX::XMMATRIX* Transform;

	//���W�ϊ��萔�o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _TransformCB;
	//���W�ϊ��q�[�v
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _TransformHeap;

	//���W�ϊ��p�萔�o�b�t�@�y�сA�o�b�t�@�r���[���쐬
	bool CreateTransformConstantBuffer();

	std::vector<uint8_t>PMDVertices;
	std::vector<uint16_t>PMDVertexIndex;
	std::vector<PMDMaterial>PMDMaterials;
	std::vector<TexturePaths>PMDTexutrePathVec;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDTextureBuffers;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDSphTextureBuffers;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDSpaTextureBuffers;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDToonTextureBuffers;


	//PMD�̃��[�h���s��
	bool LoadPMDData(std::string LoadModelpath);

	void BoneSetUp(unsigned short BoneCount, std::vector<PMDActor::PMDBone> &BoneDatas);

	//�{�[���s��
	std::vector<DirectX::XMMATRIX> _BoneMatrices;
	//�{�[���\���}�b�v
	std::map<std::string, PMDBoneNode> _BoneMap;
	//�]���p��MappedBone
	DirectX::XMMATRIX* _MappedBone;

	//�{�[���o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _BoneBuffer;
	//�{�[���q�[�v
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _BoneHeap;

	//�{�[���o�b�t�@�쐬
	bool CreateBoneBuffer();

	//�e�{�[������q�̃{�[���܂ł��Ȃ���֐�
	bool RecursiveMatrixMultiply(PMDBoneNode& Node, DirectX::XMMATRIX& PivotMatrix);

	bool BoneBend(std::string RootNodeName,const DirectX::XMVECTOR& AngleMatrix, const DirectX::XMVECTOR & PositionMatrix,bool AffectChild=true);

	//�x�W�F�Ȑ����
	float GetYFromXOnBezier(float x,const DirectX::XMFLOAT2& a,const DirectX::XMFLOAT2& b, uint8_t n);

	//�}�e���A���̃��[�h���s��
	void MaterialsSetUp(unsigned int MaterialCount, std::string &ModelPath);


	//���A���A�O���[�̃e�N�X�`�����܂Ƃ߂č��
	bool CreateBaseTex();
	//���e�N�X�`�������
	bool CreateWhiteTex();
	//���e�N�X�`�������
	bool CreateBlackTex();
	//�O���f�[�V�����e�N�X�`�������
	bool CreateGradationTex();

	//���e�N�X�`��
	Microsoft::WRL::ComPtr <ID3D12Resource> _WhiteTex;
	//���e�N�X�`��
	Microsoft::WRL::ComPtr <ID3D12Resource> _BlackTex;
	//Toon�V�F�[�_�[�p�O���f�[�V�����e�N�X�`��
	Microsoft::WRL::ComPtr <ID3D12Resource> _GradationTex;

	//���f���̃p�X�ƃe�N�X�`���p�X�̍����p�X���擾
	//�A�v���P�[�V�������猩���e�N�X�`���̑��΃p�X��Ԃ�
	std::string GetTexturePathFromModelAndTexturePath(const std::string& ModelPath, const char* TexturePath);
};

