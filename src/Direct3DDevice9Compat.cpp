#include <Platform/DirectXMathCompat.h>

// Force link ImageLoaderCompat.cpp from static libEterBase.a
extern bool Platform_GetImageInfo(const void* data, size_t size, uint32_t* outW, uint32_t* outH, uint32_t* outFormat);
extern bool Platform_DecodeImage(const void* data, size_t size, uint32_t* outW, uint32_t* outH, std::vector<uint8_t>& outPixels);
void* g_pfnForceLinkImageLoader[] = {
    (void*)&Platform_GetImageInfo,
    (void*)&Platform_DecodeImage
};

#if defined(OPENMT2_PLATFORM_LINUX)
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>
#include <cstring>

#ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_CONSTANT
#define GL_CONSTANT 0x8576
#endif
#ifndef GL_PRIMARY_COLOR
#define GL_PRIMARY_COLOR 0x8577
#endif
#ifndef GL_PREVIOUS
#define GL_PREVIOUS 0x8578
#endif
#ifndef GL_COMBINE
#define GL_COMBINE 0x8570
#define GL_COMBINE_RGB 0x8571
#define GL_COMBINE_ALPHA 0x8572
#define GL_SRC0_RGB 0x8580
#define GL_SRC1_RGB 0x8581
#define GL_SRC2_RGB 0x8582
#define GL_SRC0_ALPHA 0x8588
#define GL_SRC1_ALPHA 0x8589
#define GL_SRC2_ALPHA 0x858A
#define GL_OPERAND0_RGB 0x8590
#define GL_OPERAND1_RGB 0x8591
#define GL_OPERAND2_RGB 0x8592
#define GL_OPERAND0_ALPHA 0x8598
#define GL_OPERAND1_ALPHA 0x8599
#define GL_OPERAND2_ALPHA 0x859A
#define GL_RGB_SCALE 0x8573
#define GL_ALPHA_SCALE 0x0D1C
#define GL_ADD_SIGNED 0x8574
#define GL_INTERPOLATE 0x8575
#define GL_SUBTRACT 0x84E7
#endif

// =============================================================
// IDirect3DTexture9 Implementation
// =============================================================

IDirect3DTexture9::IDirect3DTexture9(UINT w, UINT h, D3DFORMAT fmt, UINT /*levels*/)
    : m_width(w ? w : 1), m_height(h ? h : 1), m_format(fmt), m_glTextureId(0), m_dirty(false)
{
    size_t sz = GetLevelDataSize(0);
    m_data.resize(sz ? sz : 4, 0);
}

IDirect3DTexture9::~IDirect3DTexture9()
{
    if (m_glTextureId) {
        glDeleteTextures(1, &m_glTextureId);
        m_glTextureId = 0;
    }
}

GLuint IDirect3DTexture9::GetGLTexture()
{
    if (!m_glTextureId && !m_data.empty()) {
        UploadToGL();
    }
    return m_glTextureId;
}

size_t IDirect3DTexture9::GetLevelDataSize(UINT level) const
{
    UINT mipW = std::max(1u, m_width >> level);
    UINT mipH = std::max(1u, m_height >> level);
    if (m_format == D3DFMT_DXT1) {
        return (size_t)std::max(1u, (mipW + 3) / 4) * std::max(1u, (mipH + 3) / 4) * 8;
    }
    if (m_format == D3DFMT_DXT3 || m_format == D3DFMT_DXT5) {
        return (size_t)std::max(1u, (mipW + 3) / 4) * std::max(1u, (mipH + 3) / 4) * 16;
    }
    if (m_format == D3DFMT_A4R4G4B4 || m_format == D3DFMT_A1R5G5B5 || m_format == D3DFMT_R5G6B5) {
        return (size_t)mipW * mipH * 2;
    }
    return (size_t)mipW * mipH * 4;
}

INT IDirect3DTexture9::GetPitch(UINT level) const
{
    UINT mipW = std::max(1u, m_width >> level);
    if (m_format == D3DFMT_DXT1) return (INT)(std::max(1u, (mipW + 3) / 4) * 8);
    if (m_format == D3DFMT_DXT3 || m_format == D3DFMT_DXT5) return (INT)(std::max(1u, (mipW + 3) / 4) * 16);
    if (m_format == D3DFMT_A4R4G4B4 || m_format == D3DFMT_A1R5G5B5 || m_format == D3DFMT_R5G6B5) return (INT)(mipW * 2);
    return (INT)(mipW * 4);
}

HRESULT IDirect3DTexture9::LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT*, DWORD)
{
    if (!pLockedRect) return E_INVALIDARG;
    if (Level == 0) {
        size_t sz = GetLevelDataSize(0);
        if (m_data.size() < sz) m_data.resize(sz, 0);
        pLockedRect->Pitch = GetPitch(0);
        pLockedRect->pBits = m_data.data();
    } else {
        size_t sz = GetLevelDataSize(Level);
        auto& mip = m_mipLevels[Level];
        if (mip.size() < sz) mip.resize(sz, 0);
        pLockedRect->Pitch = GetPitch(Level);
        pLockedRect->pBits = mip.data();
    }
    m_dirty = true;
    return S_OK;
}

HRESULT IDirect3DTexture9::UnlockRect(UINT)
{
    UploadToGL();
    return S_OK;
}

void IDirect3DTexture9::UploadLevelToGL(UINT level, const BYTE* pBytes, size_t dataSize)
{
    if (!pBytes || dataSize <= 0) return;
    UINT mipW = std::max(1u, m_width >> level);
    UINT mipH = std::max(1u, m_height >> level);

    if (m_format == D3DFMT_DXT1) {
        GLsizei size = (GLsizei)(std::max(1u, (mipW + 3) / 4) * std::max(1u, (mipH + 3) / 4) * 8);
        if (size > dataSize) size = dataSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mipW, mipH, 0, size, pBytes);
    } else if (m_format == D3DFMT_DXT3) {
        GLsizei size = (GLsizei)(std::max(1u, (mipW + 3) / 4) * std::max(1u, (mipH + 3) / 4) * 16);
        if (size > dataSize) size = dataSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, mipW, mipH, 0, size, pBytes);
    } else if (m_format == D3DFMT_DXT5) {
        GLsizei size = (GLsizei)(std::max(1u, (mipW + 3) / 4) * std::max(1u, (mipH + 3) / 4) * 16);
        if (size > dataSize) size = dataSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, mipW, mipH, 0, size, pBytes);
    } else if (m_format == D3DFMT_A4R4G4B4) {
        std::vector<uint32_t> rgba(mipW * mipH);
        const uint16_t* src = (const uint16_t*)pBytes;
        for (size_t i = 0; i < rgba.size(); ++i) {
            uint16_t val = src[i];
            uint8_t a = (val >> 12) & 0xF; a = (a << 4) | a;
            uint8_t r = (val >> 8) & 0xF;  r = (r << 4) | r;
            uint8_t g = (val >> 4) & 0xF;  g = (g << 4) | g;
            uint8_t b = val & 0xF;         b = (b << 4) | b;
            rgba[i] = (a << 24) | (b << 16) | (g << 8) | r;
        }
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, mipW, mipH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    } else if (m_format == D3DFMT_R5G6B5) {
        // 16 bit 5-6-5: must be expanded explicitly. Passing the 2-byte source
        // to a GL_BGRA 32-bit upload reads out of bounds and produces garbage
        // (this is the character shadow map format).
        std::vector<uint32_t> rgba(mipW * mipH);
        const uint16_t* src = (const uint16_t*)pBytes;
        for (size_t i = 0; i < rgba.size(); ++i) {
            uint16_t val = src[i];
            uint8_t r = (val >> 11) & 0x1F; r = (r << 3) | (r >> 2);
            uint8_t g = (val >> 5) & 0x3F;  g = (g << 2) | (g >> 4);
            uint8_t b = val & 0x1F;         b = (b << 3) | (b >> 2);
            rgba[i] = (0xFFu << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
        }
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, mipW, mipH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    } else if (m_format == D3DFMT_A1R5G5B5) {
        std::vector<uint32_t> rgba(mipW * mipH);
        const uint16_t* src = (const uint16_t*)pBytes;
        for (size_t i = 0; i < rgba.size(); ++i) {
            uint16_t val = src[i];
            uint8_t a = (val & 0x8000) ? 0xFF : 0x00;
            uint8_t r = (val >> 10) & 0x1F; r = (r << 3) | (r >> 2);
            uint8_t g = (val >> 5) & 0x1F;  g = (g << 3) | (g >> 2);
            uint8_t b = val & 0x1F;         b = (b << 3) | (b >> 2);
            rgba[i] = ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
        }
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, mipW, mipH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, mipW, mipH, 0, GL_BGRA, GL_UNSIGNED_BYTE, pBytes);
    }
}

void IDirect3DTexture9::Fill(const DWORD color)
{
    if (m_format == D3DFMT_DXT1 || m_format == D3DFMT_DXT3 || m_format == D3DFMT_DXT5)
        return;
    if (GetFormatBytesPerPixel(m_format) == 2) {
        WORD v = (WORD)(color & 0xFFFF);
        for (size_t i = 0; i + 1 < m_data.size(); i += 2) {
            m_data[i] = (BYTE)(v & 0xFF);
            m_data[i + 1] = (BYTE)(v >> 8);
        }
    } else {
        for (size_t i = 0; i + 3 < m_data.size(); i += 4) {
            m_data[i] = (BYTE)(color & 0xFF);
            m_data[i + 1] = (BYTE)((color >> 8) & 0xFF);
            m_data[i + 2] = (BYTE)((color >> 16) & 0xFF);
            m_data[i + 3] = (BYTE)((color >> 24) & 0xFF);
        }
    }
    m_dirty = true;
}

void IDirect3DTexture9::UploadToGL()
{
    if (m_data.empty()) return;
    if (!m_glTextureId) {
        glGenTextures(1, &m_glTextureId);
    }
    if (!m_glTextureId) return;

    glBindTexture(GL_TEXTURE_2D, m_glTextureId);

    // Upload Level 0
    UploadLevelToGL(0, m_data.data(), (GLsizei)m_data.size());

    // Upload any other mip levels
    UINT maxLevel = 0;
    for (const auto& kv : m_mipLevels) {
        UINT level = kv.first;
        if (level > 0 && !kv.second.empty()) {
            UploadLevelToGL(level, kv.second.data(), (GLsizei)kv.second.size());
            if (level > maxLevel) maxLevel = level;
        }
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);
    if (maxLevel > 0) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    m_dirty = false;
}

size_t IDirect3DTexture9::GetFormatBytesPerPixel(D3DFORMAT fmt)
{
    if (fmt == D3DFMT_DXT1) return 1;
    if (fmt == D3DFMT_DXT3 || fmt == D3DFMT_DXT5) return 1;
    if (fmt == D3DFMT_A4R4G4B4 || fmt == D3DFMT_A1R5G5B5 || fmt == D3DFMT_R5G6B5) return 2;
    return 4;
}

HRESULT IDirect3DTexture9::GetSurfaceLevel(UINT, IDirect3DSurface9** ppSurfaceLevel)
{
    if (!ppSurfaceLevel) return E_INVALIDARG;
    *ppSurfaceLevel = new IDirect3DSurface9(m_width, m_height, this);
    return S_OK;
}

// =============================================================
// IDirect3DSurface9 Implementation
// =============================================================

IDirect3DSurface9::IDirect3DSurface9(UINT w, UINT h, IDirect3DTexture9* pTexture,
                                     bool bDepthStencil, bool bDefault)
    : m_width(w ? w : 1), m_height(h ? h : 1),
      m_texture(pTexture), m_bDepthStencil(bDepthStencil), m_bDefault(bDefault),
      m_depthRenderbuffer(0)
{
    // Only plain CPU surfaces need backing storage (LockRect / GetDesc users).
    // Colour render-target surfaces and depth-stencil surfaces live on the GPU.
    if (!m_bDepthStencil && !m_texture)
        m_data.resize((size_t)m_width * m_height * 4, 0);
}

IDirect3DSurface9::~IDirect3DSurface9()
{
    if (m_depthRenderbuffer) {
        glDeleteRenderbuffers(1, &m_depthRenderbuffer);
        m_depthRenderbuffer = 0;
    }
}

GLuint IDirect3DSurface9::GetDepthRenderbuffer()
{
    if (!m_depthRenderbuffer) {
        glGenRenderbuffers(1, &m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_width, m_height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    return m_depthRenderbuffer;
}

// =============================================================
// IDirect3DDevice9 Implementation
// =============================================================

IDirect3DDevice9::IDirect3DDevice9()
    : m_pIndexBuffer(nullptr), m_fvf(D3DFVF_XYZ),
      m_srcBlend(D3DBLEND_SRCALPHA), m_destBlend(D3DBLEND_INVSRCALPHA),
      m_zEnable(false), m_zWriteEnable(true), m_zFunc(D3DCMP_LESSEQUAL),
      m_alphaBlendEnable(false), m_alphaTestEnable(false),
      m_alphaRef(0), m_alphaFunc(D3DCMP_ALWAYS),
      m_cullMode(D3DCULL_CCW), m_fillMode(D3DFILL_SOLID),
      m_textureFactor(0xFFFFFFFF), m_lighting(0), m_colorVertex(0), m_ambient(0),
      m_fogEnable(false), m_fogColor(0), m_fogVertexMode(D3DFOG_NONE), m_fogTableMode(D3DFOG_NONE),
      m_fogStart(0.0f), m_fogEnd(0.0f), m_fogDensity(1.0f), m_rangeFogEnable(false),
      m_inRenderTarget(false), m_fbo(0)
{
    std::memset(&m_viewport, 0, sizeof(m_viewport));
    m_viewport.Width = 1024;
    m_viewport.Height = 768;
    m_viewport.MaxZ = 1.0f;
    std::memset(&m_material, 0, sizeof(m_material));
    m_material.Diffuse.r = m_material.Diffuse.g = m_material.Diffuse.b = m_material.Diffuse.a = 1.0f;
    m_material.Ambient.r = m_material.Ambient.g = m_material.Ambient.b = m_material.Ambient.a = 1.0f;
    for (int i = 0; i < 8; ++i) {
        m_boundTextures[i] = nullptr;
        m_texTransformFlags[i] = D3DTTFF_DISABLE;
        m_texCoordIndex[i] = (DWORD)i;
        m_samplerStates[i].addressU = D3DTADDRESS_WRAP;
        m_samplerStates[i].addressV = D3DTADDRESS_WRAP;
        m_samplerStates[i].minFilter = D3DTEXF_LINEAR;
        m_samplerStates[i].magFilter = D3DTEXF_LINEAR;
        m_samplerStates[i].mipFilter = D3DTEXF_LINEAR;
        m_samplerStates[i].borderColor = 0;
        m_texStageStates[i].colorOp = (i == 0) ? D3DTOP_MODULATE : D3DTOP_DISABLE;
        m_texStageStates[i].colorArg0 = D3DTA_CURRENT;
        m_texStageStates[i].colorArg1 = D3DTA_TEXTURE;
        m_texStageStates[i].colorArg2 = D3DTA_CURRENT;
        m_texStageStates[i].alphaOp = (i == 0) ? D3DTOP_SELECTARG1 : D3DTOP_DISABLE;
        m_texStageStates[i].alphaArg0 = D3DTA_CURRENT;
        m_texStageStates[i].alphaArg1 = D3DTA_TEXTURE;
        m_texStageStates[i].alphaArg2 = D3DTA_CURRENT;
        m_texStageStates[i].resultArg = 0;
    }
    for (int i = 0; i < 512; ++i) D3DXMatrixIdentity(&m_transforms[i]);
}

IDirect3DDevice9::~IDirect3DDevice9()
{
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

void IDirect3DDevice9::EnsureFramebuffer()
{
    if (!m_fbo)
        glGenFramebuffers(1, &m_fbo);
}

// Direct3D applies a texture stage's colour operation even when no texture is
// bound to that stage (an unbound texture behaves as opaque white). The GL
// fixed-function pipeline instead disables texturing entirely, so the stage
// result has to be folded into the primary colour. This matters for the
// character shadow map pass, which draws untextured geometry with
// COLOROP = SELECTARG1 and COLORARG1 = TFACTOR (a flat grey silhouette).
bool IDirect3DDevice9::ComputeUntexturedStage0Color(const GLfloat* current, GLfloat* out) const
{
    if (m_boundTextures[0])
        return false;

    const TextureStageState& ss = m_texStageStates[0];
    if (ss.colorOp == D3DTOP_DISABLE)
        return false;

    auto readArg = [&](DWORD arg, GLfloat* v) {
        switch (arg & D3DTA_SELECTMASK) {
            case D3DTA_TFACTOR:
                v[0] = ((m_textureFactor >> 16) & 0xFF) / 255.0f;
                v[1] = ((m_textureFactor >> 8) & 0xFF) / 255.0f;
                v[2] = (m_textureFactor & 0xFF) / 255.0f;
                v[3] = ((m_textureFactor >> 24) & 0xFF) / 255.0f;
                break;
            case D3DTA_TEXTURE:
                // Unbound texture reads as opaque white.
                v[0] = v[1] = v[2] = v[3] = 1.0f;
                break;
            default:
                v[0] = current[0];
                v[1] = current[1];
                v[2] = current[2];
                v[3] = current[3];
                break;
        }
        if (arg & D3DTA_COMPLEMENT) {
            v[0] = 1.0f - v[0];
            v[1] = 1.0f - v[1];
            v[2] = 1.0f - v[2];
            v[3] = 1.0f - v[3];
        }
        if (arg & D3DTA_ALPHAREPLICATE) {
            v[0] = v[1] = v[2] = v[3];
        }
    };

    GLfloat a1[4], a2[4];
    readArg(ss.colorArg1, a1);
    readArg(ss.colorArg2, a2);

    switch (ss.colorOp) {
        case D3DTOP_SELECTARG1:
            for (int i = 0; i < 4; ++i) out[i] = a1[i];
            break;
        case D3DTOP_SELECTARG2:
            for (int i = 0; i < 4; ++i) out[i] = a2[i];
            break;
        case D3DTOP_ADD:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] + a2[i];
            break;
        case D3DTOP_ADDSIGNED:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] + a2[i] - 0.5f;
            break;
        case D3DTOP_SUBTRACT:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] - a2[i];
            break;
        case D3DTOP_MODULATE2X:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] * a2[i] * 2.0f;
            break;
        case D3DTOP_MODULATE4X:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] * a2[i] * 4.0f;
            break;
        case D3DTOP_BLENDTEXTUREALPHA:
            // Unbound texture alpha is 1: the result is ARG2.
            for (int i = 0; i < 4; ++i) out[i] = a2[i];
            break;
        case D3DTOP_BLENDDIFFUSEALPHA:
        case D3DTOP_BLENDCURRENTALPHA:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] + (a2[i] - a1[i]) * current[3];
            break;
        case D3DTOP_BLENDFACTORALPHA:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] + (a2[i] - a1[i]) * current[3];
            break;
        case D3DTOP_MODULATE:
        default:
            for (int i = 0; i < 4; ++i) out[i] = a1[i] * a2[i];
            break;
    }

    for (int i = 0; i < 4; ++i) {
        if (out[i] < 0.0f) out[i] = 0.0f;
        if (out[i] > 1.0f) out[i] = 1.0f;
    }
    return true;
}

HRESULT IDirect3DDevice9::SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
    if (RenderTargetIndex != 0)
        return S_OK;

    // Default framebuffer (window back buffer).
    if (!pRenderTarget || pRenderTarget->IsDefault()) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_inRenderTarget = false;
        return S_OK;
    }

    if (pRenderTarget->IsDepthStencil())
        return S_OK;

    IDirect3DTexture9* pTexture = pRenderTarget->GetTexture();
    if (!pTexture) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_inRenderTarget = false;
        return S_OK;
    }

    EnsureFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, pTexture->GetGLTexture(), 0);
    m_inRenderTarget = true;
    return S_OK;
}

HRESULT IDirect3DDevice9::SetDepthStencilSurface(IDirect3DSurface9* pZStencilSurface)
{
    // The default depth buffer is owned by the window framebuffer.
    if (!pZStencilSurface || pZStencilSurface->IsDefault())
        return S_OK;

    if (!pZStencilSurface->IsDepthStencil())
        return S_OK;

    EnsureFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                 GL_RENDERBUFFER, pZStencilSurface->GetDepthRenderbuffer());
    m_inRenderTarget = true;
    return S_OK;
}

HRESULT IDirect3DDevice9::Clear(DWORD /*Count*/, const RECT* /*pRects*/, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    GLbitfield mask = 0;
    if (Flags & D3DCLEAR_TARGET) {
        float a = ((Color >> 24) & 0xFF) / 255.0f;
        float r = ((Color >> 16) & 0xFF) / 255.0f;
        float g = ((Color >> 8) & 0xFF) / 255.0f;
        float b = (Color & 0xFF) / 255.0f;
        glClearColor(r, g, b, a);
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if (Flags & D3DCLEAR_ZBUFFER) {
        glDepthMask(GL_TRUE);
        glClearDepth(Z);
        mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (Flags & D3DCLEAR_STENCIL) {
        glClearStencil(Stencil);
        mask |= GL_STENCIL_BUFFER_BIT;
    }
    if (mask) {
        glClear(mask);
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::SetViewport(const D3DVIEWPORT9* pViewport)
{
    if (pViewport) {
        m_viewport = *pViewport;
        glViewport(m_viewport.X, m_viewport.Y, m_viewport.Width, m_viewport.Height);
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
    switch (State) {
        case D3DRS_ALPHABLENDENABLE:
            m_alphaBlendEnable = (Value != 0);
            break;
        case D3DRS_SRCBLEND:
            m_srcBlend = Value;
            break;
        case D3DRS_DESTBLEND:
            m_destBlend = Value;
            break;
        case D3DRS_ZENABLE:
            m_zEnable = (Value != 0);
            break;
        case D3DRS_ZWRITEENABLE:
            m_zWriteEnable = (Value != 0);
            break;
        case D3DRS_ZFUNC:
            m_zFunc = Value;
            break;
        case D3DRS_ALPHATESTENABLE:
            m_alphaTestEnable = (Value != 0);
            break;
        case D3DRS_ALPHAREF:
            m_alphaRef = Value;
            break;
        case D3DRS_ALPHAFUNC:
            m_alphaFunc = Value;
            break;
        case D3DRS_CULLMODE:
            m_cullMode = Value;
            break;
        case D3DRS_FILLMODE:
            m_fillMode = Value;
            break;
        case D3DRS_TEXTUREFACTOR:
            m_textureFactor = Value;
            break;
        case D3DRS_LIGHTING:
            m_lighting = Value;
            break;
        case D3DRS_COLORVERTEX:
            m_colorVertex = Value;
            break;
        case D3DRS_AMBIENT:
            m_ambient = Value;
            break;
        case D3DRS_FOGENABLE:
            m_fogEnable = (Value != 0);
            break;
        case D3DRS_FOGCOLOR:
            m_fogColor = Value;
            break;
        case D3DRS_FOGVERTEXMODE:
            m_fogVertexMode = Value;
            break;
        case D3DRS_FOGTABLEMODE:
            m_fogTableMode = Value;
            break;
        case D3DRS_FOGSTART:
            std::memcpy(&m_fogStart, &Value, sizeof(float));
            break;
        case D3DRS_FOGEND:
            std::memcpy(&m_fogEnd, &Value, sizeof(float));
            break;
        case D3DRS_FOGDENSITY:
            std::memcpy(&m_fogDensity, &Value, sizeof(float));
            break;
        case D3DRS_RANGEFOGENABLE:
            m_rangeFogEnable = (Value != 0);
            break;
        default:
            break;
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
{
    if (!pValue) return E_INVALIDARG;
    switch (State) {
        case D3DRS_ALPHABLENDENABLE: *pValue = m_alphaBlendEnable ? 1 : 0; break;
        case D3DRS_SRCBLEND: *pValue = m_srcBlend; break;
        case D3DRS_DESTBLEND: *pValue = m_destBlend; break;
        case D3DRS_ZENABLE: *pValue = m_zEnable ? 1 : 0; break;
        case D3DRS_ZWRITEENABLE: *pValue = m_zWriteEnable ? 1 : 0; break;
        case D3DRS_ZFUNC: *pValue = m_zFunc; break;
        case D3DRS_ALPHATESTENABLE: *pValue = m_alphaTestEnable ? 1 : 0; break;
        case D3DRS_ALPHAREF: *pValue = m_alphaRef; break;
        case D3DRS_ALPHAFUNC: *pValue = m_alphaFunc; break;
        case D3DRS_CULLMODE: *pValue = m_cullMode; break;
        case D3DRS_FILLMODE: *pValue = m_fillMode; break;
        case D3DRS_TEXTUREFACTOR: *pValue = m_textureFactor; break;
        case D3DRS_LIGHTING: *pValue = m_lighting; break;
        case D3DRS_COLORVERTEX: *pValue = m_colorVertex; break;
        case D3DRS_AMBIENT: *pValue = m_ambient; break;
        case D3DRS_FOGENABLE: *pValue = m_fogEnable ? 1 : 0; break;
        case D3DRS_FOGCOLOR: *pValue = m_fogColor; break;
        case D3DRS_FOGVERTEXMODE: *pValue = m_fogVertexMode; break;
        case D3DRS_FOGTABLEMODE: *pValue = m_fogTableMode; break;
        case D3DRS_FOGSTART: std::memcpy(pValue, &m_fogStart, sizeof(float)); break;
        case D3DRS_FOGEND: std::memcpy(pValue, &m_fogEnd, sizeof(float)); break;
        case D3DRS_FOGDENSITY: std::memcpy(pValue, &m_fogDensity, sizeof(float)); break;
        case D3DRS_RANGEFOGENABLE: *pValue = m_rangeFogEnable ? 1 : 0; break;
        default: *pValue = 0; break;
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    if (Stage < 8) {
        if (Type == D3DTSS_TEXTURETRANSFORMFLAGS) {
            m_texTransformFlags[Stage] = Value;
        } else if (Type == D3DTSS_TEXCOORDINDEX) {
            m_texCoordIndex[Stage] = Value;
        } else if (Type == D3DTSS_COLOROP) {
            m_texStageStates[Stage].colorOp = Value;
        } else if (Type == D3DTSS_COLORARG0) {
            m_texStageStates[Stage].colorArg0 = Value;
        } else if (Type == D3DTSS_COLORARG1) {
            m_texStageStates[Stage].colorArg1 = Value;
        } else if (Type == D3DTSS_COLORARG2) {
            m_texStageStates[Stage].colorArg2 = Value;
        } else if (Type == D3DTSS_ALPHAOP) {
            m_texStageStates[Stage].alphaOp = Value;
        } else if (Type == D3DTSS_ALPHAARG0) {
            m_texStageStates[Stage].alphaArg0 = Value;
        } else if (Type == D3DTSS_ALPHAARG1) {
            m_texStageStates[Stage].alphaArg1 = Value;
        } else if (Type == D3DTSS_ALPHAARG2) {
            m_texStageStates[Stage].alphaArg2 = Value;
        } else if (Type == D3DTSS_RESULTARG) {
            m_texStageStates[Stage].resultArg = Value;
        }
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    if (!pValue) return E_INVALIDARG;
    if (Stage < 8) {
        if (Type == D3DTSS_TEXTURETRANSFORMFLAGS) {
            *pValue = m_texTransformFlags[Stage];
            return S_OK;
        } else if (Type == D3DTSS_TEXCOORDINDEX) {
            *pValue = m_texCoordIndex[Stage];
            return S_OK;
        } else if (Type == D3DTSS_COLOROP) {
            *pValue = m_texStageStates[Stage].colorOp;
            return S_OK;
        } else if (Type == D3DTSS_COLORARG0) {
            *pValue = m_texStageStates[Stage].colorArg0;
            return S_OK;
        } else if (Type == D3DTSS_COLORARG1) {
            *pValue = m_texStageStates[Stage].colorArg1;
            return S_OK;
        } else if (Type == D3DTSS_COLORARG2) {
            *pValue = m_texStageStates[Stage].colorArg2;
            return S_OK;
        } else if (Type == D3DTSS_ALPHAOP) {
            *pValue = m_texStageStates[Stage].alphaOp;
            return S_OK;
        } else if (Type == D3DTSS_ALPHAARG0) {
            *pValue = m_texStageStates[Stage].alphaArg0;
            return S_OK;
        } else if (Type == D3DTSS_ALPHAARG1) {
            *pValue = m_texStageStates[Stage].alphaArg1;
            return S_OK;
        } else if (Type == D3DTSS_ALPHAARG2) {
            *pValue = m_texStageStates[Stage].alphaArg2;
            return S_OK;
        } else if (Type == D3DTSS_RESULTARG) {
            *pValue = m_texStageStates[Stage].resultArg;
            return S_OK;
        }
    }
    *pValue = 0;
    return S_OK;
}

HRESULT IDirect3DDevice9::SetSamplerState(DWORD Stage, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    if (Stage < 8) {
        if (Type == D3DSAMP_ADDRESSU) {
            m_samplerStates[Stage].addressU = Value;
        } else if (Type == D3DSAMP_ADDRESSV) {
            m_samplerStates[Stage].addressV = Value;
        } else if (Type == D3DSAMP_MAGFILTER) {
            m_samplerStates[Stage].magFilter = Value;
        } else if (Type == D3DSAMP_MINFILTER) {
            m_samplerStates[Stage].minFilter = Value;
        } else if (Type == D3DSAMP_MIPFILTER) {
            m_samplerStates[Stage].mipFilter = Value;
        } else if (Type == D3DSAMP_BORDERCOLOR) {
            m_samplerStates[Stage].borderColor = Value;
        }
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::GetSamplerState(DWORD Stage, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    if (!pValue) return E_INVALIDARG;
    if (Stage < 8) {
        if (Type == D3DSAMP_ADDRESSU) {
            *pValue = m_samplerStates[Stage].addressU;
            return S_OK;
        } else if (Type == D3DSAMP_ADDRESSV) {
            *pValue = m_samplerStates[Stage].addressV;
            return S_OK;
        } else if (Type == D3DSAMP_MAGFILTER) {
            *pValue = m_samplerStates[Stage].magFilter;
            return S_OK;
        } else if (Type == D3DSAMP_MINFILTER) {
            *pValue = m_samplerStates[Stage].minFilter;
            return S_OK;
        } else if (Type == D3DSAMP_MIPFILTER) {
            *pValue = m_samplerStates[Stage].mipFilter;
            return S_OK;
        } else if (Type == D3DSAMP_BORDERCOLOR) {
            *pValue = m_samplerStates[Stage].borderColor;
            return S_OK;
        }
    }
    *pValue = 0;
    return S_OK;
}

HRESULT IDirect3DDevice9::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    if (Stage < 8) {
        m_boundTextures[Stage] = static_cast<IDirect3DTexture9*>(pTexture);
        glActiveTexture(GL_TEXTURE0 + Stage);
        if (m_boundTextures[Stage] && m_boundTextures[Stage]->GetGLTexture()) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, m_boundTextures[Stage]->GetGLTexture());
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
        }
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
    if (ppTexture && Stage < 8) *ppTexture = m_boundTextures[Stage];
    return S_OK;
}

static inline GLenum ToGLBlend(DWORD d3dBlend)
{
    switch (d3dBlend) {
        case D3DBLEND_ZERO: return GL_ZERO;
        case D3DBLEND_ONE: return GL_ONE;
        case D3DBLEND_SRCCOLOR: return GL_SRC_COLOR;
        case D3DBLEND_INVSRCCOLOR: return GL_ONE_MINUS_SRC_COLOR;
        case D3DBLEND_SRCALPHA: return GL_SRC_ALPHA;
        case D3DBLEND_INVSRCALPHA: return GL_ONE_MINUS_SRC_ALPHA;
        case D3DBLEND_DESTALPHA: return GL_DST_ALPHA;
        case D3DBLEND_INVDESTALPHA: return GL_ONE_MINUS_DST_ALPHA;
        case D3DBLEND_DESTCOLOR: return GL_DST_COLOR;
        case D3DBLEND_INVDESTCOLOR: return GL_ONE_MINUS_DST_COLOR;
        default: return GL_ONE;
    }
}

static inline GLenum ToGLCmpFunc(DWORD d3dCmp)
{
    switch (d3dCmp) {
        case D3DCMP_NEVER: return GL_NEVER;
        case D3DCMP_LESS: return GL_LESS;
        case D3DCMP_EQUAL: return GL_EQUAL;
        case D3DCMP_LESSEQUAL: return GL_LEQUAL;
        case D3DCMP_GREATER: return GL_GREATER;
        case D3DCMP_NOTEQUAL: return GL_NOTEQUAL;
        case D3DCMP_GREATEREQUAL: return GL_GEQUAL;
        case D3DCMP_ALWAYS: return GL_ALWAYS;
        default: return GL_LEQUAL;
    }
}

static inline GLenum ToGLPrimitive(D3DPRIMITIVETYPE pt)
{
    switch (pt) {
        case D3DPT_POINTLIST: return GL_POINTS;
        case D3DPT_LINELIST: return GL_LINES;
        case D3DPT_LINESTRIP: return GL_LINE_STRIP;
        case D3DPT_TRIANGLELIST: return GL_TRIANGLES;
        case D3DPT_TRIANGLESTRIP: return GL_TRIANGLE_STRIP;
        case D3DPT_TRIANGLEFAN: return GL_TRIANGLE_FAN;
        default: return GL_TRIANGLES;
    }
}

static inline UINT PrimitiveCountToVertexCount(D3DPRIMITIVETYPE pt, UINT count)
{
    switch (pt) {
        case D3DPT_POINTLIST: return count;
        case D3DPT_LINELIST: return count * 2;
        case D3DPT_LINESTRIP: return count + 1;
        case D3DPT_TRIANGLELIST: return count * 3;
        case D3DPT_TRIANGLESTRIP: return count + 2;
        case D3DPT_TRIANGLEFAN: return count + 2;
        default: return count * 3;
    }
}

// ---------------------------------------------------------------------------
// Direct3D9 fixed-function texture stage emulation
// ---------------------------------------------------------------------------

static inline GLenum ToGLStageSource(DWORD arg)
{
    switch (arg & D3DTA_SELECTMASK) {
        case D3DTA_TEXTURE:  return GL_TEXTURE;
        case D3DTA_DIFFUSE:  return GL_PRIMARY_COLOR;
        case D3DTA_CURRENT:  return GL_PREVIOUS;
        case D3DTA_TFACTOR:  return GL_CONSTANT;
        case D3DTA_SPECULAR: return GL_PRIMARY_COLOR;
        case D3DTA_TEMP:     return GL_PREVIOUS;
        case D3DTA_CONSTANT: return GL_CONSTANT;
        default:             return GL_TEXTURE;
    }
}

static inline GLenum ToGLColorOperand(DWORD arg)
{
    if (arg & D3DTA_ALPHAREPLICATE) return GL_SRC_ALPHA;
    if (arg & D3DTA_COMPLEMENT)     return GL_ONE_MINUS_SRC_COLOR;
    return GL_SRC_COLOR;
}

static inline GLenum ToGLAlphaOperand(DWORD arg)
{
    if (arg & D3DTA_COMPLEMENT) return GL_ONE_MINUS_SRC_ALPHA;
    return GL_SRC_ALPHA;
}

static void SetStageColorCombine(DWORD op, DWORD arg1, DWORD arg2)
{
    const GLenum s0 = ToGLStageSource(arg1);
    const GLenum s1 = ToGLStageSource(arg2);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, ToGLColorOperand(arg1));
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, ToGLColorOperand(arg2));
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, 1);
    switch (op) {
        case D3DTOP_DISABLE:
            // Color passes through from the previous stage.
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
            break;
        case D3DTOP_SELECTARG1:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            break;
        case D3DTOP_SELECTARG2:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s1);
            break;
        case D3DTOP_ADD:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            break;
        case D3DTOP_ADDSIGNED:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD_SIGNED);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            break;
        case D3DTOP_SUBTRACT:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_SUBTRACT);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            break;
        case D3DTOP_MODULATE2X:
        case D3DTOP_MODULATE4X:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            glTexEnvi(GL_TEXTURE_ENV, GL_RGB_SCALE, (op == D3DTOP_MODULATE2X) ? 2 : 4);
            break;
        case D3DTOP_BLENDTEXTUREALPHA:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_TEXTURE);
            break;
        case D3DTOP_BLENDCURRENTALPHA:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_PREVIOUS);
            break;
        case D3DTOP_BLENDFACTORALPHA:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_CONSTANT);
            break;
        case D3DTOP_MODULATEALPHA_ADDCOLOR:
        case D3DTOP_MODULATECOLOR_ADDALPHA:
        case D3DTOP_MODULATEINVALPHA_ADDCOLOR:
        case D3DTOP_MODULATEINVCOLOR_ADDALPHA:
            // Approximation of the specular "add" stages: current + texture.
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            break;
        default:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, s1);
            break;
    }
}

static void SetStageAlphaCombine(DWORD op, DWORD arg1, DWORD arg2)
{
    const GLenum s0 = ToGLStageSource(arg1);
    const GLenum s1 = ToGLStageSource(arg2);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, ToGLAlphaOperand(arg1));
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, ToGLAlphaOperand(arg2));
    glTexEnvi(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1);

    switch (op) {
        case D3DTOP_DISABLE:
            // Alpha is passed through unchanged.
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
            break;
        case D3DTOP_SELECTARG1:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, s0);
            break;
        case D3DTOP_SELECTARG2:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, s1);
            break;
        case D3DTOP_ADD:
        case D3DTOP_ADDSIGNED:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, s1);
            break;
        case D3DTOP_SUBTRACT:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_SUBTRACT);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, s1);
            break;
        default:
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, s0);
            glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, s1);
            break;
    }
}

// Emulates D3DTSS_TCI_CAMERASPACEPOSITION with GL_EYE_LINEAR.
// The D3D texture matrix is baked into the eye plane equations so that the
// generated (s,t) coordinates are identical to the D3D ones, including the
// eye-space Z and translation terms (which are essential for stable
// world-space texture coordinates on the terrain).
static void ApplyCameraSpacePositionTexGen(const D3DXMATRIX* pTexMatrix, bool bProjected)
{
    GLfloat s[4], t[4], r[4], q[4];
    if (pTexMatrix) {
        s[0] = pTexMatrix->_11; s[1] = pTexMatrix->_21; s[2] = pTexMatrix->_31; s[3] = pTexMatrix->_41;
        t[0] = pTexMatrix->_12; t[1] = pTexMatrix->_22; t[2] = pTexMatrix->_32; t[3] = pTexMatrix->_42;
        r[0] = pTexMatrix->_13; r[1] = pTexMatrix->_23; r[2] = pTexMatrix->_33; r[3] = pTexMatrix->_43;
        q[0] = pTexMatrix->_14; q[1] = pTexMatrix->_24; q[2] = pTexMatrix->_34; q[3] = pTexMatrix->_44;
    } else {
        s[0] = 1.0f; s[1] = 0.0f; s[2] = 0.0f; s[3] = 0.0f;
        t[0] = 0.0f; t[1] = 1.0f; t[2] = 0.0f; t[3] = 0.0f;
        r[0] = 0.0f; r[1] = 0.0f; r[2] = 1.0f; r[3] = 0.0f;
        q[0] = 0.0f; q[1] = 0.0f; q[2] = 0.0f; q[3] = 1.0f;
    }

    // GL transforms the supplied eye plane by the inverse of the modelview
    // matrix. Specify the planes with an identity modelview so they are stored
    // verbatim in eye space.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGenfv(GL_S, GL_EYE_PLANE, s);
    glTexGenfv(GL_T, GL_EYE_PLANE, t);

    if (bProjected) {
        glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
        glTexGenfv(GL_R, GL_EYE_PLANE, r);
        glTexGenfv(GL_Q, GL_EYE_PLANE, q);
        glEnable(GL_TEXTURE_GEN_R);
        glEnable(GL_TEXTURE_GEN_Q);
    } else {
        glDisable(GL_TEXTURE_GEN_R);
        glDisable(GL_TEXTURE_GEN_Q);
    }

    glPopMatrix();

    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
}

void IDirect3DDevice9::SetupGLState()
{
    // 1. Matrix Setup
    // When rendering into an FBO we flip the clip-space Y axis. Direct3D render
    // targets store row 0 at the top, while OpenGL stores the bottom row first;
    // flipping keeps the resulting texture sampleable with the client's D3D
    // texture matrices (character shadow map, snow blur, ...).
    const bool flipY = m_inRenderTarget;
    // Negate the projection's Y output row (S * M, including translation) so
    // that D3D's top row lands at texture row 0.
    auto applyClipYFlip = [flipY]() {
        if (!flipY) return;
        GLfloat proj[16];
        glGetFloatv(GL_PROJECTION_MATRIX, proj);
        proj[1] = -proj[1];
        proj[5] = -proj[5];
        proj[9] = -proj[9];
        proj[13] = -proj[13];
        glLoadMatrixf(proj);
    };
    DWORD posType = m_fvf & D3DFVF_POSITION_MASK;
    bool isPerspective = false;
    if (posType == D3DFVF_XYZRHW) {
        // Pre-transformed 2D screen vertices
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, (double)m_viewport.Width, (double)m_viewport.Height, 0.0, -1000.0, 1000.0);
        applyClipYFlip();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    } else {
        // Direct3D 3D perspective projections have _44 == 0.0f (perspective divide).
        // The client's 2D interface projection (SetOrtho2D) is an off-centre
        // orthographic matrix with _41/_42 == -1/+1; it needs the half-pixel
        // screen-space replacement. Centred orthographic projections (shadow
        // map, SetOrtho3D) must use the matrix supplied by the client.
        const D3DMATRIX& proj = m_transforms[D3DTS_PROJECTION];
        isPerspective = (proj._44 == 0.0f);
        const bool isScreenOrtho = !isPerspective && (proj._41 != 0.0f || proj._42 != 0.0f);
        glMatrixMode(GL_PROJECTION);
        if (isPerspective || !isScreenOrtho) {
            glLoadMatrixf((const GLfloat*)&proj);
        } else {
            // 2D Orthographic Pipeline (UI with half-pixel texel offset)
            glLoadIdentity();
            glOrtho(-0.5, (double)m_viewport.Width - 0.5, (double)m_viewport.Height - 0.5, -0.5, -1000.0, 1000.0);
        }
        applyClipYFlip();
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf((const GLfloat*)&m_transforms[D3DTS_VIEW]);
        glMultMatrixf((const GLfloat*)&m_transforms[D3DTS_WORLD]);
    }

    // 2. Depth Test & Write
    if (m_zEnable) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(ToGLCmpFunc(m_zFunc));
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(m_zWriteEnable ? GL_TRUE : GL_FALSE);

    // 3. Alpha Blending
    if (m_alphaBlendEnable) {
        glEnable(GL_BLEND);
        glBlendFunc(ToGLBlend(m_srcBlend), ToGLBlend(m_destBlend));
    } else {
        glDisable(GL_BLEND);
    }

    // 4. Alpha Testing
    if (m_alphaTestEnable) {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(ToGLCmpFunc(m_alphaFunc), (GLclampf)(m_alphaRef / 255.0f));
    } else {
        glDisable(GL_ALPHA_TEST);
    }

    // 5. Fog (D3D9 vertex/table fog emulation)
    // The client enables fog per environment (CMapManager::BeginEnvironment) and
    // temporarily switches the fog colour to white while drawing multiplicative
    // shadow passes. GL's fixed-function fog uses the eye-space depth as the fog
    // coordinate, which matches D3D's non-range vertex fog closely enough.
    const DWORD fogMode = (m_fogTableMode != D3DFOG_NONE) ? m_fogTableMode : m_fogVertexMode;
    if (m_fogEnable && isPerspective && fogMode != D3DFOG_NONE &&
        (fogMode != D3DFOG_LINEAR || m_fogEnd > m_fogStart)) {
        glEnable(GL_FOG);
        const GLfloat fogColor[4] = {
            ((m_fogColor >> 16) & 0xFF) / 255.0f,
            ((m_fogColor >> 8) & 0xFF) / 255.0f,
            (m_fogColor & 0xFF) / 255.0f,
            ((m_fogColor >> 24) & 0xFF) / 255.0f
        };
        glFogfv(GL_FOG_COLOR, fogColor);
        switch (fogMode) {
            case D3DFOG_LINEAR:
                glFogi(GL_FOG_MODE, GL_LINEAR);
                glFogf(GL_FOG_START, m_fogStart);
                glFogf(GL_FOG_END, m_fogEnd);
                break;
            case D3DFOG_EXP:
                glFogi(GL_FOG_MODE, GL_EXP);
                glFogf(GL_FOG_DENSITY, m_fogDensity);
                break;
            case D3DFOG_EXP2:
                glFogi(GL_FOG_MODE, GL_EXP2);
                glFogf(GL_FOG_DENSITY, m_fogDensity);
                break;
            default:
                break;
        }
        glHint(GL_FOG_HINT, GL_NICEST);
    } else {
        glDisable(GL_FOG);
    }

    // 6. Culling
    if (m_cullMode == D3DCULL_NONE) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        // D3D's front face is clockwise in screen space. The clip-space Y flip
        // used for render targets reverses the window-space winding.
        glFrontFace(flipY ? GL_CCW : GL_CW);
        glCullFace((m_cullMode == D3DCULL_CW) ? GL_FRONT : GL_BACK);
    }

    // 7. Fill Mode
    if (m_fillMode == D3DFILL_WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else if (m_fillMode == D3DFILL_POINT) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // 8. Textures & Transforms
    const UINT numTex = (m_fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    const GLfloat texFactor[4] = {
        ((m_textureFactor >> 16) & 0xFF) / 255.0f,
        ((m_textureFactor >> 8) & 0xFF) / 255.0f,
        (m_textureFactor & 0xFF) / 255.0f,
        ((m_textureFactor >> 24) & 0xFF) / 255.0f
    };

    for (int stage = 0; stage < 2; ++stage) {
        glActiveTexture(GL_TEXTURE0 + stage);
        glDisable(GL_TEXTURE_GEN_R);
        glDisable(GL_TEXTURE_GEN_Q);

        const TextureStageState& ss = m_texStageStates[stage];

        // If this texture stage is disabled or no texture is bound, disable it
        if ((ss.colorOp == D3DTOP_DISABLE && ss.alphaOp == D3DTOP_DISABLE) ||
            !m_boundTextures[stage] || !m_boundTextures[stage]->GetGLTexture()) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            continue;
        }

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, m_boundTextures[stage]->GetGLTexture());

        // Texture factor (D3DTA_TFACTOR maps to GL_CONSTANT)
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, texFactor);

        // Fixed-function combine emulation (color + alpha)
        SetStageColorCombine(ss.colorOp, ss.colorArg1, ss.colorArg2);
        SetStageAlphaCombine(ss.alphaOp, ss.alphaArg1, ss.alphaArg2);

        // Apply texture sampler states
        bool hasMips = m_boundTextures[stage]->HasMipMaps();
        if (m_samplerStates[stage].addressU == D3DTADDRESS_CLAMP) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        } else if (m_samplerStates[stage].addressU == D3DTADDRESS_BORDER) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        }
        if (m_samplerStates[stage].addressV == D3DTADDRESS_CLAMP) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else if (m_samplerStates[stage].addressV == D3DTADDRESS_BORDER) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        if (m_samplerStates[stage].addressU == D3DTADDRESS_BORDER ||
            m_samplerStates[stage].addressV == D3DTADDRESS_BORDER) {
            const DWORD c = m_samplerStates[stage].borderColor;
            const GLfloat border[4] = {
                ((c >> 16) & 0xFF) / 255.0f,
                ((c >> 8) & 0xFF) / 255.0f,
                (c & 0xFF) / 255.0f,
                ((c >> 24) & 0xFF) / 255.0f
            };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        }

        GLint mag = (m_samplerStates[stage].magFilter == D3DTEXF_POINT) ? GL_NEAREST : GL_LINEAR;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);

        if (hasMips) {
            GLint min = (m_samplerStates[stage].minFilter == D3DTEXF_POINT) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
        } else {
            GLint min = (m_samplerStates[stage].minFilter == D3DTEXF_POINT) ? GL_NEAREST : GL_LINEAR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
        }

        // Texture coordinate generation / texture matrix.
        // In Direct3D, a cameraspace-position TCI produces the full camera-space
        // position (x, y, z, 1), which is then multiplied by the texture matrix.
        // Baking that matrix into GL_EYE_LINEAR planes reproduces it exactly.
        const DWORD tci = m_texCoordIndex[stage];
        const DWORD tciMode = tci & 0xFFFF0000u;
        const bool hasTransform = (m_texTransformFlags[stage] & ~D3DTTFF_PROJECTED) != D3DTTFF_DISABLE;
        const bool isProjected = (m_texTransformFlags[stage] & D3DTTFF_PROJECTED) != 0;
        const D3DXMATRIX* pTexMatrix = hasTransform ? &m_transforms[D3DTS_TEXTURE0 + stage] : nullptr;

        if (tciMode == D3DTSS_TCI_CAMERASPACEPOSITION) {
            ApplyCameraSpacePositionTexGen(pTexMatrix, isProjected);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
        } else if (tciMode == D3DTSS_TCI_PASSTHRU && stage >= (int)numTex) {
            // No per-vertex texture coordinates for this stage: preserve the
            // legacy eye-linear behaviour applied through the texture matrix.
            ApplyCameraSpacePositionTexGen(nullptr, false);
            glMatrixMode(GL_TEXTURE);
            if (pTexMatrix) glLoadMatrixf((const GLfloat*)pTexMatrix);
            else glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
        } else {
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glMatrixMode(GL_TEXTURE);
            if (pTexMatrix) glLoadMatrixf((const GLfloat*)pTexMatrix);
            else glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

static inline void SetupVertexArrays(const BYTE* vbData, UINT stride, DWORD fvf, const GLfloat* primaryColor)
{
    DWORD posType = fvf & D3DFVF_POSITION_MASK;
    size_t offset = 0;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, vbData + offset);
    if (posType == D3DFVF_XYZRHW) {
        offset += 16;
    } else {
        offset += 12;
    }

    if (fvf & D3DFVF_NORMAL) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, stride, vbData + offset);
        offset += 12;
    } else {
        glDisableClientState(GL_NORMAL_ARRAY);
    }

    if (fvf & D3DFVF_PSIZE) {
        offset += 4;
    }

    if (fvf & D3DFVF_DIFFUSE) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, vbData + offset);
        offset += 4;
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
        glColor4fv(primaryColor);
    }

    if (fvf & D3DFVF_SPECULAR) {
        offset += 4;
    }

    UINT numTex = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    if (numTex >= 1) {
        glClientActiveTexture(GL_TEXTURE0);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, vbData + offset);
        offset += 8;
    } else {
        glClientActiveTexture(GL_TEXTURE0);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (numTex >= 2) {
        glClientActiveTexture(GL_TEXTURE1);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, vbData + offset);
        offset += 8;
    } else {
        glClientActiveTexture(GL_TEXTURE1);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glClientActiveTexture(GL_TEXTURE0);
}

static inline void TeardownVertexArrays(DWORD fvf)
{
    glDisableClientState(GL_VERTEX_ARRAY);
    if (fvf & D3DFVF_NORMAL) glDisableClientState(GL_NORMAL_ARRAY);
    if (fvf & D3DFVF_DIFFUSE) glDisableClientState(GL_COLOR_ARRAY);
    UINT numTex = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    if (numTex >= 1) {
        glClientActiveTexture(GL_TEXTURE0);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    if (numTex >= 2) {
        glClientActiveTexture(GL_TEXTURE1);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glClientActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
}

HRESULT IDirect3DDevice9::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    if (!m_streams[0].vb || !m_streams[0].vb->GetData()) return S_OK;

    UINT stride = m_streams[0].stride ? m_streams[0].stride : D3DXGetFVFVertexSize(m_fvf);
    if (!stride) stride = 24;
    const BYTE* vbData = (const BYTE*)m_streams[0].vb->GetData() + m_streams[0].offset;

    GLfloat primaryColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (m_lighting && !(m_fvf & D3DFVF_DIFFUSE)) {
        primaryColor[0] = m_material.Diffuse.r;
        primaryColor[1] = m_material.Diffuse.g;
        primaryColor[2] = m_material.Diffuse.b;
        primaryColor[3] = m_material.Diffuse.a;
    }
    if (!(m_fvf & D3DFVF_DIFFUSE)) {
        GLfloat stageColor[4];
        if (ComputeUntexturedStage0Color(primaryColor, stageColor)) {
            for (int i = 0; i < 4; ++i)
                primaryColor[i] = stageColor[i];
        }
    }
    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf, primaryColor);

    GLenum glMode = ToGLPrimitive(PrimitiveType);
    UINT vtxCount = PrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount);

    glDrawArrays(glMode, StartVertex, vtxCount);

    TeardownVertexArrays(m_fvf);
    return S_OK;
}

HRESULT IDirect3DDevice9::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT /*MinIndex*/, UINT /*NumVertices*/, UINT StartIndex, UINT PrimitiveCount)
{
    if (!m_streams[0].vb || !m_pIndexBuffer) return S_OK;
    if (!m_streams[0].vb->GetData() || !m_pIndexBuffer->GetData()) return S_OK;

    UINT stride = m_streams[0].stride ? m_streams[0].stride : D3DXGetFVFVertexSize(m_fvf);
    if (!stride) stride = 24;
    const BYTE* vbData = (const BYTE*)m_streams[0].vb->GetData() + m_streams[0].offset + (BaseVertexIndex * stride);
    const WORD* ibData = (const WORD*)m_pIndexBuffer->GetData() + StartIndex;

    GLfloat primaryColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (m_lighting && !(m_fvf & D3DFVF_DIFFUSE)) {
        primaryColor[0] = m_material.Diffuse.r;
        primaryColor[1] = m_material.Diffuse.g;
        primaryColor[2] = m_material.Diffuse.b;
        primaryColor[3] = m_material.Diffuse.a;
    }
    if (!(m_fvf & D3DFVF_DIFFUSE)) {
        GLfloat stageColor[4];
        if (ComputeUntexturedStage0Color(primaryColor, stageColor)) {
            for (int i = 0; i < 4; ++i)
                primaryColor[i] = stageColor[i];
        }
    }
    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf, primaryColor);

    GLenum glMode = ToGLPrimitive(PrimitiveType);
    UINT indexCount = PrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount);

    glDrawElements(glMode, indexCount, GL_UNSIGNED_SHORT, ibData);

    TeardownVertexArrays(m_fvf);
    return S_OK;
}

HRESULT IDirect3DDevice9::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    if (!pVertexStreamZeroData) return S_OK;

    UINT stride = VertexStreamZeroStride ? VertexStreamZeroStride : D3DXGetFVFVertexSize(m_fvf);
    if (!stride) stride = 24;
    const BYTE* vbData = (const BYTE*)pVertexStreamZeroData;

    GLfloat primaryColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (m_lighting && !(m_fvf & D3DFVF_DIFFUSE)) {
        primaryColor[0] = m_material.Diffuse.r;
        primaryColor[1] = m_material.Diffuse.g;
        primaryColor[2] = m_material.Diffuse.b;
        primaryColor[3] = m_material.Diffuse.a;
    }
    if (!(m_fvf & D3DFVF_DIFFUSE)) {
        GLfloat stageColor[4];
        if (ComputeUntexturedStage0Color(primaryColor, stageColor)) {
            for (int i = 0; i < 4; ++i)
                primaryColor[i] = stageColor[i];
        }
    }
    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf, primaryColor);

    GLenum glMode = ToGLPrimitive(PrimitiveType);
    UINT vtxCount = PrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount);

    glDrawArrays(glMode, 0, vtxCount);

    TeardownVertexArrays(m_fvf);
    return S_OK;
}

HRESULT IDirect3DDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT /*MinVertexIndex*/, UINT /*NumVertexIndices*/, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    if (!pVertexStreamZeroData || !pIndexData) return S_OK;

    UINT stride = VertexStreamZeroStride ? VertexStreamZeroStride : D3DXGetFVFVertexSize(m_fvf);
    if (!stride) stride = 24;
    const BYTE* vbData = (const BYTE*)pVertexStreamZeroData;

    GLfloat primaryColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    if (m_lighting && !(m_fvf & D3DFVF_DIFFUSE)) {
        primaryColor[0] = m_material.Diffuse.r;
        primaryColor[1] = m_material.Diffuse.g;
        primaryColor[2] = m_material.Diffuse.b;
        primaryColor[3] = m_material.Diffuse.a;
    }
    if (!(m_fvf & D3DFVF_DIFFUSE)) {
        GLfloat stageColor[4];
        if (ComputeUntexturedStage0Color(primaryColor, stageColor)) {
            for (int i = 0; i < 4; ++i)
                primaryColor[i] = stageColor[i];
        }
    }
    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf, primaryColor);

    GLenum glMode = ToGLPrimitive(PrimitiveType);
    UINT indexCount = PrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount);

    GLenum idxType = (IndexDataFormat == D3DFMT_INDEX32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    glDrawElements(glMode, indexCount, idxType, pIndexData);

    TeardownVertexArrays(m_fvf);
    return S_OK;
}

#endif // OPENMT2_PLATFORM_LINUX
