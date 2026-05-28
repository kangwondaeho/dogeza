#pragma once
#include "WindowContext.hpp"
#include "GraphicsContext.hpp"
#include "Timer.hpp"
#include "ObjectBase.hpp"

class GameLoop {
public:
    WindowContext win;
    GraphicsContext gfx;
    DeltaTime timer;
    std::vector<GameObject*> world;
    bool isRunning = true;

    GameLoop() {
        printf("[Engine] GameLoop Created.\n");
    }

    ~GameLoop() {
        for (auto obj : world) delete obj;
        world.clear();
        printf("[Engine] GameLoop Destroyed.\n");
    }

    void Initialize(HINSTANCE hInst, LRESULT(CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM)) {
        win.Initialize(hInst, 1280, 720, wndProc);
        gfx.InitDX(win.hWnd, 1280, 720);
    }

    void Input() {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) isRunning = false;

        if (GetAsyncKeyState('C') & 0x0001) {
            win.Width = 1280; win.Height = 720;
            RECT rc = { 0, 0, win.Width, win.Height };
            AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
            SetWindowPos(win.hWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
            gfx.Resize(win.Width, win.Height);
        }

        for (auto obj : world) obj->Input();
    }

    void Update() {
        float dt = timer.GetDelta();
        if (dt > 0.033f) dt = 0.033f;
        for (auto obj : world) obj->Update(dt, &gfx);
    }

    void Render() {
        float col[] = { 0.06f, 0.06f, 0.07f, 1.0f };

        D3D11_VIEWPORT vp = { 0, 0, (float)win.Width, (float)win.Height, 0, 1 };
        gfx.ImmediateContext->RSSetViewports(1, &vp);
        gfx.BeginSceneRender(col, win.Width, win.Height);

        float blendFactor[4] = { 0, 0, 0, 0 };
        gfx.ImmediateContext->OMSetBlendState(gfx.AlphaBlendState, blendFactor, 0xffffffff);
        gfx.ImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (auto obj : world) obj->Render(&gfx);

        gfx.EndSceneRenderWithBloom(win.Width, win.Height);
        gfx.SwapChain->Present(gfx.VSync, 0);
    }

    void Run() {
        MSG msg = {};
        while (msg.message != WM_QUIT && isRunning) {
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg); DispatchMessage(&msg);
            }
            else {
                Input();
                Update();
                Render();
            }
        }
    }
};
