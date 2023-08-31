#include "graphics.hpp"
#include "point.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <thread>
#define _USE_MATH_DEFINES
#include <math.h>

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

namespace {

using namespace Microsoft::WRL;
using namespace std::chrono;

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

} // namespace
    
template<class T>
inline bool Graphics::plot(T const& data, float x, float y, float w, float h, float scale, ID2D1Brush* brush) {
    ComPtr<ID2D1PathGeometry> path;
    HRESULT hr;
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
    sink->BeginFigure(D2D1::Point2F(x + 0.1f * w, y - (0.1f + 0.8f * data.front() * scale) * h), D2D1_FIGURE_BEGIN_FILLED);
    for (size_t i = 0; i < data.size(); ++i) {
        sink->AddLine(D2D1::Point2F(
            x + w * (0.1f + i * 0.8f / data.size()),
            y - h * (0.1f + data[i] * 0.8f * scale)
        ));
    }
    sink->EndFigure(D2D1_FIGURE_END_OPEN);
    hr = sink->Close();
    if (FAILED(hr)) {
        std::cout << sink << "->Close() 0x" << std::hex << hr << _com_error(hr).ErrorMessage() << std::endl;
        return false;
    }
    target->DrawGeometry(path.Get(), brush, w / 500);
    return true;
}

bool Graphics::render() {
    if (!create()) {
        std::cout << "Create()" << std::endl;
        return false;
    }
    target->BeginDraw();
    HRESULT hr = S_OK;
    target->SetTransform(
        D2D1::Matrix3x2F::Scale(D2D1::SizeF(450 / w, 450 / w)) * D2D1::Matrix3x2F::Translation(D2D1::SizeF(500, 500)));
    target->Clear(D2D1::ColorF(D2D1::ColorF::White));

    {
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
        target->DrawGeometry(path.Get(), blue, w / 100.0f);
    }

    auto const max_vel = std::max_element(velocity.begin(), velocity.end());
    if (max_vel != velocity.end()) {
        auto const vel = max_vel - velocity.begin();
        auto const num = *max_vel;
        auto const b = 1.0f / vel / vel;
        auto const B = M_E * b * num;
        if (maxwell.empty()) {
            maxwell.resize(velocity.size());
        }
        for (size_t i = 0; i < velocity.size(); ++i) {
            auto const x2 = i * i;
            maxwell[i] = B * x2 * std::exp(-b * x2);
        }
        plot(velocity, w, 0, w, w, 1.0f / num, blue);
        plot(maxwell, w, 0, w, w, 1.0f / num, red);
    }
    plot(energy, w, w, w, w, 1.0f / *std::max_element(energy.begin(), energy.end()), blue);
    plot(deviation, 2 * w, w, w, w, 1.0f / *std::max_element(deviation.begin(), deviation.end()), blue);
    plot(rdf, 2 * w, 0, w, w, 1.0f / *std::max_element(rdf.begin(), rdf.end()), blue);

    hr = target->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        hr = S_OK;
        release();
    }
    last_frame = steady_clock::now();
    return SUCCEEDED(hr);
}

bool Graphics::resize(unsigned width, unsigned height) {
    return target && SUCCEEDED(target->Resize(D2D1::SizeU(width, height)));
}

bool Graphics::valid() {
    return running && factory && window;
}

Graphics::Graphics(float width_, float fps)
    : frame_time(fps ? static_cast<unsigned>(1000 / std::abs(fps)) : 0)
    , wait(fps > 0)
    , last_frame(steady_clock::now())
    , w(width_)
{
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
        0, 0,
        1900, 1000,
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

    hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Blue), &blue);
    hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &red);
    hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Green), &green);

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

    return true;
}

float Graphics::invalidate() {
    auto const now = steady_clock::now();
    auto const diff = now - last_frame;
    if (diff > frame_time) {
        InvalidateRect(reinterpret_cast<HWND>(window), nullptr, false);
        return 1.0f / duration_cast<duration<float>>(diff).count();
    }
    return 0;
}

namespace {

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
        ValidateRect(window, nullptr);
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

} // namespace

bool Graphics::step(histogram const& velocity_, histogram const& rdf_, point impulse_, float energy_, float deviation_) {
    rdf = rdf_,
    velocity = velocity_;
    impulse = impulse_;
    while (energy.size() >= velocity.size()) {
        energy.pop_front();
    }
    while (deviation.size() >= velocity.size()) {
        deviation.pop_front();
    }
    energy.push_back(energy_);
    deviation.push_back(deviation_);
    auto const fps = invalidate();
    // if (fps) std::cout << fps << std::endl;
    MSG msg;
    while (PeekMessage(&msg, reinterpret_cast<HWND>(window), 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (wait) {
        std::this_thread::sleep_until(last_frame + frame_time);
    }
    return fps > 0;
}

void Graphics::release() {
    SafeRelease(blue);
    SafeRelease(red);
    SafeRelease(green);
    SafeRelease(stops);
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

