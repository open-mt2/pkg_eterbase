#include <Platform/DirectXMathCompat.h>

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

IDirect3DTexture9::IDirect3DTexture9(UINT w, UINT h, D3DFORMAT fmt)
    : m_width(w ? w : 1), m_height(h ? h : 1), m_format(fmt), m_glTextureId(0), m_dirty(false)
{
    size_t bpp = GetFormatBytesPerPixel(fmt);
    m_data.resize(m_width * m_height * (bpp ? bpp : 4), 0);
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

HRESULT IDirect3DTexture9::LockRect(UINT, D3DLOCKED_RECT* pLockedRect, const RECT*, DWORD)
{
    if (!pLockedRect) return E_INVALIDARG;
    pLockedRect->Pitch = GetPitch();
    pLockedRect->pBits = m_data.data();
    m_dirty = true;
    return S_OK;
}

HRESULT IDirect3DTexture9::UnlockRect(UINT)
{
    UploadToGL();
    return S_OK;
}

void IDirect3DTexture9::UploadToGL()
{
    if (m_data.empty()) return;
    if (!m_glTextureId) {
        glGenTextures(1, &m_glTextureId);
    }
    if (!m_glTextureId) return;

    glBindTexture(GL_TEXTURE_2D, m_glTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (m_format == D3DFMT_DXT1) {
        GLsizei size = (GLsizei)(std::max(1u, (m_width + 3) / 4) * std::max(1u, (m_height + 3) / 4) * 8);
        if (size > (GLsizei)m_data.size()) size = (GLsizei)m_data.size();
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, m_width, m_height, 0, size, m_data.data());
    } else if (m_format == D3DFMT_DXT3) {
        GLsizei size = (GLsizei)(std::max(1u, (m_width + 3) / 4) * std::max(1u, (m_height + 3) / 4) * 16);
        if (size > (GLsizei)m_data.size()) size = (GLsizei)m_data.size();
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, m_width, m_height, 0, size, m_data.data());
    } else if (m_format == D3DFMT_DXT5) {
        GLsizei size = (GLsizei)(std::max(1u, (m_width + 3) / 4) * std::max(1u, (m_height + 3) / 4) * 16);
        if (size > (GLsizei)m_data.size()) size = (GLsizei)m_data.size();
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, m_width, m_height, 0, size, m_data.data());
    } else if (m_format == D3DFMT_A4R4G4B4) {
        std::vector<uint32_t> rgba(m_width * m_height);
        const uint16_t* src = (const uint16_t*)m_data.data();
        for (size_t i = 0; i < rgba.size(); ++i) {
            uint16_t val = src[i];
            uint8_t a = (val >> 12) & 0xF; a = (a << 4) | a;
            uint8_t r = (val >> 8) & 0xF;  r = (r << 4) | r;
            uint8_t g = (val >> 4) & 0xF;  g = (g << 4) | g;
            uint8_t b = val & 0xF;         b = (b << 4) | b;
            rgba[i] = (a << 24) | (b << 16) | (g << 8) | r;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, m_data.data());
    }
    m_dirty = false;
}

INT IDirect3DTexture9::GetPitch() const
{
    if (m_format == D3DFMT_DXT1) return (INT)(std::max(1u, (m_width + 3) / 4) * 8);
    if (m_format == D3DFMT_DXT3 || m_format == D3DFMT_DXT5) return (INT)(std::max(1u, (m_width + 3) / 4) * 16);
    if (m_format == D3DFMT_A4R4G4B4 || m_format == D3DFMT_A1R5G5B5 || m_format == D3DFMT_R5G6B5) return (INT)(m_width * 2);
    return (INT)(m_width * 4);
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
      m_srcBlend(D3DBLEND_SRCALPHA), m_destBlend(D3DBLEND_INVSRCALPHA)
{
    std::memset(&m_viewport, 0, sizeof(m_viewport));
    m_viewport.Width = 1024;
    m_viewport.Height = 768;
    m_viewport.MaxZ = 1.0f;
    for (int i = 0; i < 8; ++i) m_boundTextures[i] = nullptr;
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
    if (State == D3DRS_ALPHABLENDENABLE) {
        if (Value) glEnable(GL_BLEND);
        else glDisable(GL_BLEND);
    } else if (State == D3DRS_ZENABLE) {
        if (Value) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
    } else if (State == D3DRS_CULLMODE) {
        if (Value == D3DCULL_NONE) glDisable(GL_CULL_FACE);
        else {
            glEnable(GL_CULL_FACE);
            glCullFace((Value == D3DCULL_CW) ? GL_BACK : GL_FRONT);
        }
    } else if (State == D3DRS_SRCBLEND) {
        m_srcBlend = Value;
    } else if (State == D3DRS_DESTBLEND) {
        m_destBlend = Value;
    }
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

static inline void SetupGL2DState(const D3DVIEWPORT9& vp, IDirect3DTexture9* boundTex, DWORD srcBlend, DWORD destBlend)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, vp.Width, vp.Height, 0.0, -1000.0, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(ToGLBlend(srcBlend), ToGLBlend(destBlend));
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (boundTex && boundTex->GetGLTexture()) {
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, boundTex->GetGLTexture());
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    } else {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
}

HRESULT IDirect3DDevice9::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    if (!m_streams[0].vb) return S_OK;

    const BYTE* vbData = (const BYTE*)m_streams[0].vb->GetData() + m_streams[0].offset;
    UINT stride = m_streams[0].stride ? m_streams[0].stride : 24;

    SetupGL2DState(m_viewport, m_boundTextures[0], m_srcBlend, m_destBlend);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, vbData);

    bool hasDiffuse = (m_fvf & D3DFVF_DIFFUSE) != 0;
    bool hasTex = (m_fvf & D3DFVF_TEX1) != 0;

    if (hasDiffuse) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, vbData + 12);
    } else {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (hasTex) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, vbData + (hasDiffuse ? 16 : 12));
    }

    GLenum glMode = GL_TRIANGLES;
    UINT vtxCount = PrimitiveCount * 3;
    if (PrimitiveType == D3DPT_TRIANGLESTRIP) { glMode = GL_TRIANGLE_STRIP; vtxCount = PrimitiveCount + 2; }
    else if (PrimitiveType == D3DPT_LINELIST) { glMode = GL_LINES; vtxCount = PrimitiveCount * 2; }
    else if (PrimitiveType == D3DPT_LINESTRIP) { glMode = GL_LINE_STRIP; vtxCount = PrimitiveCount + 1; }
    else if (PrimitiveType == D3DPT_POINTLIST) { glMode = GL_POINTS; vtxCount = PrimitiveCount; }

    glDrawArrays(glMode, StartVertex, vtxCount);

    glDisableClientState(GL_VERTEX_ARRAY);
    if (hasDiffuse) glDisableClientState(GL_COLOR_ARRAY);
    if (hasTex) glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    return S_OK;
}

HRESULT IDirect3DDevice9::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT, UINT, UINT, UINT StartIndex, UINT PrimitiveCount)
{
    if (!m_streams[0].vb || !m_pIndexBuffer) return S_OK;

    const BYTE* vbData = (const BYTE*)m_streams[0].vb->GetData() + m_streams[0].offset;
    UINT stride = m_streams[0].stride ? m_streams[0].stride : 24;
    const WORD* ibData = (const WORD*)m_pIndexBuffer->GetData() + StartIndex;

    SetupGL2DState(m_viewport, m_boundTextures[0], m_srcBlend, m_destBlend);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, vbData);

    bool hasDiffuse = (m_fvf & D3DFVF_DIFFUSE) != 0;
    bool hasTex = (m_fvf & D3DFVF_TEX1) != 0;

    if (hasDiffuse) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, vbData + 12);
    } else {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (hasTex) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, vbData + (hasDiffuse ? 16 : 12));
    }

    GLenum glMode = GL_TRIANGLES;
    UINT indexCount = PrimitiveCount * 3;
    if (PrimitiveType == D3DPT_TRIANGLESTRIP) { glMode = GL_TRIANGLE_STRIP; indexCount = PrimitiveCount + 2; }
    else if (PrimitiveType == D3DPT_LINELIST) { glMode = GL_LINES; indexCount = PrimitiveCount * 2; }
    else if (PrimitiveType == D3DPT_LINESTRIP) { glMode = GL_LINE_STRIP; indexCount = PrimitiveCount + 1; }
    else if (PrimitiveType == D3DPT_POINTLIST) { glMode = GL_POINTS; indexCount = PrimitiveCount; }

    glDrawElements(glMode, indexCount, GL_UNSIGNED_SHORT, ibData);

    glDisableClientState(GL_VERTEX_ARRAY);
    if (hasDiffuse) glDisableClientState(GL_COLOR_ARRAY);
    if (hasTex) glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    return S_OK;
}

HRESULT IDirect3DDevice9::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    if (!pVertexStreamZeroData) return S_OK;

    const BYTE* vbData = (const BYTE*)pVertexStreamZeroData;
    UINT stride = VertexStreamZeroStride ? VertexStreamZeroStride : 24;

    SetupGL2DState(m_viewport, m_boundTextures[0], m_srcBlend, m_destBlend);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, vbData);

    bool hasDiffuse = (m_fvf & D3DFVF_DIFFUSE) != 0;
    bool hasTex = (m_fvf & D3DFVF_TEX1) != 0;

    if (hasDiffuse) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, vbData + 12);
    } else {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (hasTex) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, vbData + (hasDiffuse ? 16 : 12));
    }

    GLenum glMode = GL_TRIANGLES;
    UINT vtxCount = PrimitiveCount * 3;
    if (PrimitiveType == D3DPT_TRIANGLESTRIP) { glMode = GL_TRIANGLE_STRIP; vtxCount = PrimitiveCount + 2; }
    else if (PrimitiveType == D3DPT_LINELIST) { glMode = GL_LINES; vtxCount = PrimitiveCount * 2; }
    else if (PrimitiveType == D3DPT_LINESTRIP) { glMode = GL_LINE_STRIP; vtxCount = PrimitiveCount + 1; }
    else if (PrimitiveType == D3DPT_POINTLIST) { glMode = GL_POINTS; vtxCount = PrimitiveCount; }

    glDrawArrays(glMode, 0, vtxCount);

    glDisableClientState(GL_VERTEX_ARRAY);
    if (hasDiffuse) glDisableClientState(GL_COLOR_ARRAY);
    if (hasTex) glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    return S_OK;
}

HRESULT IDirect3DDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT, UINT, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    if (!pVertexStreamZeroData || !pIndexData) return S_OK;

    const BYTE* vbData = (const BYTE*)pVertexStreamZeroData;
    UINT stride = VertexStreamZeroStride ? VertexStreamZeroStride : 24;

    SetupGL2DState(m_viewport, m_boundTextures[0], m_srcBlend, m_destBlend);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, vbData);

    bool hasDiffuse = (m_fvf & D3DFVF_DIFFUSE) != 0;
    bool hasTex = (m_fvf & D3DFVF_TEX1) != 0;

    if (hasDiffuse) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, vbData + 12);
    } else {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (hasTex) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, vbData + (hasDiffuse ? 16 : 12));
    }

    GLenum glMode = GL_TRIANGLES;
    UINT indexCount = PrimitiveCount * 3;
    if (PrimitiveType == D3DPT_TRIANGLESTRIP) { glMode = GL_TRIANGLE_STRIP; indexCount = PrimitiveCount + 2; }
    else if (PrimitiveType == D3DPT_LINELIST) { glMode = GL_LINES; indexCount = PrimitiveCount * 2; }
    else if (PrimitiveType == D3DPT_LINESTRIP) { glMode = GL_LINE_STRIP; indexCount = PrimitiveCount + 1; }
    else if (PrimitiveType == D3DPT_POINTLIST) { glMode = GL_POINTS; indexCount = PrimitiveCount; }

    GLenum idxType = (IndexDataFormat == D3DFMT_INDEX32) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    glDrawElements(glMode, indexCount, idxType, pIndexData);

    glDisableClientState(GL_VERTEX_ARRAY);
    if (hasDiffuse) glDisableClientState(GL_COLOR_ARRAY);
    if (hasTex) glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    return S_OK;
}

#endif // __LINUX__
