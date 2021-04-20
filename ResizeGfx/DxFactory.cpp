#include "DxFactory.h"
using namespace DirectX;

DxFactory::DxFactory() : 
	m_Device(nullptr),
	m_DeviceContext(nullptr),
	m_RTV(nullptr),
	m_SamplerLinear(nullptr),
	m_BlendState(nullptr),
	m_VertexShader(nullptr),
	m_PixelShader(nullptr),
	m_InputLayout(nullptr),
	m_SharedSurf(nullptr),
	m_KeyMutex(nullptr)
{
}

DxFactory::~DxFactory()
{
}

DUPL_RETURN DxFactory::Init() 
{
    HRESULT hr;

    // Driver types supported
    D3D_DRIVER_TYPE DriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    // Feature levels supported
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
    D3D_FEATURE_LEVEL FeatureLevel;

    // Create device
    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
    {
        hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
            D3D11_SDK_VERSION, &m_Device, &FeatureLevel, &m_DeviceContext);
        if (SUCCEEDED(hr))
        {
            // Device creation succeeded, no need to loop anymore
            break;
        }
    }
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    // Create target texture
    D3D11_TEXTURE2D_DESC targetDesc;
    RECT targetRect;
    InitializeDesc(&targetDesc, &targetRect);
    hr = m_Device->CreateTexture2D(&targetDesc, nullptr, &m_SharedSurf);

    // Make new render target view
    DUPL_RETURN Return = MakeRTV();
    if (Return != DUPL_RETURN_SUCCESS)
    {
        return Return;
    }

    // Set view port
    SetViewPort(640, 480);

    // Create the sample state
    D3D11_SAMPLER_DESC SampDesc;
    RtlZeroMemory(&SampDesc, sizeof(SampDesc));
    SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    SampDesc.MinLOD = 0;
    SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_Device->CreateSamplerState(&SampDesc, &m_SamplerLinear);
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    // Create the blend state
    D3D11_BLEND_DESC BlendStateDesc;
    BlendStateDesc.AlphaToCoverageEnable = FALSE;
    BlendStateDesc.IndependentBlendEnable = FALSE;
    BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    hr = m_Device->CreateBlendState(&BlendStateDesc, &m_BlendState);
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    // Initialize shaders
    Return = InitShaders();
    if (Return != DUPL_RETURN_SUCCESS)
    {
        return Return;
    }

    Return = DrawFrame();

    return Return;

}

HRESULT DxFactory::InitializeDesc(_Out_ D3D11_TEXTURE2D_DESC* pTargetDesc, _Out_ RECT* pDestRect) {
    UINT monitorWidth = 640;
    UINT monitorHeight = 480;

    RECT sourceRect;
    sourceRect.left = 0;
    sourceRect.right = monitorWidth;
    sourceRect.top = 0;
    sourceRect.bottom = monitorHeight;

    RECT destRect = sourceRect;

    // Create shared texture for all duplication threads to draw into
    D3D11_TEXTURE2D_DESC DeskTexD;
    RtlZeroMemory(&DeskTexD, sizeof(D3D11_TEXTURE2D_DESC));
    DeskTexD.Width = destRect.right - destRect.left;
    DeskTexD.Height = destRect.bottom - destRect.top;
    DeskTexD.MipLevels = 1;
    DeskTexD.ArraySize = 1;
    DeskTexD.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    DeskTexD.SampleDesc.Count = 1;
    DeskTexD.Usage = D3D11_USAGE_DEFAULT;
    DeskTexD.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    DeskTexD.CPUAccessFlags = 0;
    DeskTexD.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    *pDestRect = destRect;
    *pTargetDesc = DeskTexD;
    return S_OK;
}

//
// Reset render target view
//
DUPL_RETURN DxFactory::MakeRTV()
{
    // Create a render target view
    HRESULT hr = m_Device->CreateRenderTargetView(m_SharedSurf, nullptr, &m_RTV);
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    // Set new render target
    m_DeviceContext->OMSetRenderTargets(1, &m_RTV, nullptr);

    return DUPL_RETURN_SUCCESS;
}

//
// Set new viewport
//
void DxFactory::SetViewPort(UINT Width, UINT Height)
{
    D3D11_VIEWPORT VP;
    VP.Width = static_cast<FLOAT>(Width);
    VP.Height = static_cast<FLOAT>(Height);
    VP.MinDepth = 0.0f;
    VP.MaxDepth = 1.0f;
    VP.TopLeftX = 0;
    VP.TopLeftY = 0;
    m_DeviceContext->RSSetViewports(1, &VP);
}
//
// Initialize shaders for drawing to screen
//
DUPL_RETURN DxFactory::InitShaders()
{
    HRESULT hr;

    UINT Size = ARRAYSIZE(g_VS);
    hr = m_Device->CreateVertexShader(g_VS, Size, nullptr, &m_VertexShader);
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    D3D11_INPUT_ELEMENT_DESC Layout[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    UINT NumElements = ARRAYSIZE(Layout);
    hr = m_Device->CreateInputLayout(Layout, NumElements, g_VS, Size, &m_InputLayout);
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }
    m_DeviceContext->IASetInputLayout(m_InputLayout);

    Size = ARRAYSIZE(g_PS);
    hr = m_Device->CreatePixelShader(g_PS, Size, nullptr, &m_PixelShader);
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    return DUPL_RETURN_SUCCESS;
}
DUPL_RETURN DxFactory::DrawFrame()
{
    HRESULT hr;

    // Vertices for drawing whole texture
    VERTEX Vertices[NUMVERTICES] =
    {
        {XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f)},
    };

    D3D11_TEXTURE2D_DESC FrameDesc;
    m_SharedSurf->GetDesc(&FrameDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC ShaderDesc;
    ShaderDesc.Format = FrameDesc.Format;
    ShaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    ShaderDesc.Texture2D.MostDetailedMip = FrameDesc.MipLevels - 1;
    ShaderDesc.Texture2D.MipLevels = FrameDesc.MipLevels;

    // Create new shader resource view
    ID3D11ShaderResourceView* ShaderResource = nullptr;
    hr = m_Device->CreateShaderResourceView(m_SharedSurf, &ShaderDesc, &ShaderResource);
    if (FAILED(hr))
    {
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }

    // Set resources
    UINT Stride = sizeof(VERTEX);
    UINT Offset = 0;
    FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    m_DeviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
    m_DeviceContext->OMSetRenderTargets(1, &m_RTV, nullptr);
    m_DeviceContext->VSSetShader(m_VertexShader, nullptr, 0);
    m_DeviceContext->PSSetShader(m_PixelShader, nullptr, 0);
    m_DeviceContext->PSSetShaderResources(0, 1, &ShaderResource);
    m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinear);
    m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D11_BUFFER_DESC BufferDesc;
    RtlZeroMemory(&BufferDesc, sizeof(BufferDesc));
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    RtlZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = Vertices;

    ID3D11Buffer* VertexBuffer = nullptr;

    // Create vertex buffer
    hr = m_Device->CreateBuffer(&BufferDesc, &InitData, &VertexBuffer);
    if (FAILED(hr))
    {
        ShaderResource->Release();
        ShaderResource = nullptr;
        return DUPL_RETURN_ERROR_UNEXPECTED;
    }
    m_DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);

    // Draw textured quad onto render target
    m_DeviceContext->Draw(NUMVERTICES, 0);

    VertexBuffer->Release();
    VertexBuffer = nullptr;

    // Release shader resource
    ShaderResource->Release();
    ShaderResource = nullptr;

    return DUPL_RETURN_SUCCESS;
}
