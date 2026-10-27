#include <Platform/DirectXMathCompat.h>

// Force link ImageLoaderCompat.cpp from static libEterBase.a
extern bool Platform_GetImageInfo(const void* data, size_t size, uint32_t* outW, uint32_t* outH, uint32_t* outFormat);
extern bool Platform_DecodeImage(const void* data, size_t size, uint32_t* outW, uint32_t* outH, std::vector<uint8_t>& outPixels);
void* g_pfnForceLinkImageLoader[] = {
    (void*)&Platform_GetImageInfo,
    (void*)&Platform_DecodeImage
};

#if defined(__LINUX__)
#include <GL/gl.h>
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
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
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
    } else {
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, mipW, mipH, 0, GL_BGRA, GL_UNSIGNED_BYTE, pBytes);
    }
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
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
    *ppSurfaceLevel = new IDirect3DSurface9(m_width, m_height);
    return S_OK;
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
      m_cullMode(D3DCULL_CCW), m_fillMode(D3DFILL_SOLID)
{
    std::memset(&m_viewport, 0, sizeof(m_viewport));
    m_viewport.Width = 1024;
    m_viewport.Height = 768;
    m_viewport.MaxZ = 1.0f;
    for (int i = 0; i < 8; ++i) {
        m_boundTextures[i] = nullptr;
        m_texTransformFlags[i] = D3DTTFF_DISABLE;
        m_samplerStates[i].addressU = D3DTADDRESS_WRAP;
        m_samplerStates[i].addressV = D3DTADDRESS_WRAP;
        m_samplerStates[i].minFilter = D3DTEXF_LINEAR;
        m_samplerStates[i].magFilter = D3DTEXF_LINEAR;
        m_samplerStates[i].mipFilter = D3DTEXF_LINEAR;
    }
    for (int i = 0; i < 512; ++i) D3DXMatrixIdentity(&m_transforms[i]);
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
        default: *pValue = 0; break;
    }
    return S_OK;
}

HRESULT IDirect3DDevice9::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    if (Stage < 8) {
        if (Type == D3DTSS_TEXTURETRANSFORMFLAGS) {
            m_texTransformFlags[Stage] = Value;
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

void IDirect3DDevice9::SetupGLState()
{
    // 1. Matrix Setup
    DWORD posType = m_fvf & D3DFVF_POSITION_MASK;
    if (posType == D3DFVF_XYZRHW) {
        // Pre-transformed 2D screen vertices
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, (double)m_viewport.Width, (double)m_viewport.Height, 0.0, -1000.0, 1000.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    } else {
        // Direct3D 3D Perspective projection has _44 == 0.0f (due to perspective divide)
        // Orthographic (UI) has _44 == 1.0f
        bool isPerspective = (m_transforms[D3DTS_PROJECTION]._44 == 0.0f);
        if (isPerspective) {
            // 3D Perspective Pipeline
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf((const GLfloat*)&m_transforms[D3DTS_PROJECTION]);
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf((const GLfloat*)&m_transforms[D3DTS_VIEW]);
            glMultMatrixf((const GLfloat*)&m_transforms[D3DTS_WORLD]);
        } else {
            // 2D Orthographic Pipeline (UI with half-pixel texel offset)
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(-0.5, (double)m_viewport.Width - 0.5, (double)m_viewport.Height - 0.5, -0.5, -1000.0, 1000.0);
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf((const GLfloat*)&m_transforms[D3DTS_VIEW]);
            glMultMatrixf((const GLfloat*)&m_transforms[D3DTS_WORLD]);
        }
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

    // 5. Culling
    if (m_cullMode == D3DCULL_NONE) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CW); // D3D standard front-face is clockwise
        glCullFace((m_cullMode == D3DCULL_CW) ? GL_FRONT : GL_BACK);
    }

    // 6. Fill Mode
    if (m_fillMode == D3DFILL_WIREFRAME) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else if (m_fillMode == D3DFILL_POINT) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // 7. Textures & Transforms
    UINT numTex = (m_fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (int stage = 0; stage < 2; ++stage) {
        glActiveTexture(GL_TEXTURE0 + stage);
        if (m_boundTextures[stage] && m_boundTextures[stage]->GetGLTexture()) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, m_boundTextures[stage]->GetGLTexture());
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

            // Apply texture sampler states
            bool hasMips = m_boundTextures[stage]->HasMipMaps();
            if (m_samplerStates[stage].addressU == D3DTADDRESS_CLAMP) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            } else if (m_samplerStates[stage].addressU == D3DTADDRESS_WRAP && hasMips) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            }
            if (m_samplerStates[stage].addressV == D3DTADDRESS_CLAMP) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            } else if (m_samplerStates[stage].addressV == D3DTADDRESS_WRAP && hasMips) {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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

            // Apply texture stage transformation if enabled
            glMatrixMode(GL_TEXTURE);
            if ((m_texTransformFlags[stage] & ~D3DTTFF_PROJECTED) != D3DTTFF_DISABLE) {
                glLoadMatrixf((const GLfloat*)&m_transforms[D3DTS_TEXTURE0 + stage]);
            } else {
                glLoadIdentity();
            }
            glMatrixMode(GL_MODELVIEW);

            if (stage >= (int)numTex) {
                // If texture is bound but vertex has no texcoords for this stage (e.g. terrain splat, minimap circular mask), enable eye-linear texgen
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);
            } else {
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
            }
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
        }
    }
    glActiveTexture(GL_TEXTURE0);
}

static inline void SetupVertexArrays(const BYTE* vbData, UINT stride, DWORD fvf)
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
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
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

    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf);

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

    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf);

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

    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf);

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

    SetupGLState();
    SetupVertexArrays(vbData, stride, m_fvf);

    GLenum glMode = ToGLPrimitive(PrimitiveType);
    UINT indexCount = PrimitiveCountToVertexCount(PrimitiveType, PrimitiveCount);

    GLenum idxType = (IndexDataFormat == D3DFMT_INDEX32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    glDrawElements(glMode, indexCount, idxType, pIndexData);

    TeardownVertexArrays(m_fvf);
    return S_OK;
}

#endif // __LINUX__
