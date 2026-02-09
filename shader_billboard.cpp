/*==============================================================================

  Billboard shader wrapper [shader_billboard.cpp]
  Author : Tanaka Kouki
  Date   : 2025/11/14

  Fix notes (2026-01-19):
  - VS/PS interface + constant buffer slots were mismatched.
  - View/Projection/UV/Color constant buffers are now created and bound correctly.

==============================================================================*/

#include "shader_billboard.h"
#include "debug_ostream.h"
#include "direct3d.h"
#include "sampler.h"

#include <d3d11.h>
#include <fstream>

using namespace DirectX;

static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11InputLayout*  g_pInputLayout  = nullptr;
static ID3D11PixelShader*  g_pPixelShader  = nullptr;

// VS: b0 world, b1 view, b2 projection, b3 uv
static ID3D11Buffer* g_pVSConstantBufferWorld = nullptr;
static ID3D11Buffer* g_pVSConstantBufferView  = nullptr;
static ID3D11Buffer* g_pVSConstantBufferProj  = nullptr;
static ID3D11Buffer* g_pVSConstantBufferUV    = nullptr;

// PS: b0 color
static ID3D11Buffer* g_pPSConstantBufferColor = nullptr;

static bool LoadFileBinary(const char* path, unsigned char** outData, size_t* outSize)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return false;
    }
    ifs.seekg(0, std::ios::end);
    const std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    if (size <= 0) {
        return false;
    }

    unsigned char* data = new unsigned char[(size_t)size];
    ifs.read((char*)data, size);
    ifs.close();

    *outData = data;
    *outSize = (size_t)size;
    return true;
}

bool ShaderBillboard_Initialize()
{
    HRESULT hr;

    // --- Vertex shader ---
    unsigned char* vsBin = nullptr;
    size_t vsSize = 0;
    if (!LoadFileBinary("shader_vertex_billboard.cso", &vsBin, &vsSize)) {
        MessageBox(nullptr,
            TEXT("Failed to load shader_vertex_billboard.cso"),
            TEXT("Error"), MB_OK);
        return false;
    }

    hr = Direct3D_GetDevice()->CreateVertexShader(vsBin, vsSize, nullptr, &g_pVertexShader);
    if (FAILED(hr)) {
        hal::dout << "ShaderBillboard_Initialize(): CreateVertexShader failed" << std::endl;
        delete[] vsBin;
        return false;
    }

    // Input layout must match billboard vertex buffer (POSITION/COLOR/TEXCOORD)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = Direct3D_GetDevice()->CreateInputLayout(layout, (UINT)ARRAYSIZE(layout), vsBin, vsSize, &g_pInputLayout);
    delete[] vsBin;
    if (FAILED(hr)) {
        hal::dout << "ShaderBillboard_Initialize(): CreateInputLayout failed" << std::endl;
        return false;
    }

    // --- Constant buffers ---
    {
        D3D11_BUFFER_DESC bd{};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        bd.ByteWidth = sizeof(XMFLOAT4X4);
        if (FAILED(Direct3D_GetDevice()->CreateBuffer(&bd, nullptr, &g_pVSConstantBufferWorld))) return false;
        if (FAILED(Direct3D_GetDevice()->CreateBuffer(&bd, nullptr, &g_pVSConstantBufferView)))  return false;
        if (FAILED(Direct3D_GetDevice()->CreateBuffer(&bd, nullptr, &g_pVSConstantBufferProj)))  return false;

        bd.ByteWidth = sizeof(UVParameter); // 16 bytes
        if (FAILED(Direct3D_GetDevice()->CreateBuffer(&bd, nullptr, &g_pVSConstantBufferUV))) return false;

        bd.ByteWidth = sizeof(XMFLOAT4); // 16 bytes
        if (FAILED(Direct3D_GetDevice()->CreateBuffer(&bd, nullptr, &g_pPSConstantBufferColor))) return false;
    }

    // --- Pixel shader ---
    unsigned char* psBin = nullptr;
    size_t psSize = 0;
    if (!LoadFileBinary("shader_pixel_billboard.cso", &psBin, &psSize)) {
        MessageBox(nullptr,
            TEXT("Failed to load shader_pixel_billboard.cso"),
            TEXT("Error"), MB_OK);
        return false;
    }

    hr = Direct3D_GetDevice()->CreatePixelShader(psBin, psSize, nullptr, &g_pPixelShader);
    delete[] psBin;
    if (FAILED(hr)) {
        hal::dout << "ShaderBillboard_Initialize(): CreatePixelShader failed" << std::endl;
        return false;
    }

    // Sampler state is managed globally in sampler.cpp
    Sampler_SetFilterAnisotropic();

    // Default values (avoid using uninitialized CBs)
    ShaderBillboard_SetWorldMatrix(XMMatrixIdentity());
    ShaderBillboard_SetViewMatrix(XMMatrixIdentity());
    ShaderBillboard_SetProjectionMatrix(XMMatrixIdentity());
    ShaderBillboard_SetUVParameter({ {1.0f, 1.0f}, {0.0f, 0.0f} });
    ShaderBillboard_SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

    return true;
}

void ShaderBillboard_Finalize()
{
    SAFE_RELEASE(g_pPSConstantBufferColor);
    SAFE_RELEASE(g_pVSConstantBufferUV);
    SAFE_RELEASE(g_pVSConstantBufferProj);
    SAFE_RELEASE(g_pVSConstantBufferView);
    SAFE_RELEASE(g_pVSConstantBufferWorld);
    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pPixelShader);
    SAFE_RELEASE(g_pVertexShader);
}

static void UpdateMatrixCB(ID3D11Buffer* cb, const XMMATRIX& m)
{
    XMFLOAT4X4 t;
    XMStoreFloat4x4(&t, XMMatrixTranspose(m));
    Direct3D_GetContext()->UpdateSubresource(cb, 0, nullptr, &t, 0, 0);
}

void ShaderBillboard_SetWorldMatrix(const XMMATRIX& matrix)
{
    UpdateMatrixCB(g_pVSConstantBufferWorld, matrix);
}

void ShaderBillboard_SetViewMatrix(const XMMATRIX& matrix)
{
    UpdateMatrixCB(g_pVSConstantBufferView, matrix);
}

void ShaderBillboard_SetProjectionMatrix(const XMMATRIX& matrix)
{
    UpdateMatrixCB(g_pVSConstantBufferProj, matrix);
}

void ShaderBillboard_SetColor(const XMFLOAT4& color)
{
    Direct3D_GetContext()->UpdateSubresource(g_pPSConstantBufferColor, 0, nullptr, &color, 0, 0);
}

void ShaderBillboard_SetUVParameter(const UVParameter& parameter)
{
    Direct3D_GetContext()->UpdateSubresource(g_pVSConstantBufferUV, 0, nullptr, &parameter, 0, 0);
}

void ShaderBillboard_Begin()
{
    auto* ctx = Direct3D_GetContext();

    ctx->VSSetShader(g_pVertexShader, nullptr, 0);
    ctx->PSSetShader(g_pPixelShader, nullptr, 0);
    ctx->IASetInputLayout(g_pInputLayout);

    ID3D11Buffer* vsCBs[4] = {
        g_pVSConstantBufferWorld,
        g_pVSConstantBufferView,
        g_pVSConstantBufferProj,
        g_pVSConstantBufferUV,
    };
    ctx->VSSetConstantBuffers(0, 4, vsCBs);

    ctx->PSSetConstantBuffers(0, 1, &g_pPSConstantBufferColor);
}
