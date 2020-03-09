#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <map>
#include <memory>
#include <array>
#include <EffekseerRendererDX12.h>
#include<SpriteFont.h>
#include<ResourceUploadBatch.h>
#pragma comment(lib,"DirectXTex.lib")

//DirectX12�̏��������̊e�v�f�����܂Ƃ߂邽�߂̃N���X
class PassRenderer;
class NormalMappingPass;
class GaussPass;
class PlanePass;
class ShadowMapPass;
class PrimitiveRenderer;
class PrimitiveBase;
class SSAOPass;
class RayMarchingPass;
class Dx12Wrapper
{
public:
	Dx12Wrapper(HWND windowH);
	~Dx12Wrapper();

	bool Init();

	void SetRootSignature(Microsoft::WRL::ComPtr <ID3D12RootSignature>& RootSigunature);
	void SetPipeLineState(Microsoft::WRL::ComPtr <ID3D12PipelineState>& PipelineState);
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

	Microsoft::WRL::ComPtr<ID3D12Device>& GetDevice();
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList>& GetCmdList();
	Microsoft::WRL::ComPtr <ID3D12CommandQueue>& GetCmdQueue();

	void SetFov(float Angle);
	void SetEyePosition(float x, float y, float z);
	void SetTargetPosition(float x, float y, float z);

	void MoveFov(float Angle);
	void MoveEyePosition(float x, float y, float z);
	void MoveTargetPosition(float x, float y, float z);
	void MoveOffset(float x, float y, float z);


	//�e�p�`��
	void ShadowDrawSetting();

	void ShadowDraw();

	bool DrawPrimitiveShapes();

	//��ʂ̃��Z�b�g
	bool ScreenCrear();
	//�`��O�ݒ�
	void DrawSetting();
	//�`��I��(�R�}���h���s)
	void EndDraw();
	//1stPath�`��
	void Draw();
	//��ʂ̍X�V
	void ScreenFlip();

	void StartEffect();
private:
	struct TransformMatrix
	{
		DirectX::XMMATRIX Camera;//View&Projection
		DirectX::XMFLOAT3 Eye;
		DirectX::XMFLOAT3 LightPos;
		DirectX::XMMATRIX Shadow;
		DirectX::XMMATRIX LightCamera;	//�J�������猩���r���[�v���W�F�N�V����
		DirectX::XMMATRIX Projection;
		DirectX::XMMATRIX InProjection;
	};

//�������牺�ő���C--------------------------------
	struct DrawBuffers
	{
		Microsoft::WRL::ComPtr <ID3D12Resource> _MainBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _NormalBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _BloomBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _ShrinkBloomBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _FovBuffer;
		Microsoft::WRL::ComPtr <ID3D12Resource> _ShrinkFovBuffer;
	};

	struct DrawBufferHeaps
	{
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Rtv_DescriptorHeap;
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Srv_DescriptorHeap;
	};
	
	struct SSAOBufferAndHeaps
	{
		Microsoft::WRL::ComPtr <ID3D12Resource> _Buffer;
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Rtv_DescriptorHeap;
		Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Srv_DescriptorHeap;
	};

	SSAOBufferAndHeaps SSAODatas;
	//�`��o�b�t�@�[2��
	std::array<std::array<Microsoft::WRL::ComPtr <ID3D12Resource>, 6>, 2>Buffers;
	//�`��o�b�t�@�[�pHeap2�Z�b�g
	std::array<DrawBufferHeaps, 2> BufferHeaps;
	std::unique_ptr<SSAOPass> _SSAORender;
	bool Create_Rtv_Srvs();
	bool Create_SSAO_Rtv_Srv();
	void DrawSSAO();
	void AddBarrier(int PassNum, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
	void PassClearAndSetRendererTarget(int Num,bool First);

//�����܂łŊ撣��------------------------

	//�e�`��p
	std::unique_ptr<ShadowMapPass> _ShadowPass;
	//�v���~�e�B�u�`��p
	std::unique_ptr<PrimitiveRenderer> _PrimitiveRender;

	//�p�X�`��p
	std::vector<std::shared_ptr<PassRenderer>> RendererPasses;

	//�E�B���h�E�n���h��-----------
	HWND _WindowH;

	//�L��������Ă���p�X(0�X�^�[�g)
	int TotalActivePass;

	int _EffekseerHandle;
	Microsoft::WRL::ComPtr	< EffekseerRenderer::Renderer> _EffekseerRenderer;
	Microsoft::WRL::ComPtr	< Effekseer::Manager> _EffekseerManager;
	Microsoft::WRL::ComPtr	< EffekseerRenderer::SingleFrameMemoryPool> _EffekseerMemoryPool;
	Microsoft::WRL::ComPtr	< EffekseerRenderer::CommandList> _EffekseerCommandList;

	Microsoft::WRL::ComPtr<Effekseer::Effect> _Effect;

	void EffekseerInit();

	void EffekseerDraw();

	//IMGUI
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _ImGuiHeap;

	//DXTK
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _DxtkHeap;
	std::shared_ptr < DirectX::GraphicsMemory> _Gmemory = nullptr;//�O���t�B�N�X�������I�u�W�F�N�g 
	std::shared_ptr < DirectX::SpriteFont> _SpriteFont = nullptr;//�t�H���g�\���p�I�u�W�F�N�g 
	std::shared_ptr < DirectX::SpriteBatch> _SpriteBatch = nullptr;//�X�v���C�g�\���p�I�u�W�F�N�g 

	//DXGI-----------
	Microsoft::WRL::ComPtr<IDXGIFactory6> _DxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> _DxgiSwapChain;

	//�f�o�C�X-----------
	Microsoft::WRL::ComPtr<ID3D12Device> _Dev;

	//Primitive
	std::vector<std::unique_ptr<PrimitiveBase>> Primitives;
	bool CreatePrimitives();

	//���[�g�V�O�l�`���n
	//���[�g�V�O�l�`��
	Microsoft::WRL::ComPtr <ID3D12RootSignature> _RootSigunature;

	//�p�C�v���C���X�e�[�g
	Microsoft::WRL::ComPtr <ID3D12PipelineState> _PipelineState;



	//�����_�[�^�[�Q�b�gView�쐬
	bool CreateRenderTargetView();
	//�����_�[�^�[�Q�b�g�p�f�X�N���v�^�q�[�v
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _Rtv_DescriptorHeap;
	//�X���b�v�`�F�C���̃��\�[�X�|�C���^�Q
	std::vector< Microsoft::WRL::ComPtr<ID3D12Resource>> _BackBuffers;


	//�R�}���h���X�g�쐬
	bool CreateCommandList();
	//�R�}���h���X�g�m�ۗp�I�u�W�F�N�g
	Microsoft::WRL::ComPtr <ID3D12CommandAllocator> _CmdAlloc;
	//�R�}���h���X�g�{��
	Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList> _CmdList;
	//�R�}���h���X�g�̃L���[
	Microsoft::WRL::ComPtr <ID3D12CommandQueue> _CmdQueue;

	//�k�x�o�b�t�@���쐬
	bool CreateDepthBuffer();

	//�[�x�e�N�X�`��
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _DepthBufferHeap;
	//�[�x�o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _DepthBuffer;
	//�V���h�E�}�b�v�p�[�x�o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _LightDepthBuffer;
	//�[�x�X�e���V���r���[�q�[�v
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _DepthStencilViewHeap;

	//�r���[�|�[�g���쐬
	bool SetViewPort();
	//�r���[�|�[�g
	D3D12_VIEWPORT _ViewPort;
	//�V�U�[
	D3D12_RECT _ScissorRect;


	//���W�ϊ��p�萔�o�b�t�@�y�сA�o�b�t�@�r���[���쐬
	bool CreateTransformConstantBuffer();
	//���W�ϊ��萔�o�b�t�@
	Microsoft::WRL::ComPtr <ID3D12Resource> _TransformCB;
	//���W�ϊ��q�[�v
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> _TransformHeap;
	//�`��}�g���N�X
	TransformMatrix* MappedMatrix;


	DirectX::XMMATRIX GetCurrentCameraMatrix();
	//���_
	DirectX::XMFLOAT3 Eye;
	//�����_
	DirectX::XMFLOAT3 Target;
	//���_
	DirectX::XMFLOAT3 Offset;
	//��x�N�g�����K�v
	DirectX::XMFLOAT3 UpperVec;
	//���f���̏ꏊ
	DirectX::XMFLOAT3 ActorPos = DirectX::XMFLOAT3(0,0,0);
	//��p
	float _Fov = DirectX::XM_PIDIV4;

	//�|�[�����O�ҋ@
	void WaitWithFence();

	//�o���A�ǉ�
	void AddBarrier(Microsoft::WRL::ComPtr<ID3D12Resource>& Buffer,D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
	//�t�F���X�I�u�W�F�N�g-----------
	Microsoft::WRL::ComPtr <ID3D12Fence> _Fence;
	//�t�F���X�l
	UINT64 _FenceValue;

};
