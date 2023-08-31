#include "graphics.hpp"
#include "point.hpp"

#include <atomic>
#include <iostream>

#include <csignal>
#include <objidlbase.h>
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <winerror.h>
#include <winnls.h>
#include <winnt.h>
#include <winuser.h>
#include <wrl/client.h>
#include <comdef.h>

using namespace Microsoft::WRL;

#ifndef HINST_CURRENT
static IMAGE_DOS_HEADER _instance;
#define HINST_CURRENT ((HINSTANCE)&_instance)
#endif // !HINST_CURRENT

static std::atomic_bool running { true };
void handler(int signal) {
    running.store(false);
}

template<class T>
void SafeRelease(T*& t) {
    if (t == nullptr) return;
    t->Release();
    t = nullptr;
}

static LRESULT CALLBACK procedure(
    HWND window,
    UINT message,
    WPARAM wparam,
    LPARAM lparam
);

bool Graphics::render() {
    if (!create()) {
        std::cout << "Create()" << std::endl;
        return false;
    }
    target->BeginDraw();
    HRESULT hr = S_OK;
    auto const size = target->GetSize();
    auto const w = size.height / 2;
    target->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(w, w)));
    target->Clear(D2D1::ColorF(D2D1::ColorF::White));

    ComPtr<ID2D1PathGeometry> path;
    hr = factory->CreatePathGeometry(&path);
    if (FAILED(hr)) {
        std::cout << factory << "->CreatePathGeometry() 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    ComPtr<ID2D1GeometrySink> sink;
    hr = path->Open(&sink);
    if (FAILED(hr)) {
        std::cout << path << "->Open(sink) 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    for (auto const& point: points) {
        sink->SetSegmentFlags(D2D1_PATH_SEGMENT_NONE);
        sink->BeginFigure(D2D1::Point2F(point[0], point[1]), D2D1_FIGURE_BEGIN_HOLLOW);
        sink->AddLine(D2D1::Point2F(point[0] + point[2], point[1] + point[3]));
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    hr = sink->Close();
    if (FAILED(hr)) {
        std::cout << sink << "->Close() 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }
    target->DrawGeometry(path.Get(), brush, 5.0f);

    ComPtr<ID2D1PathGeometry> path2;
    hr = factory->CreatePathGeometry(&path2);
    if (FAILED(hr)) {
        std::cout << factory << "->CreatePathGeometry() 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    ComPtr<ID2D1GeometrySink> sink2;
    hr = path2->Open(&sink2);
    if (FAILED(hr)) {
        std::cout << path2 << "->Open(sink) 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }
    sink2->BeginFigure(D2D1::Point2F(w * 1.1, -(0.1 + velocity[0] * 4.0f / points.size()) * w), D2D1_FIGURE_BEGIN_FILLED);
    for (size_t i = 0; i < velocity.size(); ++i) {
        sink2->AddLine(D2D1::Point2F(
            (1.1f + i * 0.8f / velocity.size()) * w,
            -(0.1f + velocity[i] * 4.0f / points.size()) * w
        ));
    }
    sink2->EndFigure(D2D1_FIGURE_END_OPEN);
    hr = sink2->Close();
    if (FAILED(hr)) {
        std::cout << sink2 << "->Close() 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }
    target->DrawGeometry(path2.Get(), brush);

    hr = target->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        hr = S_OK;
        release();
    }
    return SUCCEEDED(hr);
}

bool Graphics::resize(unsigned width, unsigned height) {
    return target && SUCCEEDED(target->Resize(D2D1::SizeU(width, height)));
}

bool Graphics::valid() {
    return running && factory && window;
}

Graphics::Graphics(unsigned width) {
    std::signal(SIGINT, handler);
    HRESULT hr = S_OK;
    hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        std::cout << "CoInitialize(nullptr) 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return;
    }

    if (!SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT))) {
        std::cout << "SetThreadLocale(en_US, en_US)" << std::endl;
        return;
    }

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
    if (FAILED(hr)) {
        std::cout << "D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED) 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return;
    }

    WNDCLASSEX wcex { };
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = HINST_CURRENT;
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.hCursor = nullptr;
    wcex.lpszClassName = "box";
    wcex.lpfnWndProc = procedure;
    if (!RegisterClassEx(&wcex)) {
        std::cout << "RegisterClassEx(" << wcex.lpszClassName << ')' << std::endl;
        return;
    }

    window = (void*)CreateWindow(
        "box", "box",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width * 3, width * 2,
        nullptr, nullptr,
        HINST_CURRENT, this
    );
    if (!window) {
        std::cout << "CreateWindow(box)" << std::endl;
        return;
    }
    ShowWindow(reinterpret_cast<HWND>(window), SW_SHOWNORMAL);
}

bool Graphics::create() {
    if (target) {
        return true;
    }

    RECT rc;
    if (!GetClientRect(reinterpret_cast<HWND>(window), &rc)) {
        std::cout << "GetClientRect(" << window << ')' << std::endl;
        return false;
    }

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
    HRESULT hr = factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(reinterpret_cast<HWND>(window), size),
        &target
    );
    if (FAILED(hr)) {
        std::cout << target << "->CreateHwndRenderTarget(" << window << ") 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    D2D1_GRADIENT_STOP gradient[2];
    gradient[0].position = 0;
    gradient[0].color = D2D1::ColorF(D2D1::ColorF::Red);
    gradient[0].position = 1;
    gradient[0].color = D2D1::ColorF(D2D1::ColorF::Blue);
    hr = target->CreateGradientStopCollection(
        gradient, std::size(gradient),
        D2D1_GAMMA_2_2,
        D2D1_EXTEND_MODE_CLAMP,
        &stops
    );
    if (FAILED(hr)) {
        std::cout << target << "->CreateGradientStops() 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    hr = target->CreateLinearGradientBrush(
        D2D1::LinearGradientBrushProperties(
            D2D1::Point2F(0, 0),
            D2D1::Point2F(0, 1000000)
        ),
        stops,
        &brush
    );
    if (FAILED(hr)) {
        std::cout << target << "->CreateSolidColorBrush(Black) 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }

    return true;
}

LRESULT CALLBACK procedure(
    HWND window,
    UINT message,
    WPARAM wparam,
    LPARAM lparam
) {
    if (message == WM_CREATE) {
        auto const pcs = (LPCREATESTRUCT)lparam;
        auto const graphics = (Graphics*)pcs->lpCreateParams;
        SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(graphics));
        return 1;
    }

    auto graphics = reinterpret_cast<Graphics*>(static_cast<LONG_PTR>(
        GetWindowLongPtr(window, GWLP_USERDATA)
    ));
    if (graphics == nullptr) {
        return DefWindowProc(window, message, wparam, lparam);
    }

    switch (message) {
    case WM_SIZE:
        graphics->resize(LOWORD(lparam), HIWORD(lparam));
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        running = false;
        return 1;
    case WM_DISPLAYCHANGE:
        InvalidateRect(window, nullptr, false);
        break;
    case WM_PAINT:
        graphics->render();
        // ValidateRect(window, nullptr);
        break;
    case WM_KEYUP:
        switch (wparam) {
        case 'Q': case VK_ESCAPE:
            PostQuitMessage(0);
            running = false;
            break;
        default:
            return DefWindowProc(window, message, wparam, lparam);
        }
        break;
    default:
        return DefWindowProc(window, message, wparam, lparam);
    }
    return 0;
}

bool Graphics::step() {
    MSG msg;
    if (!GetMessage(&msg, nullptr, 0, 0)) {
        PostQuitMessage(0);
        return false;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return true;
}

void Graphics::release() {
    SafeRelease(stops);
    SafeRelease(brush);
    SafeRelease(target);
}

Graphics::~Graphics()
{
    if (valid()) {
        CloseWindow(reinterpret_cast<HWND>(window));
        release();
        SafeRelease(factory);
        CoUninitialize();
    }
}

