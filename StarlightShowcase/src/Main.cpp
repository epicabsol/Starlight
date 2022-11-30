
#include "Window.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* pCmdLine, int nCmdShow)
{
	Window* window = new Window(hInstance, L"Starlight Showcase");

	MSG msg = { };
	/*while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}*/

	bool wantsQuit = false;
	int exitCode = 0;
	while (!wantsQuit)
	{
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
			window->Update(0.0f); // TODO: Get elapsed time!
			window->Render();
		}
	}

	delete window;
	
	return exitCode;
}