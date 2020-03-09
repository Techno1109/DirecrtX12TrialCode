#pragma once
#include <d3d12.h>
#include <wrl/client.h>

class PMDRenderer
{
public:
	PMDRenderer(Microsoft::WRL::ComPtr<ID3D12Device>& _dev);
	~PMDRenderer();

	bool Init();

	Microsoft::WRL::ComPtr <ID3D12PipelineState>& GetPipeLineState();
	Microsoft::WRL::ComPtr <ID3D12RootSignature>& GetRootSignature();

private:
	//�f�o�C�X--�������Ɏ󂯎��B
	Microsoft::WRL::ComPtr<ID3D12Device> _Dev;


	//���[�g�V�O�l�`���n
	//���[�g�V�O�l�`��
	Microsoft::WRL::ComPtr <ID3D12RootSignature> _RootSigunature;
	//�V�O�l�`��
	Microsoft::WRL::ComPtr <ID3DBlob> _Signature;
	//�G���[�Ώ��p
	Microsoft::WRL::ComPtr <ID3DBlob> _Error;

	//�p�C�v���C���X�e�[�g
	Microsoft::WRL::ComPtr <ID3D12PipelineState> _PipelineState;

	//�V�F�[�_�[�֌W-------
	//���_�V�F�[�_
	Microsoft::WRL::ComPtr <ID3DBlob> _VertexShader;

	//�s�N�Z���V�F�[�_
	Microsoft::WRL::ComPtr <ID3DBlob> _PixelShader;

	bool CreateRootSignature();
	bool CreatePipeLineState();
	bool LoadShader();

};

