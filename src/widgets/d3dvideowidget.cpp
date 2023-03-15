/*
 * Copyright (c) 2023 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "d3dvideowidget.h"
#include <Logger.h>

#include <d3dcompiler.h>


D3DVideoWidget::D3DVideoWidget(QObject *parent)
    : Mlt::VideoWidget{parent}
{
    m_maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    ::memset(&m_constants, 0, sizeof(m_constants));
}

D3DVideoWidget::~D3DVideoWidget()
{
    for (int i = 0; i < 3; i++) {
        if (m_texture[i])
            m_texture[i]->Release();
    }
    if (m_vs)
        m_vs->Release();
    if (m_ps)
        m_ps->Release();
    if (m_vbuf)
        m_vbuf->Release();
    if (m_cbuf)
        m_cbuf->Release();
    if (m_inputLayout)
        m_inputLayout->Release();
    if (m_rastState)
        m_rastState->Release();
    if (m_dsState)
        m_dsState->Release();
}

void D3DVideoWidget::initialize()
{
    m_initialized = true;
    QSGRendererInterface *rif = quickWindow()->rendererInterface();

    // We are not prepared for anything other than running with the RHI and its D3D11 backend.
    Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::Direct3D11);

    m_device = reinterpret_cast<ID3D11Device *>(rif->getResource(quickWindow(),
                                                                 QSGRendererInterface::DeviceResource));
    Q_ASSERT(m_device);
    m_context = reinterpret_cast<ID3D11DeviceContext *>(rif->getResource(quickWindow(),
                                                                         QSGRendererInterface::DeviceContextResource));
    Q_ASSERT(m_context);

    if (m_vert.isEmpty())
        prepareShader(VertexStage);
    if (m_frag.isEmpty())
        prepareShader(FragmentStage);

    const QByteArray vs = compileShader(VertexStage, m_vert, m_vertEntryPoint);
    const QByteArray fs = compileShader(FragmentStage, m_frag, m_fragEntryPoint);

    HRESULT hr = m_device->CreateVertexShader(vs.constData(), vs.size(), nullptr, &m_vs);
    if (FAILED(hr))
        qFatal("Failed to create vertex shader: 0x%x", uint(hr));

    hr = m_device->CreatePixelShader(fs.constData(), fs.size(), nullptr, &m_ps);
    if (FAILED(hr))
        qFatal("Failed to create pixel shader: 0x%x", uint(hr));

    D3D11_BUFFER_DESC bufDesc;
    memset(&bufDesc, 0, sizeof(bufDesc));
    bufDesc.ByteWidth = sizeof(float) * 16;
    bufDesc.Usage = D3D11_USAGE_DEFAULT;
    bufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    hr = m_device->CreateBuffer(&bufDesc, nullptr, &m_vbuf);
    if (FAILED(hr))
        qFatal("Failed to create buffer: 0x%x", uint(hr));

    bufDesc.ByteWidth = sizeof(m_constants) + 0xf & 0xfffffff0; // must be a multiple of 16
    bufDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_device->CreateBuffer(&bufDesc, nullptr, &m_cbuf);
    if (FAILED(hr))
        qFatal("Failed to create buffer: 0x%x", uint(hr));

    const D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
        { "VERTEX", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(DirectX::XMFLOAT2), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = m_device->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), vs.constData(), vs.size(),
                                     &m_inputLayout);
    if (FAILED(hr))
        qFatal("Failed to create input layout: 0x%x", uint(hr));

    D3D11_RASTERIZER_DESC rastDesc;
    memset(&rastDesc, 0, sizeof(rastDesc));
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;
    hr = m_device->CreateRasterizerState(&rastDesc, &m_rastState);
    if (FAILED(hr))
        qFatal("Failed to create rasterizer state: 0x%x", uint(hr));

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    memset(&dsDesc, 0, sizeof(dsDesc));
    hr = m_device->CreateDepthStencilState(&dsDesc, &m_dsState);
    if (FAILED(hr))
        qFatal("Failed to create depth/stencil state: 0x%x", uint(hr));

    Mlt::VideoWidget::initialize();
}

void D3DVideoWidget::beforeRendering()
{
    quickWindow()->beginExternalCommands();
    m_context->ClearState();

    // Provide vertices of triangle strip
    float width = rect().width() * devicePixelRatioF() / 2.0f;
    float height = rect().height() * devicePixelRatioF() / 2.0f;
    float vertexData[] = { // x,y plus u,v texture coordinates
        width,  -height, 1.f, 1.f, // bottom left
        -width,  -height, 0.f, 1.f, // bottom right
        width,  height, 1.f, 0.f, // top left
        -width,  height, 0.f, 0.f   // top right
    };

    // Setup an orthographic projection
    QMatrix4x4 modelView;
    width = this->width() * devicePixelRatioF();
    height = this->height() * devicePixelRatioF();
    modelView.scale(2.0f / width, 2.0f / height);

    // Set model-view
    if (rect().width() > 0.0 && zoom() > 0.0) {
        if (offset().x() || offset().y())
            modelView.translate(-offset().x() * devicePixelRatioF(),
                                offset().y() * devicePixelRatioF());
        modelView.scale(zoom(), zoom());
    }
    for (int i = 0; i < 4; i++) {
        vertexData[4 * i] *= modelView(0, 0);
        vertexData[4 * i] += modelView(0, 3);
        vertexData[4 * i + 1] *= modelView(1, 1);
        vertexData[4 * i + 1] += modelView(1, 3);
    }
    m_context->UpdateSubresource(m_vbuf, 0, nullptr, vertexData, 0, 0);

    // (Re)create the textures
    m_mutex.lock();
    if (!m_sharedFrame.is_valid()) {
        m_mutex.unlock();
        quickWindow()->endExternalCommands();
        Mlt::VideoWidget::beforeRendering();
        return;
    }
    int iwidth = m_sharedFrame.get_image_width();
    int iheight = m_sharedFrame.get_image_height();
    const uint8_t *image = m_sharedFrame.get_image(mlt_image_yuv420p);
    for (int i = 0; i < 3; i++) {
        if (m_texture[i])
            m_texture[i]->Release();
    }
    m_texture[0] = initTexture(image, iwidth, iheight);
    m_texture[1] = initTexture(image + iwidth * iheight, iwidth / 2, iheight / 2);
    m_texture[2] = initTexture(image + iwidth * iheight + iwidth / 2 * iheight / 2, iwidth / 2,
                               iheight / 2);
    m_mutex.unlock();

    // Update the constants
    D3D11_MAPPED_SUBRESOURCE mp;
    // will copy the entire constant buffer every time -> pass WRITE_DISCARD -> prevent pipeline stalls
    HRESULT hr = m_context->Map(m_cbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mp);
    if (SUCCEEDED(hr)) {
        m_constants.colorspace = MLT.profile().colorspace();
        ::memcpy(mp.pData, &m_constants, sizeof(m_constants));
        m_context->Unmap(m_cbuf, 0);
    } else {
        quickWindow()->endExternalCommands();
        qFatal("Failed to map constant buffer: 0x%x", uint(hr));
        return;
    }

    quickWindow()->endExternalCommands();
    Mlt::VideoWidget::beforeRendering();
}

void D3DVideoWidget::renderVideo()
{
    if (!m_texture[0]) {
        Mlt::VideoWidget::renderVideo();
        return;
    }
    quickWindow()->beginExternalCommands();

    D3D11_VIEWPORT v;
    v.TopLeftX = 0.f;
    v.TopLeftY = 0.f;
    v.Width = this->width() * devicePixelRatioF();
    v.Height = this->height() * devicePixelRatioF();
    v.MinDepth = 0.f;
    v.MaxDepth = 1.f;

    m_context->RSSetViewports(1, &v);
    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetShader(m_ps, nullptr, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_context->IASetInputLayout(m_inputLayout);
    m_context->OMSetDepthStencilState(m_dsState, 0);
    m_context->RSSetState(m_rastState);
    const UINT stride = sizeof(float) * 4;
    const UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vbuf, &stride, &offset);
    m_context->PSSetConstantBuffers(0, 1, &m_cbuf);
    m_context->PSSetShaderResources(0, 3, m_texture);
    m_context->Draw(4, 0);

    quickWindow()->endExternalCommands();
    Mlt::VideoWidget::renderVideo();
}

void D3DVideoWidget::prepareShader(Stage stage)
{
    if (stage == VertexStage) {
        m_vert = "struct VSInput {"
                 "  float2 vertex : VERTEX;"
                 "  float2 coords : TEXCOORD;"
                 "};"
                 "struct VSOutput {"
                 "  float2 coords : TEXCOORD0;"
                 "  float4 position : SV_Position;"
                 "};"
                 "VSOutput main(VSInput input) {"
                 "  VSOutput output;"
                 "  output.position = float4(input.vertex, 0.0f, 1.0f);"
                 "  output.coords = input.coords;"
                 "  return output;"
                 "}";
        Q_ASSERT(!m_vert.isEmpty());
        m_vertEntryPoint = QByteArrayLiteral("main");
    } else {
        m_frag = "Texture2D yTex, uTex, vTex;"
                 "SamplerState yuvSampler;"
                 "cbuffer buf {"
                 "    int colorspace;"
                 "};"
                 "struct PSInput {"
                 "  float2 coords : TEXCOORD0;"
                 "};"
                 "struct PSOutput {"
                 "  float4 color : SV_Target0;"
                 "};"
                 "PSOutput main(PSInput input) {"
                 "  float3 yuv;"
                 "  yuv.x = yTex.Sample(yuvSampler, input.coords).r -  16.0f/255.0f;"
                 "  yuv.y = uTex.Sample(yuvSampler, input.coords).r - 128.0f/255.0f;"
                 "  yuv.z = vTex.Sample(yuvSampler, input.coords).r - 128.0f/255.0f;"
                 "  float3x3 coefficients;"
                 "  if (colorspace == 601) {"
                 "    coefficients = float3x3("
                 "      1.1643f,  0.0f,      1.5958f,"
                 "      1.1643f, -0.39173f, -0.8129f,"
                 "      1.1643f,  2.017f,    0.0f);"
                 "  } else {" // ITU-R 709
                 "    coefficients = float3x3("
                 "      1.1643f,  0.0f,    1.793f,"
                 "      1.1643f, -0.213f, -0.533f,"
                 "      1.1643f,  2.112f,  0.0f);"
                 "  }"
                 "  PSOutput output;"
                 "  output.color = float4(mul(coefficients, yuv), 1.0f);"
                 "  return output;"
                 "}";
        m_fragEntryPoint = QByteArrayLiteral("main");
    }
}

QByteArray D3DVideoWidget::compileShader(Stage stage, const QByteArray &source,
                                         const QByteArray &entryPoint)
{
    const char *target;
    switch (stage) {
    case VertexStage:
        target = "vs_5_0";
        break;
    case FragmentStage:
        target = "ps_5_0";
        break;
    default:
        qFatal("Unknown shader stage %d", stage);
        return QByteArray();
    }

    ID3DBlob *bytecode = nullptr;
    ID3DBlob *errors = nullptr;
    HRESULT hr = D3DCompile(source.constData(), source.size(),
                            nullptr, nullptr, nullptr,
                            entryPoint.constData(), target, 0, 0, &bytecode, &errors);
    if (FAILED(hr) || !bytecode) {
        qWarning("HLSL shader compilation failed: 0x%x", uint(hr));
        if (errors) {
            const QByteArray msg(static_cast<const char *>(errors->GetBufferPointer()),
                                 errors->GetBufferSize());
            errors->Release();
            qWarning("%s", msg.constData());
        }
        return QByteArray();
    }

    QByteArray result;
    result.resize(bytecode->GetBufferSize());
    memcpy(result.data(), bytecode->GetBufferPointer(), result.size());
    bytecode->Release();

    return result;
}

ID3D11ShaderResourceView *D3DVideoWidget::initTexture(const void *p, int width, int height)
{
    ID3D11ShaderResourceView *result;
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA subresourceData;
    subresourceData.pSysMem = p;
    subresourceData.SysMemPitch = width;
    subresourceData.SysMemSlicePitch = 0;

    ID3D11Texture2D *texture;
    m_device->CreateTexture2D(&desc, &subresourceData, &texture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    m_device->CreateShaderResourceView(texture, &srvDesc, &result);
    texture->Release();

    return result;
}
