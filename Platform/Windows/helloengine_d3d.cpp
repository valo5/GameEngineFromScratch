// 包括基本的windows头文件
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdint.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>

using namespace DirectX;
using namespace DirectX::PackedVector;

const uint32_t SCREEN_WIDTH  =  960;
const uint32_t SCREEN_HEIGHT =  480;

//全局声明
IDXGISwapChain          *g_pSwapchain = nullptr;              // 指向交换链接口的指针
ID3D11Device            *g_pDev       = nullptr;              // 指向我们的Direct3D设备接口的指针
ID3D11DeviceContext     *g_pDevcon    = nullptr;              //指向我们的Direct3D设备上下文的指针

ID3D11RenderTargetView  *g_pRTView    = nullptr;

ID3D11InputLayout       *g_pLayout    = nullptr;              //指向输入布局的指针
ID3D11VertexShader      *g_pVS        = nullptr;              // 指向顶点着色器的指针
ID3D11PixelShader       *g_pPS        = nullptr;              // 指向像素着色器的指针

ID3D11Buffer            *g_pVBuffer   = nullptr;              // 顶点缓冲

// 顶点缓冲结构
struct VERTEX {
        XMFLOAT3    Position;
        XMFLOAT4    Color;
};


template<class T>
inline void SafeRelease(T **ppInterfaceToRelease)
{
    if (*ppInterfaceToRelease != nullptr)
    {
        (*ppInterfaceToRelease)->Release();

        (*ppInterfaceToRelease) = nullptr;
    }
}

//创建 RenderTarget 
void CreateRenderTarget() {
    HRESULT hr;
    ID3D11Texture2D *pBackBuffer;

    // 获取返回缓冲区的指针
    g_pSwapchain->GetBuffer( 0, __uuidof( ID3D11Texture2D ),
                                 ( LPVOID* )&pBackBuffer );

    // 创建一个render-target视图
    g_pDev->CreateRenderTargetView( pBackBuffer, NULL,
                                          &g_pRTView );
    pBackBuffer->Release();

    // 绑定视图
    g_pDevcon->OMSetRenderTargets( 1, &g_pRTView, NULL );
}

//设置视口
void SetViewPort() {
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    g_pDevcon->RSSetViewports(1, &viewport);
}

//这是加载和准备着色器的函数
void InitPipeline() {
    // 加载并编译这两个着色器
    ID3DBlob *VS, *PS;

    D3DReadFileToBlob(L"copy.vso", &VS);
    D3DReadFileToBlob(L"copy.pso", &PS);

    // 将两个着色器封装到着色器对象中
    g_pDev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &g_pVS);
    g_pDev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &g_pPS);

    // 设置着色器对象
    g_pDevcon->VSSetShader(g_pVS, 0, 0);
    g_pDevcon->PSSetShader(g_pPS, 0, 0);

    // 创建输入布局对象
    D3D11_INPUT_ELEMENT_DESC ied[] =
     {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };


    g_pDev->CreateInputLayout(ied, 2, VS->GetBufferPointer(), VS->GetBufferSize(), &g_pLayout);
    g_pDevcon->IASetInputLayout(g_pLayout);

    VS->Release();
    PS->Release();
}

//这是创建要呈现的形状的函数
void InitGraphics() {
    // 使用顶点结构创建一个三角形
    VERTEX OurVertices[] =
    {
        {XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(0.45f, -0.5, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-0.45f, -0.5f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)}
    };

    //创建顶点缓冲区
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
    bd.ByteWidth = sizeof(VERTEX) * 3;             // size is the VERTEX struct * 3
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // 用作顶点缓冲区
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // 允许CPU写入缓冲区

    g_pDev->CreateBuffer(&bd, NULL, &g_pVBuffer);       // create the buffer

    // 将顶点复制到缓冲区中
    D3D11_MAPPED_SUBRESOURCE ms;
    g_pDevcon->Map(g_pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // 映射缓冲区
    memcpy(ms.pData, OurVertices, sizeof(VERTEX) * 3);                       // copy the data
    g_pDevcon->Unmap(g_pVBuffer, NULL);                                      // 映射缓冲区
}

//此函数为使用图形资源做准备
HRESULT CreateGraphicsResources(HWND hWnd)
{
    HRESULT hr = S_OK;

    if (g_pSwapchain == nullptr)
    {
        //创建一个结构来保存关于交换链的信息
        DXGI_SWAP_CHAIN_DESC scd;

        // 清除结构以供使用
        ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

        //填充交换链描述结构
        scd.BufferCount = 1;                                    // 回一个缓冲区
        scd.BufferDesc.Width = SCREEN_WIDTH;
        scd.BufferDesc.Height = SCREEN_HEIGHT;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     //使用32位的颜色
        scd.BufferDesc.RefreshRate.Numerator = 60;
        scd.BufferDesc.RefreshRate.Denominator = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // 如何使用交换链
        scd.OutputWindow = hWnd;                                // 要使用的窗口
        scd.SampleDesc.Count = 4;                               //多少个多样本
        scd.Windowed = TRUE;                                    // 视窗化/全屏模式
        scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;     //允许全屏切换

        const D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_1,
                                                    D3D_FEATURE_LEVEL_11_0,
                                                    D3D_FEATURE_LEVEL_10_1,
                                                    D3D_FEATURE_LEVEL_10_0,
                                                    D3D_FEATURE_LEVEL_9_3,
                                                    D3D_FEATURE_LEVEL_9_2,
                                                    D3D_FEATURE_LEVEL_9_1};
        D3D_FEATURE_LEVEL FeatureLevelSupported;

        HRESULT hr = S_OK;

        // 使用scd结构中的信息创建设备、设备上下文和交换链
        hr = D3D11CreateDeviceAndSwapChain(NULL,
                                      D3D_DRIVER_TYPE_HARDWARE,
                                      NULL,
                                      0,
                                      FeatureLevels,
                                      _countof(FeatureLevels),
                                      D3D11_SDK_VERSION,
                                      &scd,
                                      &g_pSwapchain,
                                      &g_pDev,
                                      &FeatureLevelSupported,
                                      &g_pDevcon);

        if (hr == E_INVALIDARG) {
            hr = D3D11CreateDeviceAndSwapChain(NULL,
                                      D3D_DRIVER_TYPE_HARDWARE,
                                      NULL,
                                      0,
                                      &FeatureLevelSupported,
                                      1,
                                      D3D11_SDK_VERSION,
                                      &scd,
                                      &g_pSwapchain,
                                      &g_pDev,
                                      NULL,
                                      &g_pDevcon);
        }

        if (hr == S_OK) {
            CreateRenderTarget();
            SetViewPort();
            InitPipeline();
            InitGraphics();
         }
    }
    
    return hr;
}


//释放图形资源
void DiscardGraphicsResources()
{
    SafeRelease(&g_pLayout);
    SafeRelease(&g_pVS);
    SafeRelease(&g_pPS);
    SafeRelease(&g_pVBuffer);
    SafeRelease(&g_pSwapchain);
    SafeRelease(&g_pRTView);
    SafeRelease(&g_pDev);
    SafeRelease(&g_pDevcon);
}

//这是用来渲染单个帧的函数
void RenderFrame()
{
    // clear the back buffer to a deep blue
    const FLOAT clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
    g_pDevcon->ClearRenderTargetView(g_pRTView, clearColor);

    // do 3D rendering on the back buffer here
    {
        // select which vertex buffer to display
        UINT stride = sizeof(VERTEX);
        UINT offset = 0;
        g_pDevcon->IASetVertexBuffers(0, 1, &g_pVBuffer, &stride, &offset);

        // select which primtive type we are using
        g_pDevcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // draw the vertex buffer to the back buffer
        g_pDevcon->Draw(3, 0);
    }

    // swap the back buffer and the front buffer
    g_pSwapchain->Present(0, 0);
}



// WindowProc函数原型
LRESULT CALLBACK WindowProc(HWND hWnd,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam);


// 任何Windows程序的入口点
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPTSTR lpCmdLine,
                   int nCmdShow)
{
    // 窗口的句柄，由函数填充
    HWND hWnd;
    // 这个结构包含窗口类的信息
    WNDCLASSEX wc;

    // 清除窗口类以供使用
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // 用所需的信息填充结构
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = _T("WindowClass1");

    // 注册窗口类
    RegisterClassEx(&wc);

    // 创建窗口并使用结果作为句柄
    hWnd = CreateWindowEx(0,
                          _T("WindowClass1"),                   // 窗口类的名称
                          _T("Hello, Engine![Direct 3D]"),      // 窗口标题
                          WS_OVERLAPPEDWINDOW,                  // 窗口样式
                          100,                                  // 窗口的x位置
                          100,                                  // 窗口的y位置
                          SCREEN_WIDTH,                         // 窗口宽度
                          SCREEN_HEIGHT,                        // 窗口高度
                          NULL,                                 // 我们没有父窗口，NULL
                          NULL,                                 // 我们没有使用菜单，空
                          hInstance,                            //应用程序处理
                          NULL);                                // 与多个窗口一起使用，NULL

    // 在屏幕上显示窗口
    ShowWindow(hWnd, nCmdShow);

    // 这个结构体包含Windows事件消息
    MSG msg;

    // 等待队列中的下一条消息，将结果存储在'msg'中
    while(GetMessage(&msg, nullptr, 0, 0))
    {
        // 将击键消息转换为正确的格式
        TranslateMessage(&msg);

        //将消息发送到WindowProc函数
        DispatchMessage(&msg);
    }

    // 将WM_QUIT消息的这一部分返回给Windows
    return msg.wParam;
}

//这是程序的主要消息处理程序
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	LRESULT result = 0;
    bool wasHandled = false;
    
	// 排序并找出要为给定的消息运行什么代码
    switch(message)
    {
	    case WM_CREATE:
			wasHandled = true;
        break;


		case WM_PAINT:
			result = CreateGraphicsResources(hWnd);
			RenderFrame();
			wasHandled = true;
        break;

        case WM_SIZE:
            if (g_pSwapchain != nullptr)
		   {
				DiscardGraphicsResources();
		   }
		   wasHandled = true;
        break;

        case WM_DESTROY:
		    DiscardGraphicsResources();
		    PostQuitMessage(0);
            wasHandled = true;
        break;

		case WM_DISPLAYCHANGE:
			InvalidateRect(hWnd, nullptr, false);
			wasHandled = true;
		break;

    }

    // 处理switch语句没有处理的任何消息
	if (!wasHandled) { result = DefWindowProc (hWnd, message, wParam, lParam); }
    return result;
}
