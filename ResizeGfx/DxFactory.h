#pragma once
#include "CommonTypes.h"

class DxFactory
{
public:
	DxFactory();
	~DxFactory();
    DUPL_RETURN Init();

private:
    HRESULT InitializeDesc(_Out_ D3D11_TEXTURE2D_DESC* pTargetDesc, _Out_ RECT* pDestRect);
    DUPL_RETURN MakeRTV();
    void SetViewPort(UINT Width, UINT Height);
    DUPL_RETURN InitShaders();
    DUPL_RETURN DrawFrame();

    ID3D11Device* m_Device;
    ID3D11DeviceContext* m_DeviceContext;
    ID3D11RenderTargetView* m_RTV;
    ID3D11SamplerState* m_SamplerLinear;
    ID3D11BlendState* m_BlendState;
    ID3D11VertexShader* m_VertexShader;
    ID3D11PixelShader* m_PixelShader;
    ID3D11InputLayout* m_InputLayout;
    ID3D11Texture2D* m_SharedSurf;
    IDXGIKeyedMutex* m_KeyMutex;
};

