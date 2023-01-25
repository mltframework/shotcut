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

#ifndef D3DVIDEOWIDGET_H
#define D3DVIDEOWIDGET_H

#include "videowidget.h"

#include <d3d11.h>
#include <directxmath.h>

class D3DVideoWidget : public Mlt::VideoWidget
{
    Q_OBJECT
public:
    explicit D3DVideoWidget(QObject *parent = nullptr);
    virtual ~D3DVideoWidget();

public slots:
    virtual void initialize();
    virtual void renderVideo();

private:
    enum Stage {
        VertexStage,
        FragmentStage
    };
    void prepareShader(Stage stage);
    QByteArray compileShader(Stage stage,
                             const QByteArray &source,
                             const QByteArray &entryPoint);
    ID3D11ShaderResourceView *initTexture(const void *p, int width, int height);

    ID3D11Device *m_device = nullptr;
    ID3D11DeviceContext *m_context = nullptr;
    QByteArray m_vert;
    QByteArray m_vertEntryPoint;
    QByteArray m_frag;
    QByteArray m_fragEntryPoint;

    bool m_initialized = false;
    ID3D11Buffer *m_vbuf = nullptr;
    ID3D11Buffer *m_cbuf = nullptr;
    ID3D11VertexShader *m_vs = nullptr;
    ID3D11PixelShader *m_ps = nullptr;
    ID3D11InputLayout *m_inputLayout = nullptr;
    ID3D11RasterizerState *m_rastState = nullptr;
    ID3D11DepthStencilState *m_dsState = nullptr;
    ID3D11ShaderResourceView *m_texture[3] = {nullptr, nullptr, nullptr};

    struct ConstantBuffer {
        int32_t colorspace;
    };

    ConstantBuffer m_constants;
};

#endif // D3DVIDEOWIDGET_H
