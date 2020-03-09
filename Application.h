#pragma once
#include <Windows.h>
#include<memory>
#include<vector>
class Dx12Wrapper;
class PMDRenderer;
class PMDActor;
class GraphicsMemory;
class SpriteFont;
class SpriteBatch;
struct Size
{
	int Height;
	int Width;
	Size() {};
	Size(int w, int h) :Width(w), Height(h) {};
};

enum Axis
{
	X,
	Y,
	Z,
	AXIS_SIZE
};

class Application
{

public:
	//シングルトンパターン
	static Application& Instance();
	//初期化
	bool Init();
	//実行
	void Run();
	//終了処理
	void Terminate();

	Size GetWindowSize();

	~Application();

private:
	Application();
	Application(const Application&);
	void operator=(const Application&);

	std::shared_ptr<Dx12Wrapper> _Dx12;
	std::shared_ptr<PMDRenderer> _PMDRenderer;
	std::vector<std::shared_ptr<PMDActor>> _PMDActor;

	HWND _Hwnd;
	WNDCLASSEX _WndClass;
};

