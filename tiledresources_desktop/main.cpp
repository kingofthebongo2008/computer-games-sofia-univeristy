#include "pch.h"
#include "DirectXHelper.h"
#include "DeviceResources.h"
#include "TiledResourcesMain.h"
#include "SampleSettings.h"

using namespace std;
using namespace TiledResources;

HWND g_hwnd;
shared_ptr<DeviceResources> g_deviceResources;
shared_ptr<TiledResourcesMain> g_main;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    static bool first = true;
    static int xpos = 0;
    static int ypos = 0;
    int xnew = 0;
    int ynew = 0;

    switch (uMsg)
    {
    case WM_KEYDOWN:
        g_main->OnKeyChanged(wParam, true);
        break;
    case WM_KEYUP:
        g_main->OnKeyChanged(wParam, false);
        break;
    case WM_MOUSEMOVE:
        xnew = (int)(lParam & 0xFFFF);
        ynew = (int)(lParam >> 16);
        if (!first) g_main->OnPointerMoved((float) (xnew - xpos), (float) (ynew - ypos));
        first = false;
        xpos = xnew;
        ypos = ynew;
        break;
    case WM_RBUTTONDOWN:
        g_main->OnRightClick((float)xpos, (float)ypos);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        result = 1;
        break;
    case WM_QUIT:
        result = 0;
        break;
    default:
        result = DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
    }
    return result;
}

void MakeWindow()
{
    HR hr = S_OK;

    WNDCLASSEX wcex = { 0 };
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName = L"TiledResources";

    hr = RegisterClassEx(&wcex) ? S_OK : E_FAIL;
#if 0 // set to 1 for full-screen
    g_hwnd = CreateWindow(
        wcex.lpszClassName,
        L"Tiled Resources",
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        NULL,
        NULL
        );
    if (g_hwnd == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    ShowWindow(g_hwnd, SW_MAXIMIZE);
#else
    g_hwnd = CreateWindow(
        wcex.lpszClassName,
        L"Tiled Resources",
        WS_OVERLAPPED,
        0,
        0,
        1600,
        900,
        NULL,
        NULL,
        NULL,
        NULL
        );
    if (g_hwnd == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    ShowWindow(g_hwnd, SW_NORMAL);
#endif

}

float nd(SHORT value)
{
    // normalize and dead-zone
    float dead = 0.25f;
    float result = value / 32767.0f;
    if (abs(result) < dead) return 0.0f;
    if (result > 1.0f) return 1.0f;
    if (result < -1.0f) return -1.0f;
    if(result < 0.0f) return (result + dead) / (1.0f - dead);
    return (result - dead) / (1.0f - dead);
}

void HandleXInput()
{
    static bool controllerConnected = false;
    static UINT64 lastEnumTime = 0;

    if (!controllerConnected)
    {
        UINT64 currentTime = GetTickCount64();
        if (currentTime - lastEnumTime < 2000) return;
        lastEnumTime = currentTime;

        XINPUT_CAPABILITIES caps;
        UINT32 capsResult = XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &caps);
        if (capsResult != ERROR_SUCCESS) return;
        controllerConnected = true;
        OutputDebugStringA("XINPUT: Controller Connected\n");
    }
    XINPUT_STATE state;
    UINT32 stateResult = XInputGetState(0, &state);
    if (stateResult != ERROR_SUCCESS)
    {
        controllerConnected = false;
        OutputDebugStringA("XINPUT: Controller Disconnected\n");
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        lastEnumTime = GetTickCount64();
        return;
    }
    float vx = nd(state.Gamepad.sThumbLX);
    float vy = ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? 1.0f : 0.0f) + ((state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? -1.0f : 0.0f);
    float vz = -nd(state.Gamepad.sThumbLY);
    float vrx = 3.0f * nd(state.Gamepad.sThumbRY);
    float vry = -3.0f * nd(state.Gamepad.sThumbRX);
    float vrz = 2.0f * ((float) state.Gamepad.bLeftTrigger - (float) state.Gamepad.bRightTrigger) / 255.0f;
    if (vx != 0 || vy != 0 || vz != 0 || vrx != 0 || vry != 0 || vrz != 0)
    {
        SetCursor(NULL);
        g_main->SetControlState(vx, vy, vz, vrx, vry, vrz);
    }

    static bool xdown = false;
    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
    {
        if (!xdown)
        {
            g_main->OnKeyChanged(SampleSettings::Controls::ToggleLodLimit, true);
        }
        xdown = true;
    }
    else
    {
        xdown = false;
    }

    static bool ydown = false;
    if (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
    {
        if (!ydown)
        {
            g_main->OnKeyChanged(SampleSettings::Controls::ToggleBorder, true);
        }
        ydown = true;
    }
    else
    {
        ydown = false;
    }

    float sunx = ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? 1.0f : 0.0f) + ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? -1.0f : 0.0f);
    float suny = ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) ? 1.0f : 0.0f) + ((state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? -1.0f : 0.0f);
    sunx *= 4.0f;
    suny *= 4.0f;
    g_main->RotateSun(sunx, suny);

    float scaleDelta = ((state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1.0f : 0.0f) + ((state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? -1.0f : 0.0f);
    scaleDelta *= 7.0f;
    g_main->ChangeScale(scaleDelta);
};

void RunLoop()
{
    HR hr = S_OK;

    MSG msg { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            HandleXInput();
            g_main->Update();
            g_main->Render();
            g_deviceResources->Present();
        }
    }
}

// validate that expected files are in the working directory
// these need to be downloaded separately from http://go.microsoft.com/fwlink/?LinkID=313296
void ValidateFiles()
{
	vector<wstring> filenames;
	filenames.push_back(L"diffuse.bin"); // diffuse layer data
	filenames.push_back(L"normal.bin"); // normal layer data
	filenames.push_back(L"geometry.vb.bin"); // vertex buffer data
	filenames.push_back(L"geometry.ib.bin"); // index buffer data

	for (const auto & filename : filenames)
	{
		HANDLE file = CreateFile2(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);

		if (file == INVALID_HANDLE_VALUE)
		{
			HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
			wstring error = L"Couldn't find file \"" + filename + L"\" in the working directory:\n";
			error += _wgetcwd(NULL, 0);
			error += L"\nThis file can be downloaded separately from http://go.microsoft.com/fwlink/?LinkID=313296";
			MessageBox(NULL, error.c_str(), L"Error Opening Data File", MB_OK | MB_ICONERROR);
			DX::ThrowIfFailed(FAILED(hr) ? hr : E_FAIL); // in case getlasterror doesn't have fail data
		}

		CloseHandle(file);
	}
}

int wmain(int argc, wchar_t* argv[])
{
	ValidateFiles();
    MakeWindow();
    g_deviceResources = make_shared<DeviceResources>();
    g_deviceResources->SetWindow(g_hwnd);
    g_main = std::shared_ptr<TiledResourcesMain>(new TiledResourcesMain(g_deviceResources));
    RunLoop();
    wcout << L"Program ran to completion." << endl;
    return 0;
}
