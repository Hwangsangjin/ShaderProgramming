﻿#include "ShaderFramework.h"
#include <stdio.h>

#define PI				3.14159265f
#define FOV				(PI / 4.0f)							// 시야각
#define ASPECT_RATIO	(WIN_WIDTH / (float)WIN_HEIGHT)		// 화면의 종횡비
#define NEAR_PLANE		1									// 근접 평면
#define FAR_PLANE		10000								// 원거리 평면

//----------------------------------------------------------------------
// 전역 변수
//----------------------------------------------------------------------

// D3D 관련
LPDIRECT3D9             gpD3D			= nullptr;			// D3D
LPDIRECT3DDEVICE9       gpD3DDevice		= nullptr;			// D3D 장치

// 폰트
ID3DXFont*              gpFont			= nullptr;

// 모델
LPD3DXMESH				gpSphere		= nullptr;

// 셰이더
LPD3DXEFFECT			gpSpecularMappingShader = nullptr;

// 텍스처
LPDIRECT3DTEXTURE9		gpStoneDM		= nullptr;
LPDIRECT3DTEXTURE9		gpStoneSM		= nullptr;

// 프로그램 이름
const char*				gAppName		= "Shader Framework";

// 회전값
float					gRotationY		= 0.0f;

// 빛의 위치
D3DXVECTOR4				gWorldLightPosition(500.0f, 500.0f, -500.0f, 1.0f);

// 빛의 색상
D3DXVECTOR4				gLightColor(0.7f, 0.7f, 1.0f, 1.0f);

// 카메라 위치
D3DXVECTOR4				gWorldCameraPosition(0.0f, 0.0f, -200.0f, 1.0f);

//-----------------------------------------------------------------------
// 프로그램 진입점, 메시지 루프
//-----------------------------------------------------------------------

// 진입점
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
    // 윈도우 클래스를 등록한다.
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, gAppName, nullptr };
    RegisterClassEx(&wc);

    // 프로그램 창을 생성한다.
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    HWND hWnd = CreateWindow(gAppName, gAppName, style, CW_USEDEFAULT, 0, WIN_WIDTH, WIN_HEIGHT, GetDesktopWindow(), nullptr, wc.hInstance, nullptr);

	// Client Rect 크기가 WIN_WIDTH, WIN_HEIGHT와 같도록 크기를 조정한다.
	POINT ptDiff;
	RECT rcClient, rcWindow;
	
	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd,rcWindow.left, rcWindow.top, WIN_WIDTH + ptDiff.x, WIN_HEIGHT + ptDiff.y, TRUE);

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

	// D3D를 비롯한 모든 것을 초기화한다.
	if(!InitEverything(hWnd))
		PostQuitMessage(1);

	// 메시지 루프
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while(msg.message!=WM_QUIT)
    {
		if( PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else // 메시지가 없으면 게임을 업데이트하고 장면을 그린다.
		{
			PlayDemo();
		}
    }

    UnregisterClass(gAppName, wc.hInstance);
    return 0;
}

// 메시지 처리기
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
	case WM_KEYDOWN:
		ProcessInput(hWnd, wParam);
		break;

    case WM_DESTROY:
		Cleanup();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 키보드 입력처리
void ProcessInput(HWND hWnd, WPARAM keyPress)
{
	switch(keyPress)
	{
	// ESC 키가 눌리면 프로그램을 종료한다.
	case VK_ESCAPE:
		PostMessage(hWnd, WM_DESTROY, 0L, 0L);
		break;
	}
}

//------------------------------------------------------------
// 게임 루프
//------------------------------------------------------------
void PlayDemo()
{
	Update();
    RenderFrame();
}

// 게임 로직 업데이트
void Update()
{
}

//------------------------------------------------------------
// 렌더링
//------------------------------------------------------------
void RenderFrame()
{
	D3DCOLOR bgColor = 0x003366;	// 배경 색상

    gpD3DDevice->Clear(0, nullptr, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), bgColor, 1.0f, 0);

    gpD3DDevice->BeginScene();
    {
		RenderScene();				// 3D 물체 등을 그린다.
		RenderInfo();				// 디버그 정보 등을 출력한다.
    }
	gpD3DDevice->EndScene();

    gpD3DDevice->Present(nullptr, nullptr, nullptr, nullptr);
}

// 3D 물체 등을 그린다.
void RenderScene()
{
	// 뷰 행렬을 만든다.
	D3DXMATRIXA16 matView;
	D3DXVECTOR3 vEye(gWorldCameraPosition.x, gWorldCameraPosition.y, gWorldCameraPosition.z);
	D3DXVECTOR3 vLookAt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&matView, &vEye, &vLookAt, &vUp);

	// 투영 행렬을 만든다.
	D3DXMATRIXA16 matProjection;
	D3DXMatrixPerspectiveFovLH(&matProjection, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);

	// 프레임마다 0.1도씩 회전을 시킨다.
	gRotationY += 0.1f * PI / 180.0f;
	if (gRotationY > 2 * PI)
	{
		gRotationY -= 2 * PI;
	}

	// 월드 행렬을 만든다.
	D3DXMATRIXA16 matWorld;
	D3DXMatrixRotationY(&matWorld, gRotationY);

	// 셰이더 전역 변수들을 설정
	gpSpecularMappingShader->SetMatrix("gWorldMatrix", &matWorld);
	gpSpecularMappingShader->SetMatrix("gViewMatrix", &matView);
	gpSpecularMappingShader->SetMatrix("gProjectionMatrix", &matProjection);
	gpSpecularMappingShader->SetVector("gWorldLightPosition", &gWorldLightPosition);
	gpSpecularMappingShader->SetVector("gWorldCameraPosition", &gWorldCameraPosition);
	gpSpecularMappingShader->SetVector("gLightColor", &gLightColor);
	gpSpecularMappingShader->SetTexture("DiffuseMap_Tex", gpStoneDM);
	gpSpecularMappingShader->SetTexture("SpecularMap_Tex", gpStoneSM);

	// 셰이더를 시작한다.
	UINT numPasses = 0;
	gpSpecularMappingShader->Begin(&numPasses, NULL);
	{
		for (UINT i = 0; i < numPasses; ++i)
		{
			gpSpecularMappingShader->BeginPass(i);
			{
				// 구체를 그린다.
				gpSphere->DrawSubset(0);
			}
			gpSpecularMappingShader->EndPass();
		}
	}
	gpSpecularMappingShader->End();
}

// 디버그 정보 등을 출력.
void RenderInfo()
{
	// 텍스트 색상
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);    

	// 텍스트를 출력할 위치
	RECT rct;
	rct.left=5;
	rct.right=WIN_WIDTH / 3;
	rct.top=5;
	rct.bottom = WIN_HEIGHT / 3;
	 
	// 키 입력 정보를 출력
	//gpFont->DrawText(nullptr, " ", -1, &rct, 0, fontColor);
}

//------------------------------------------------------------
// 초기화 코드
//------------------------------------------------------------

// 모든 초기화
bool InitEverything(HWND hWnd)
{
	// D3D를 초기화
	if(!InitD3D(hWnd))
	{
		return false;
	}
	
	// 모델, 셰이더, 텍스처 등을 로딩
	if(!LoadAssets())
	{
		return false;
	}

	// 폰트를 로딩
    if(FAILED(D3DXCreateFont(gpD3DDevice, 20, 10, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE), "Arial", &gpFont)))
	{
		return false;
	}

	return true;
}

// D3D 객체 및 장치 초기화
bool InitD3D(HWND hWnd)
{
    // D3D 객체
    gpD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!gpD3D)
	{
		return false;
	}

    // D3D 장치를 생성하는데 필요한 구조체를 채워 넣는다.
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferWidth				= WIN_WIDTH;
	d3dpp.BackBufferHeight				= WIN_HEIGHT;
	d3dpp.BackBufferFormat				= D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount				= 1;
    d3dpp.MultiSampleType				= D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality			= 0;
    d3dpp.SwapEffect					= D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow					= hWnd;
    d3dpp.Windowed						= TRUE;
    d3dpp.EnableAutoDepthStencil		= TRUE;
    d3dpp.AutoDepthStencilFormat		= D3DFMT_D24X8;
    d3dpp.Flags							= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    d3dpp.FullScreen_RefreshRateInHz	= 0;
    d3dpp.PresentationInterval			= D3DPRESENT_INTERVAL_ONE;

    // D3D 장치를 생성한다.
    if(FAILED(gpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &gpD3DDevice)))
    {
        return false;
    }

    return true;
}

// 에셋 로딩
bool LoadAssets()
{
	// 텍스처 로딩
	gpStoneDM = LoadTexture("Fieldstone_DM.tga");
	if (!gpStoneDM)
	{
		return false;
	}

	gpStoneSM = LoadTexture("Fieldstone_SM.tga");
	if (!gpStoneSM)
	{
		return false;
	}

	// 셰이더 로딩
	gpSpecularMappingShader = LoadShader("SpecularMapping.fx");
	if (!gpSpecularMappingShader)
	{
		return false;
	}

	// 모델 로딩
	gpSphere = LoadModel("Sphere.x");
	if (!gpSphere)
	{
		return false;
	}

	return true;
}

// 셰이더 로딩
LPD3DXEFFECT LoadShader(const char* filename)
{
	LPD3DXEFFECT ret = nullptr;
	LPD3DXBUFFER pError = nullptr;
	DWORD dwShaderFlags = 0;

#if _DEBUG
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

	D3DXCreateEffectFromFile(gpD3DDevice, filename, nullptr, nullptr, dwShaderFlags, nullptr, &ret, &pError);
	
	// 셰이더 로딩에 실패한 경우 Output창에 셰이더 컴파일 에러를 출력한다.
	if (!ret && pError)
	{
		int size  = pError->GetBufferSize();
		void* ack = pError->GetBufferPointer();

		if (ack)
		{
			char* str = new char[size];
			sprintf(str, (const char*)ack, size);
			OutputDebugString(str);
			delete[] str;
		}
	}

	return ret;
}

// 모델 로딩
LPD3DXMESH LoadModel(const char* filename)
{
	LPD3DXMESH ret = nullptr;
	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, gpD3DDevice, nullptr, nullptr, nullptr, nullptr, &ret)))
	{
		OutputDebugString("모델 로딩 실패: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	};

	return ret;
}

// 텍스처 로딩
LPDIRECT3DTEXTURE9 LoadTexture(const char* filename)
{
	LPDIRECT3DTEXTURE9 ret = nullptr;
	if (FAILED(D3DXCreateTextureFromFile(gpD3DDevice, filename, &ret)))
	{
		OutputDebugString("텍스처 로딩 실패: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	}

	return ret;
}

//------------------------------------------------------------
// 메모리 해제
//------------------------------------------------------------

// 클린업
void Cleanup()
{
	// 폰트를 release 한다.
	if(gpFont)
	{
		gpFont->Release();
		gpFont = nullptr;
	}

	// 모델을 release 한다.
	if (gpSphere)
	{
		gpSphere->Release();
		gpSphere = nullptr;
	}

	// 셰이더를 release 한다.
	if (gpSpecularMappingShader)
	{
		gpSpecularMappingShader->Release();
		gpSpecularMappingShader = nullptr;
	}

	// 텍스처를 release 한다.
	if (gpStoneDM)
	{
		gpStoneDM->Release();
		gpStoneDM = nullptr;
	}

	if (gpStoneSM)
	{
		gpStoneSM->Release();
		gpStoneSM = nullptr;
	}

	// D3D를 release 한다.
    if(gpD3DDevice)
	{
        gpD3DDevice->Release();
		gpD3DDevice = nullptr;
	}

    if(gpD3D)
	{
        gpD3D->Release();
		gpD3D = nullptr;
	}
}

