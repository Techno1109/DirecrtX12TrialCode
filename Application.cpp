#include "Application.h"
#include "Dx12Wrapper.h"
#include "PMDRenderer.h"
#include "PMDActor.h"
#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_win32.h"

#include<SpriteFont.h>
#include<ResourceUploadBatch.h>

#include<Effekseer.h>
#include<EffekseerRendererDX12.h>


constexpr int Window_W = 1280;
constexpr int Window_H = 720;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


//�R�[���o�b�N�֐��QOS����Ăяo�����̂Œ�`���K�v�B
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);

		return 0;
	}
	ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam);
	return DefWindowProc(hwnd,msg,wparam,lparam);
}


Application::Application()
{
}


Application & Application::Instance()
{
	static Application instance;
	return instance;
}

bool Application::Init()
{
	_WndClass.hInstance = GetModuleHandle(nullptr);
	_WndClass.cbSize=sizeof(WNDCLASSEX);
	_WndClass.lpfnWndProc=(WNDPROC)WindowProcedure;
	_WndClass.lpszClassName = "DirectX12_Techno1109";
	RegisterClassEx(&_WndClass);

	RECT Wrect = {0,0,Window_W,Window_H};
	AdjustWindowRect(&Wrect, WS_OVERLAPPEDWINDOW, false);
	_Hwnd = CreateWindow
		(
		_WndClass.lpszClassName,
		"DirectX12_Techno1109",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		Wrect.right-Wrect.left,
		Wrect.bottom - Wrect.top,
		nullptr,
		nullptr,
		_WndClass.hInstance,
		nullptr
		);

	if (_Hwnd == 0)
	{
		return false;
	}

	_Dx12 = std::make_shared<Dx12Wrapper>(_Hwnd);

	if (!_Dx12->Init())
	{
		return false;
	}
	_PMDRenderer = std::make_shared<PMDRenderer>(_Dx12->GetDevice());

	if (!_PMDRenderer->Init())
	{
		return false;
	}

	_PMDActor.emplace_back(std::make_shared<PMDActor>(_Dx12->GetDevice(),_Dx12->GetCmdList()));
	_PMDActor.emplace_back(std::make_shared<PMDActor>(_Dx12->GetDevice(), _Dx12->GetCmdList()));

	//�����͌��J�p�Ɏg�pPMD�̖��O���yTestModel�z�Ƃ��Ă��܂��B
	//�����̓��f���̃t�@�C���p�X,VMD�̃t�@�C���p�X
	if (!_PMDActor[0]->Init("models/TestModel.pmd", "Pose/swing2.vmd"))
	{
		return false;
	}
	if (!_PMDActor[1]->Init("models/TestModel.pmd", "Pose/swing2.vmd"))
	{
		return false;
	}

	return true;
}

void Application::Run()
{
	ShowWindow(_Hwnd,SW_SHOW);
	MSG msg = {};

	while (true)
	{
		//OS���烁�b�Z�[�W���󂯎��
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			//�󂯎�������b�Z�[�W���̂܂܂ł͏��s���ATranslate����
			TranslateMessage(&msg);
			//�ϊ�����Ȃ��������b�Z�[�W��OS�ւ��̂܂ܕԂ��B
			DispatchMessage(&msg);
		}


		//QUIT���b�Z�[�W�����Ă�����u���C�N�B
		if (msg.message==WM_QUIT)
		{
			break;
		}

		BYTE keycode[256];
		GetKeyboardState(keycode);

		auto Move = [&keycode](float& Target,BYTE Key,float Speed) 
					{
						if (keycode[Key] & 0x80)
						{
							Target += Speed;
						}
					};

		//�J�����ړ�
		float CamPos[Axis::AXIS_SIZE] = { 0,0,0 };
		Move(CamPos[Axis::X], VK_RIGHT,0.1f);
		Move(CamPos[Axis::X], VK_LEFT , -0.1f);
		Move(CamPos[Axis::Y], VK_UP, 0.1f);
		Move(CamPos[Axis::Y], VK_DOWN, -0.1f);
		Move(CamPos[Axis::Z], 'Z', 0.1f);
		Move(CamPos[Axis::Z], 'X', -0.1f);

		_Dx12->MoveOffset(CamPos[Axis::X], CamPos[Axis::Y], CamPos[Axis::Z]);


		//���f���ړ�
		float ActorPos[Axis::AXIS_SIZE] = { 0,0,0 };

		Move(ActorPos[Axis::X], 'D', 0.1f);
		Move(ActorPos[Axis::X], 'A', -0.1f);
		Move(ActorPos[Axis::Y], 'Q', 0.1f);
		Move(ActorPos[Axis::Y], 'E', -0.1f);
		Move(ActorPos[Axis::Z], 'W', 0.1f);
		Move(ActorPos[Axis::Z], 'S', -0.1f);

		//���f����]
		float Angle = 0;
		Move(Angle, VK_NUMPAD6, 0.1f);
		Move(Angle, VK_NUMPAD4, -0.1f);

		_PMDActor[0]->MoveWorldAngle(Angle);
		_PMDActor[0]->MovePosition(ActorPos[Axis::X], ActorPos[Axis::Y], ActorPos[Axis::Z]);

		if (keycode[VK_SPACE] & 0x80)
		{
			_Dx12->StartEffect();
		}
		for(auto& Actor:_PMDActor)
		{
			Actor->Update();
		}

		_Dx12->ShadowDrawSetting();
		_Dx12->ShadowDraw();
		_Dx12->SetPipeLineState(_PMDRenderer->GetPipeLineState());
		_Dx12->SetRootSignature(_PMDRenderer->GetRootSignature());
		for (auto& Actor : _PMDActor)
		{
			Actor->Draw(true);
		}

		//�`��O�ݒ�
		_Dx12->DrawSetting();

		for (auto& Actor : _PMDActor)
		{
			Actor->Draw();
		}

		//�`��I��(�R�}���h���s)
		_Dx12->EndDraw();
		//�`��O�ݒ�
		_Dx12->ScreenCrear();
		_Dx12->Draw();
		//��ʂ̍X�V
		_Dx12->ScreenFlip();
	}
}

void Application::Terminate()
{
	CoUninitialize();
	//Window�̎g�p����ԋp
	UnregisterClass(_WndClass.lpszClassName,_WndClass.hInstance);
}

Size Application::GetWindowSize()
{
	return Size(Window_W,Window_H);
}

Application::~Application()
{
}
