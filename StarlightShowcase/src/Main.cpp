
#include "Window.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* pCmdLine, int nCmdShow)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	Window* window = new Window(hInstance, L"Starlight Showcase");

	MSG msg = { };

	bool wantsQuit = false;
	int exitCode = 0;
	while (!wantsQuit)
	{
		window->WaitOnSwapChain();

		// First, process all pending messages
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			LRESULT result = DispatchMessageW(&msg);
			if (msg.message == WM_QUIT)
			{
				exitCode = result;
				wantsQuit = true;
			}
		}

		if (!wantsQuit)
		{
			window->Update(1.0f / 60.0f); // TODO: Get elapsed time!

			// Render if window is visible, otherwise yield CPU time
			if (window->IsVisible())
			{
				window->Render();

				window->Present();
			}
			else
			{
				Sleep(0);
			}
		}
	}

	delete window;
	
	return exitCode;
}