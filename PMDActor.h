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
	//38バイト
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
		float Specularity;		//光沢率 4
		DirectX::XMFLOAT3 SpecularColor;//光沢色 12
		DirectX::XMFLOAT3 MirrorColor;  //環境色 12
		byte ToonIndex;			//		1
		byte EdgeFlag;		   //輪郭	1
		unsigned FaceVertexCount;//面頂点数(マテリアルで使う頂点リストのデータ数) 4
		char TextureFileName[20];//テクスチャ、マテリアル数20
	};

	struct PMDBone
	{
		char BoneName[20];
		unsigned short	ParentBoneIndex;	//ない場合は0xffff
		unsigned short	TailPosBoneIndex;	//末尾だと0
		unsigned char	BoneType;
		unsigned short	IkParentBoneIndex;	//ない場合だと0
		DirectX::XMFLOAT3 BoneHeadPos;//X,Y,Z ボーンのヘッド位置
	};
#pragma pack()

	PMDActor(Microsoft::WRL::ComPtr<ID3D12Device>& dev, Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& CmdList);
	~PMDActor();

	const std::vector<uint8_t>&		GetVertices();
	const std::vector<uint16_t>&	GetVertexIndex();
	const std::vector<PMDMaterial>&	GetMaterials();
	const std::vector<TexturePaths>&GetTexutrePathVec();

	//初期化
	bool Init(std::string LoadModelpath, std::string LoadVmdpath);

	void Update();

	void MotionUpDate(int FrameNum);

	void Draw(bool DrawShadow=false);

	//マテリアルの数を調べます
	const unsigned int GetMaterialSize();

	void MoveWorldAngle(float Angle);
	void SetWorldAngle(float Angle);

	void SetPosition(float x, float y,float z);
	void MovePosition(float x, float y, float z);

private:

	float YAngle;
	DirectX::XMFLOAT3 Position;
	std::unique_ptr<VMDMotion> _MotionClass;

	//デバイス
	Microsoft::WRL::ComPtr<ID3D12Device>& _Dev;
	//コマンドリスト
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> _CmdList;

	//頂点バッファ作成
	bool CreateVertexBuffer();
	//頂点バッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _VertexBuffer;
	//頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW _VertexBufferView;


	//インデクスバッファ作成
	bool CreateIndexBuffer();
	//インデックスバッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _IndexBuffer;
	//インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW _IndexBufferView;
	//頂点番号
	std::vector<unsigned short> Indices;

	//マテリアルバッファの作成
	bool CreateMaterialBuffer();
	//マテリアルバッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _MaterialBuffer;
	//マテリアルヒープ
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _MaterialHeap;


	//トランスフォーム
	DirectX::XMMATRIX* Transform;

	//座標変換定数バッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _TransformCB;
	//座標変換ヒープ
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _TransformHeap;

	//座標変換用定数バッファ及び、バッファビューを作成
	bool CreateTransformConstantBuffer();

	std::vector<uint8_t>PMDVertices;
	std::vector<uint16_t>PMDVertexIndex;
	std::vector<PMDMaterial>PMDMaterials;
	std::vector<TexturePaths>PMDTexutrePathVec;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDTextureBuffers;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDSphTextureBuffers;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDSpaTextureBuffers;
	std::vector<Microsoft::WRL::ComPtr <ID3D12Resource>>PMDToonTextureBuffers;


	//PMDのロードを行う
	bool LoadPMDData(std::string LoadModelpath);

	void BoneSetUp(unsigned short BoneCount, std::vector<PMDActor::PMDBone> &BoneDatas);

	//ボーン行列
	std::vector<DirectX::XMMATRIX> _BoneMatrices;
	//ボーン構造マップ
	std::map<std::string, PMDBoneNode> _BoneMap;
	//転送用のMappedBone
	DirectX::XMMATRIX* _MappedBone;

	//ボーンバッファ
	Microsoft::WRL::ComPtr <ID3D12Resource> _BoneBuffer;
	//ボーンヒープ
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _BoneHeap;

	//ボーンバッファ作成
	bool CreateBoneBuffer();

	//親ボーンから子のボーンまでを曲げる関数
	bool RecursiveMatrixMultiply(PMDBoneNode& Node, DirectX::XMMATRIX& PivotMatrix);

	bool BoneBend(std::string RootNodeName,const DirectX::XMVECTOR& AngleMatrix, const DirectX::XMVECTOR & PositionMatrix,bool AffectChild=true);

	//ベジェ曲線補間
	float GetYFromXOnBezier(float x,const DirectX::XMFLOAT2& a,const DirectX::XMFLOAT2& b, uint8_t n);

	//マテリアルのロードを行う
	void MaterialsSetUp(unsigned int MaterialCount, std::string &ModelPath);


	//白、黒、グレーのテクスチャをまとめて作る
	bool CreateBaseTex();
	//白テクスチャを作る
	bool CreateWhiteTex();
	//黒テクスチャを作る
	bool CreateBlackTex();
	//グラデーションテクスチャを作る
	bool CreateGradationTex();

	//白テクスチャ
	Microsoft::WRL::ComPtr <ID3D12Resource> _WhiteTex;
	//黒テクスチャ
	Microsoft::WRL::ComPtr <ID3D12Resource> _BlackTex;
	//Toonシェーダー用グラデーションテクスチャ
	Microsoft::WRL::ComPtr <ID3D12Resource> _GradationTex;

	//モデルのパスとテクスチャパスの合成パスを取得
	//アプリケーションから見たテクスチャの相対パスを返す
	std::string GetTexturePathFromModelAndTexturePath(const std::string& ModelPath, const char* TexturePath);
};

