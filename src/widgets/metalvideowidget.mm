/*
 * Copyright (c) 2023-2025 Meltytech, LLC
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

#include "metalvideowidget.h"

#include "Logger.h"

#include <Metal/Metal.h>


class MetalVideoRenderer : public QObject
{
    Q_OBJECT
public:
    MetalVideoRenderer()
    {
        for (int i = 0; i < 3; ++i) {
            m_ubuf[i] = nil;
            m_texture[i] = nil;
        }
        m_vbuf = nil;
        m_vs.first = nil;
        m_vs.second = nil;
        m_fs.first = nil;
        m_fs.second = nil;
    }

    ~MetalVideoRenderer()
    {
        LOG_DEBUG() << "cleanup";

        for (int i = 0; i < 3; i++) {
            [m_texture[i] release];
            [m_ubuf[i] release];
        }
        [m_vbuf release];
        [m_vs.first release];
        [m_vs.second release];
        [m_fs.first release];
        [m_fs.second release];
    }

    void initialize(QQuickWindow *window)
    {
        LOG_DEBUG() << "init";
        m_window = window;

        QSGRendererInterface *rif = m_window->rendererInterface();

        // We are not prepared for anything other than running with the RHI and its Metal backend.
        Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::Metal);

        m_device = (id<MTLDevice>) rif->getResource(m_window, QSGRendererInterface::DeviceResource);
        Q_ASSERT(m_device);

        if (m_vert.isEmpty())
            prepareShader(VertexStage);
        if (m_frag.isEmpty())
            prepareShader(FragmentStage);

        m_vbuf = [m_device newBufferWithLength: 16*sizeof(float) options: MTLResourceStorageModeShared];

        for (int i = 0; i < m_window->graphicsStateInfo().framesInFlight && i < 3; ++i)
            m_ubuf[i] = [m_device newBufferWithLength: sizeof(int) options: MTLResourceStorageModeShared];

        MTLVertexDescriptor *inputLayout = [MTLVertexDescriptor vertexDescriptor];
        inputLayout.attributes[0].format = MTLVertexFormatFloat4;
        inputLayout.attributes[0].offset = 0;
        inputLayout.attributes[0].bufferIndex = 1; // ubuf is 0, vbuf is 1
        inputLayout.layouts[1].stride = 4 * sizeof(float);

        MTLRenderPipelineDescriptor *rpDesc = [[MTLRenderPipelineDescriptor alloc] init];
        rpDesc.vertexDescriptor = inputLayout;

        m_vs = compileShader(m_vert, m_vertEntryPoint);
        rpDesc.vertexFunction = m_vs.first;
        m_fs = compileShader(m_frag, m_fragEntryPoint);
        rpDesc.fragmentFunction = m_fs.first;

        rpDesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
//        if (m_device.depth24Stencil8PixelFormatSupported) {
//            rpDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
//            rpDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth24Unorm_Stencil8;
//        } else {
//            rpDesc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
//            rpDesc.stencilAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
//        }

        NSError *err = nil;
        m_pipeline = [m_device newRenderPipelineStateWithDescriptor: rpDesc error: &err];
        if (!m_pipeline) {
            const QString msg = QString::fromNSString(err.localizedDescription);
            qFatal("Failed to create render pipeline state: %s", qPrintable(msg));
        }
        [rpDesc release];
    }

    void render(const QSize& viewportSize, const QRectF& videoRect, const double devicePixelRatio,
                const double zoom, const QPoint& offset, const SharedFrame& sharedFrame)
    {
        const QQuickWindow::GraphicsStateInfo &stateInfo(m_window->graphicsStateInfo());

        QSGRendererInterface *rif = m_window->rendererInterface();
        id<MTLRenderCommandEncoder> encoder = (id<MTLRenderCommandEncoder>) rif->getResource(
                    m_window, QSGRendererInterface::CommandEncoderResource);
        Q_ASSERT(encoder);

        // Provide vertices of triangle strip
        float width = videoRect.width() * devicePixelRatio / 2.0f;
        float height = videoRect.height() * devicePixelRatio / 2.0f;
        float vertexData[] = { // x,y plus u,v texture coordinates
            -width,  height, 0.f, 0.f,
             width,  height, 1.f, 0.f,
            -width, -height, 0.f, 1.f,
             width, -height, 1.f, 1.f
        };

        // Setup an orthographic projection
        QMatrix4x4 modelView;
        width = viewportSize.width() * devicePixelRatio;
        height = viewportSize.height() * devicePixelRatio;
        modelView.scale(2.0f / width, 2.0f / height);

        // Set model-view
        if (videoRect.width() > 0.0 && zoom > 0.0) {
            if (offset.x() || offset.y())
                modelView.translate(-offset.x() * devicePixelRatio,
                                    offset.y() * devicePixelRatio);
            modelView.scale(zoom, zoom);
        }
        for (int i = 0; i < 4; i++) {
            vertexData[4 * i] *= modelView(0, 0);
            vertexData[4 * i] += modelView(0, 3);
            vertexData[4 * i + 1] *= modelView(1, 1);
            vertexData[4 * i + 1] += modelView(1, 3);
        }

        m_window->beginExternalCommands();

        void *p = [m_vbuf contents];
        memcpy(p, vertexData, sizeof(vertexData));

        p = [m_ubuf[stateInfo.currentFrameSlot] contents];
        int colorspace = MLT.profile().colorspace();
        memcpy(p, &colorspace, sizeof(colorspace));

        MTLViewport vp;
        vp.originX = 0;
        vp.originY = 0;
        vp.width = width;
        vp.height = height;
        vp.znear = 0;
        vp.zfar = 1;
        [encoder setViewport: vp];

        // (Re)create the textures
        int iwidth = sharedFrame.get_image_width();
        int iheight = sharedFrame.get_image_height();
        const uint8_t *image = sharedFrame.get_image(mlt_image_yuv420p);
        for (int i = 0; i < 3; i++) {
            [m_texture[i] release];
        }
        m_texture[0] = initTexture(image, iwidth, iheight);
        m_texture[1] = initTexture(image + iwidth * iheight, iwidth / 2, iheight / 2);
        m_texture[2] = initTexture(image + iwidth * iheight + iwidth / 2 * iheight / 2, iwidth / 2,
                                   iheight / 2);
        // Set the texture object.  The AAPLTextureIndexBaseColor enum value corresponds
        ///  to the 'colorMap' argument in the 'samplingShader' function because its
        //   texture attribute qualifier also uses AAPLTextureIndexBaseColor for its index.
        for (NSUInteger i = 0; i < 3; i++) {
            [encoder setFragmentTexture:m_texture[i] atIndex:i];
        }


        [encoder setFragmentBuffer: m_ubuf[stateInfo.currentFrameSlot] offset: 0 atIndex: 0];
        [encoder setVertexBuffer: m_vbuf offset: 0 atIndex: 1];
        [encoder setRenderPipelineState: m_pipeline];
        [encoder drawPrimitives: MTLPrimitiveTypeTriangleStrip vertexStart: 0 vertexCount: 4 instanceCount: 1 baseInstance: 0];

        m_window->endExternalCommands();
    }

    id<MTLTexture> initTexture(const void *p, NSUInteger width, NSUInteger height)
    {
        MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];

        // 8-bit unsigned normalized value (i.e. 0 maps to 0.0 and 255 maps to 1.0)
        textureDescriptor.pixelFormat = MTLPixelFormatR8Unorm;
        textureDescriptor.width = width;
        textureDescriptor.height = height;
        id<MTLTexture> texture = [m_device newTextureWithDescriptor:textureDescriptor];
        [textureDescriptor release];

        MTLRegion region = {
            { 0, 0, 0 },  // MTLOrigin
            {width, height, 1} // MTLSize
        };

        // Copy the bytes from the data object into the texture
        [texture replaceRegion:region mipmapLevel:0 withBytes:p bytesPerRow:width];
        return texture;
    }

private:
    enum Stage {
        VertexStage,
        FragmentStage
    };
    using FuncAndLib = QPair<id<MTLFunction>, id<MTLLibrary> >;
    QQuickWindow *m_window = nullptr;
    QByteArray m_vert;
    QByteArray m_vertEntryPoint;
    QByteArray m_frag;
    QByteArray m_fragEntryPoint;
    id<MTLDevice> m_device;
    id<MTLBuffer> m_vbuf;
    id<MTLBuffer> m_ubuf[3];
    FuncAndLib m_vs;
    FuncAndLib m_fs;
    id<MTLRenderPipelineState> m_pipeline;
    id<MTLTexture> m_texture[3];

    void prepareShader(Stage stage)
    {
        if (stage == VertexStage) {
            m_vert ="#include <metal_stdlib>\n"
                    "#include <simd/simd.h>\n"
                    "using namespace metal;"
                    "struct main0_out {"
                    "    float2 coords [[user(locn0)]];"
                    "    float4 vertices [[position]];"
                    "};"
                    "struct main0_in {"
                    "    float4 vertices [[attribute(0)]];"
                    "};"
                    "vertex main0_out main0(main0_in in [[stage_in]]) {"
                    "    main0_out out = {};"
                    "    out.vertices = vector_float4(in.vertices.xy, 0.0f, 1.0f);"
                    "    out.coords = in.vertices.zw;"
                    "    return out;"
                    "}";
            Q_ASSERT(!m_vert.isEmpty());
            m_vertEntryPoint = QByteArrayLiteral("main0");
        } else {
            m_frag ="#include <metal_stdlib>\n"
                    "#include <simd/simd.h>\n"
                    "using namespace metal;"
                    "struct buf {"
                    "    int colorspace;"
                    "};"
                    "struct main0_out {"
                    "    float4 fragColor [[color(0)]];"
                    "};"
                    "struct main0_in {"
                    "    float2 coords [[user(locn0)]];"
                    "};"
                    "fragment main0_out main0(main0_in in [[stage_in]], constant buf& ubuf [[buffer(0)]],"
                    "      texture2d<half> yTex [[texture(0)]],"
                    "      texture2d<half> uTex [[texture(1)]],"
                    "      texture2d<half> vTex [[texture(2)]]"
                    "      ) {"
                    "    main0_out out = {};"
                    "    constexpr sampler yuvSampler (mag_filter::linear, min_filter::linear);"
                    "    float3 yuv;"
                    "    yuv.x = yTex.sample(yuvSampler, in.coords).r -  16.0f/255.0f;"
                    "    yuv.y = uTex.sample(yuvSampler, in.coords).r - 128.0f/255.0f;"
                    "    yuv.z = vTex.sample(yuvSampler, in.coords).r - 128.0f/255.0f;"
                    "    float3x3 coefficients;"
                    "    if (ubuf.colorspace == 601) {"
                    "      coefficients = float3x3("
                    "        {1.1643f,  1.1643f,  1.1643f},"
                    "        {0.0f,    -0.39173f, 2.017f},"
                    "        {1.5958f, -0.8129f,  0.0f});"
                    "    } else if (ubuf.colorspace == 2020) {" // ITU-R BT.2020
                    "      coefficients = float3x3("
                    "        {1.1643f, 1.1643f, 1.1643f},"
                    "        {0.0f,   -0.1873f, 2.1418f},"
                    "        {1.7167f, -0.6504f, 0.0f});"
                    "    } else {" // ITU-R 709
                    "      coefficients = float3x3("
                    "        {1.1643f, 1.1643f, 1.1643f},"
                    "        {0.0f,   -0.213f,  2.112f},"
                    "        {1.793f, -0.533f,  0.0f});"
                    "    }"
                    "    out.fragColor = float4(coefficients * yuv, 1.0f);"
                    "    return out;"
                    "}";
            m_fragEntryPoint = QByteArrayLiteral("main0");
        }
    }

    FuncAndLib compileShader(const QByteArray &source, const QByteArray &entryPoint)
    {
        FuncAndLib fl;

        NSString *srcstr = [NSString stringWithUTF8String: source.constData()];
        MTLCompileOptions *opts = [[MTLCompileOptions alloc] init];
        opts.languageVersion = MTLLanguageVersion1_2;
        NSError *err = nil;
        fl.second = [m_device newLibraryWithSource: srcstr options: opts error: &err];
        [opts release];
        // srcstr is autoreleased

        if (err) {
            const QString msg = QString::fromNSString(err.localizedDescription);
            qFatal("%s", qPrintable(msg));
            return fl;
        }

        NSString *name = [NSString stringWithUTF8String: entryPoint.constData()];
        fl.first = [fl.second newFunctionWithName: name];
//        [name release];

        return fl;
    }
};

MetalVideoWidget::MetalVideoWidget(QObject *parent)
    : Mlt::VideoWidget{parent}
    , m_renderer{new MetalVideoRenderer}
{
    m_maxTextureSize = 16384;
}

MetalVideoWidget::~MetalVideoWidget()
{
}

void MetalVideoWidget::initialize()
{
    m_renderer->initialize(quickWindow());
    Mlt::VideoWidget::initialize();
}

void MetalVideoWidget::renderVideo()
{
    m_mutex.lock();
    if (m_sharedFrame.is_valid()) {
        m_renderer->render(size(), rect(), devicePixelRatio(), zoom(), offset(), m_sharedFrame);
    }
    m_mutex.unlock();
    Mlt::VideoWidget::renderVideo();
}

#include "metalvideowidget.moc"
