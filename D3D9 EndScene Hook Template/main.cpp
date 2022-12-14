#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include "mem.h"
#include "gh_d3d9.h"
#include "imgui.h"
#include "backends/imgui_impl_dx9.h"
#include "backends/imgui_impl_win32.h"


bool bInit = false;
bool g_ShowMenu = true;
tPresent oPresent = nullptr;
tReset oReset = nullptr;
LPDIRECT3DDEVICE9 pD3DDevice = nullptr;
static WNDPROC origWndProc = nullptr;
static WNDPROC oWndProc = nullptr;
void* d3d9Device[119];

char oPresBytes[5];
char oResetBytes[5];

bool done = false;

HHOOK mouseHook = NULL;

void cleanupImgui() {
	/* Delete imgui to avoid errors */
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void InitImGui(IDirect3DDevice9* pDevice) {
	D3DDEVICE_CREATION_PARAMETERS CP;
	pDevice->GetCreationParameters(&CP);
	window = CP.hFocusWindow;
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->AddFontDefault();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(pDevice);
	bInit = true;
	return;
}
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return 1;
	}


	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// Check if ImGui wants to handle the keyboard..
	if (msg >= WM_KEYFIRST && msg <= WM_KEYLAST)
	{
		if (io.WantTextInput || io.WantCaptureKeyboard || ImGui::IsAnyItemActive())
			return 1;
	}

	// Check if ImGui wants to handle the mouse..
	if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
	{
		if (io.WantCaptureMouse)
			return 1;
	}

	return ::CallWindowProcA(oWndProc, hWnd, msg, wParam, lParam);
}

HRESULT APIENTRY hkPresent(LPDIRECT3DDEVICE9 pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{

	//draw stuff here like so:
	if (!bInit) InitImGui(pDevice);
	else {			

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();

		ImGui::NewFrame();

		if (g_ShowMenu)
		{
			bool bShow = true;
			ImGui::ShowDemoWindow(&bShow);
		}
		else {
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		ImGui::EndFrame();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}

	return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT APIENTRY hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{

	if (bInit) {
		/* Delete imgui to avoid errors */
		cleanupImgui();
	}


	return oReset(pDevice, pPresentationParameters);
}



DWORD WINAPI Init(HMODULE hModule)
{
//#ifdef _DEBUG
//	//Create Console
//	AllocConsole();
//	FILE* f;
//	freopen_s(&f, "CONOUT$", "w", stdout);
//	std::cout << "DLL got injected!" << std::endl;
//#endif 

	if (GetD3D9Device(d3d9Device, sizeof(d3d9Device)))
	{
		//write original bytes to buffer for cleanup later
		memcpy(oPresBytes, (char*)d3d9Device[17], 5);
		memcpy(oResetBytes, (char*)d3d9Device[16], 5);
		
		//std::cout << "reset at: 0x" << std::hex << d3d9Device[16] << std::endl;
		oPresent = (tPresent)TrampHook((char*)d3d9Device[17], (char*)hkPresent, 5);
		oReset = (tReset)TrampHook((char*)d3d9Device[16], (char*)hkReset, 5);
	}
	window = GetProcessWindow();
	origWndProc = (WNDPROC)GetWindowLongPtr(window, GWL_WNDPROC);
	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);

	while (true) {
		if (GetAsyncKeyState(VK_RCONTROL) & 1) {
			break;
		}
	}

//#ifdef _DEBUG
//	if (f != 0) {
//		fclose(f);
//	}
//	FreeConsole();
//#endif 
//

	(WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)origWndProc);
	WriteMem((char*)d3d9Device[17], oPresBytes, 5);
	WriteMem((char*)d3d9Device[16], oResetBytes, 5);
	cleanupImgui();
	FreeLibraryAndExitThread(hModule, 0);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Init, hModule, 0, nullptr));
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}