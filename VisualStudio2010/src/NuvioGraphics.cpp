#include "NuvioGraphics.h"
#include "NuvioUiAtlas.h"
#include "NuvioLogoPng.h"
#include "stb_image.h"

#include <d3dx9.h>
#include <cmath>
#include <cstring>

// Xbox 360 extension: screen-space coordinates with viewport disabled.
static const char *kVertexSource =
    "struct VS_IN { float4 p:POSITION; float2 uv:TEXCOORD0; float4 tint:TEXCOORD1; };"
    "struct VS_OUT { float4 p:POSITION; float2 uv:TEXCOORD0; float4 tint:TEXCOORD1; };"
    "VS_OUT main(VS_IN i) { VS_OUT o; o.p=i.p; o.uv=i.uv; o.tint=i.tint; return o; }";
static const char *kPixelSource =
    "sampler2D tex:register(s0);"
    "struct PS_IN { float2 uv:TEXCOORD0; float4 tint:TEXCOORD1; };"
    "float4 main(PS_IN i):COLOR0 { return tex2D(tex,i.uv)*i.tint; }";

NuvioGraphics::NuvioGraphics()
 : m_device(NULL), m_scaleX(1.0f), m_scaleY(1.0f), m_vertexShader(NULL), m_pixelShader(NULL),
   m_vertexDeclaration(NULL), m_white(NULL), m_font(NULL), m_logo(NULL), m_backdrop(NULL),
   m_batchTexture(NULL), m_batch(NULL), m_batchQuads(0)
{
    std::memset(m_posters, 0, sizeof(m_posters));
    for (int i=0;i<POSTER_CACHE_SIZE;++i) { m_posterIds[i]=-1; m_posterAge[i]=0; }
    m_cacheClock=0;
    std::memset(m_avatars, 0, sizeof(m_avatars));
}

NuvioGraphics::~NuvioGraphics()
{
    Shutdown();
}

void NuvioGraphics::Shutdown()
{
    delete[] m_batch;
    m_batch = NULL;
    for (int i = 0; i < POSTER_CACHE_SIZE; ++i)
        if (m_posters[i]) { m_posters[i]->Release(); m_posters[i] = NULL; }
    ClearAvatars();
    ClearBackdrop();
    if (m_font) { m_font->Release(); m_font = NULL; }
    if (m_logo) { m_logo->Release(); m_logo = NULL; }
    if (m_white) { m_white->Release(); m_white = NULL; }
    if (m_vertexDeclaration) { m_vertexDeclaration->Release(); m_vertexDeclaration = NULL; }
    if (m_pixelShader) { m_pixelShader->Release(); m_pixelShader = NULL; }
    if (m_vertexShader) { m_vertexShader->Release(); m_vertexShader = NULL; }
}

IDirect3DTexture9 *NuvioGraphics::CreateTexture(const unsigned char *rgba,
                                                 int width, int height, bool grayscale)
{
    if (!m_device || width <= 0 || height <= 0 || width > 1024 || height > 1024)
        return NULL;
    IDirect3DTexture9 *tex = NULL;
    if (FAILED(m_device->CreateTexture(width, height, 1, 0, D3DFMT_LIN_A8R8G8B8,
                                       D3DPOOL_DEFAULT, &tex, NULL)) || !tex) return NULL;
    D3DLOCKED_RECT lock;
    if (FAILED(tex->LockRect(0, &lock, NULL, 0))) { tex->Release(); return NULL; }
    for (int y = 0; y < height; ++y) {
        DWORD *dst = reinterpret_cast<DWORD *>(reinterpret_cast<unsigned char *>(lock.pBits) + y * lock.Pitch);
        for (int x = 0; x < width; ++x) {
            const int offset = y * width + x;
            if (grayscale) {
                dst[x] = D3DCOLOR_ARGB(rgba[offset], 255, 255, 255);
            } else {
                const unsigned char *px = rgba + offset * 4;
                dst[x] = D3DCOLOR_ARGB(px[3], px[0], px[1], px[2]);
            }
        }
    }
    tex->UnlockRect(0);
    return tex;
}

bool NuvioGraphics::Initialize(IDirect3DDevice9 *device, float scaleX, float scaleY)
{
    m_device = device;
    m_scaleX = scaleX > 0.0f ? scaleX : 1.0f;
    m_scaleY = scaleY > 0.0f ? scaleY : 1.0f;
    m_batch = new SpriteVertex[MAX_BATCH_QUADS * 4];
    if (!m_batch) return false;
    ID3DXBuffer *compiled = NULL;
    ID3DXBuffer *errors = NULL;
    if (FAILED(D3DXCompileShader(kVertexSource, (UINT)std::strlen(kVertexSource),
          NULL, NULL, "main", "vs_2_0", 0, &compiled, &errors, NULL))) {
        if (errors) errors->Release();
        OutputDebugStringA("NUVIO - Vertex shader failed to compile\n");
        return false;
    }
    HRESULT result = m_device->CreateVertexShader(
        reinterpret_cast<const DWORD *>(compiled->GetBufferPointer()), &m_vertexShader);
    compiled->Release();
    if (FAILED(result) || !m_vertexShader) return false;
    if (FAILED(D3DXCompileShader(kPixelSource, (UINT)std::strlen(kPixelSource),
          NULL, NULL, "main", "ps_2_0", 0, &compiled, &errors, NULL))) {
        if (errors) errors->Release();
        OutputDebugStringA("NUVIO - Pixel shader failed to compile\n");
        return false;
    }
    result = m_device->CreatePixelShader(
        reinterpret_cast<const DWORD *>(compiled->GetBufferPointer()), &m_pixelShader);
    compiled->Release();
    if (FAILED(result) || !m_pixelShader) return false;

    static const D3DVERTEXELEMENT9 elements[] = {
        {0,  0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 24, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    };
    if (FAILED(m_device->CreateVertexDeclaration(elements, &m_vertexDeclaration))) return false;
    const unsigned char white[4] = {255, 255, 255, 255};
    m_white = CreateTexture(white, 1, 1, false);

    unsigned char *pixels = new unsigned char[NUVIO_ATLAS_WIDTH * NUVIO_ATLAS_HEIGHT];
    std::memset(pixels, 0, NUVIO_ATLAS_WIDTH * NUVIO_ATLAS_HEIGHT);
    unsigned position = 0;
    for (unsigned index = 0; index + 1 < sizeof(kNuvioUiAtlasRle); index += 2) {
        const unsigned repeat = kNuvioUiAtlasRle[index];
        if (position + repeat > NUVIO_ATLAS_WIDTH * NUVIO_ATLAS_HEIGHT) break;
        std::memset(pixels + position, kNuvioUiAtlasRle[index + 1], repeat);
        position += repeat;
    }
    m_font = CreateTexture(pixels, NUVIO_ATLAS_WIDTH, NUVIO_ATLAS_HEIGHT, true);
    delete[] pixels;
    int logoW = 0, logoH = 0, channels = 0;
    unsigned char *logo = stbi_load_from_memory(kNuvioLogoPng, sizeof(kNuvioLogoPng),
                                                &logoW, &logoH, &channels, 4);
    if (logo) {
        m_logo = CreateTexture(logo, logoW, logoH, false);
        stbi_image_free(logo);
    }
    return m_white && m_font && m_logo;
}

void NuvioGraphics::Logo(float x, float y, float size) {
    // The official transparent mark is 488 x 536: keep its native proportions.
    if (m_logo) Quad(x, y, size * (488.0f / 536.0f), size, 0, 0, 1, 1, m_logo, D3DCOLOR_ARGB(255,255,255,255));
}

void NuvioGraphics::Begin()
{
    m_batchQuads = 0;
    m_batchTexture = NULL;
    m_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 11, 13, 17), 1.0f, 0);
    m_device->BeginScene();
    ResumeUI();
}

void NuvioGraphics::ResumeUI()
{
    m_device->SetVertexShader(m_vertexShader);
    m_device->SetPixelShader(m_pixelShader);
    m_device->SetVertexDeclaration(m_vertexDeclaration);
    m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_device->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_device->SetRenderState(D3DRS_VIEWPORTENABLE, FALSE);
    m_device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    m_device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    m_device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
}

void NuvioGraphics::Flush()
{
    if (m_batchQuads <= 0) return;
    m_device->SetTexture(0, m_batchTexture ? m_batchTexture : m_white);
    m_device->DrawPrimitiveUP(D3DPT_QUADLIST, m_batchQuads, m_batch, sizeof(SpriteVertex));
    m_batchQuads = 0;
}

void NuvioGraphics::Quad(float x, float y, float w, float h,
                         float u0, float v0, float u1, float v1,
                         IDirect3DTexture9 *texture, D3DCOLOR color)
{
    if (w <= 0.0f || h <= 0.0f) return;
    if (m_batchQuads == MAX_BATCH_QUADS || texture != m_batchTexture) {
        Flush();
        m_batchTexture = texture;
    }
    SpriteVertex *v = &m_batch[m_batchQuads * 4];
    const float r = float((color >> 16) & 255) / 255.0f;
    const float g = float((color >> 8) & 255) / 255.0f;
    const float b = float(color & 255) / 255.0f;
    const float a = float((color >> 24) & 255) / 255.0f;
    const float posX[4] = {x, x + w, x + w, x};
    const float posY[4] = {y, y, y + h, y + h};
    const float uvX[4] = {u0, u1, u1, u0};
    const float uvY[4] = {v0, v0, v1, v1};
    for (int i = 0; i < 4; ++i) {
        v[i].x = posX[i] * m_scaleX; v[i].y = posY[i] * m_scaleY;
        v[i].z = 0.0f; v[i].w = 1.0f;
        v[i].u = uvX[i]; v[i].v = uvY[i];
        v[i].r = r; v[i].g = g; v[i].b = b; v[i].a = a;
    }
    ++m_batchQuads;
}

void NuvioGraphics::Rect(float x, float y, float w, float h, D3DCOLOR color)
{
    Quad(x, y, w, h, 0, 0, 1, 1, m_white, color);
}

void NuvioGraphics::RoundedRect(float x, float y, float w, float h, float radius,
                                D3DCOLOR color)
{
    if (radius <= 0) { Rect(x, y, w, h, color); return; }
    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;
    Rect(x, y + radius, w, h - 2 * radius, color);
    for (int row = 0; row < int(radius); row += 2) {
        const float yDist = radius - float(row) - 1.0f;
        const float reach = float(std::sqrt(radius * radius - yDist * yDist));
        const float inset = radius - reach;
        Rect(x + inset, y + row, w - 2 * inset, 2, color);
        Rect(x + inset, y + h - row - 2, w - 2 * inset, 2, color);
    }
}

void NuvioGraphics::Circle(float x, float y, float radius, D3DCOLOR color)
{
    for (int row = -int(radius); row < int(radius); row += 2) {
        const float reach = float(std::sqrt(radius * radius - float(row * row)));
        Rect(x - reach, y + row, reach * 2, 2, color);
    }
}

void NuvioGraphics::Text(float x, float y, float fontSize, const char *text,
                         D3DCOLOR color, float maxWidth)
{
    if (!text || fontSize <= 0) return;
    const float scale = fontSize / 31.0f;
    const float start = x;
    for (; *text; ++text) {
        if (*text == '\n') { x = start; y += fontSize * 1.35f; continue; }
        int c = static_cast<unsigned char>(*text);
        if (c < 32 || c > 127) c = '?';
        const int index = c - 32;
        const float advance = float(kNuvioGlyphAdvance[index]) * scale;
        if (x - start + advance > maxWidth) break;
        const int cellX = (index % 12) * NUVIO_FONT_CELL_W;
        const int cellY = (index / 12) * NUVIO_FONT_CELL_H;
        Quad(x, y, NUVIO_FONT_CELL_W * scale, NUVIO_FONT_CELL_H * scale,
             float(cellX) / NUVIO_ATLAS_WIDTH, float(cellY) / NUVIO_ATLAS_HEIGHT,
             float(cellX + NUVIO_FONT_CELL_W) / NUVIO_ATLAS_WIDTH,
             float(cellY + NUVIO_FONT_CELL_H) / NUVIO_ATLAS_HEIGHT,
             m_font, color);
        x += advance;
    }
}

void NuvioGraphics::Icon(int index, float x, float y, float size, D3DCOLOR color)
{
    if (index < 0 || index >= 5) return;
    Quad(x, y, size, size, float(index * 64) / NUVIO_ATLAS_WIDTH,
         float(NUVIO_ICON_TOP) / NUVIO_ATLAS_HEIGHT,
         float((index + 1) * 64) / NUVIO_ATLAS_WIDTH,
         float(NUVIO_ICON_TOP + 64) / NUVIO_ATLAS_HEIGHT, m_font, color);
}

bool NuvioGraphics::UploadPoster(int index, const unsigned char *rgba, int width, int height)
{
    if (index < 0 || index >= NUVIO_MAX_CATALOG_ITEMS || !rgba ||
        width < 32 || height < 32 || width > 512 || height > 768) return false;
    IDirect3DTexture9 *tex = CreateTexture(rgba, width, height, false);
    if (!tex) return false;
    int slot=-1;
    for (int i=0;i<POSTER_CACHE_SIZE;++i) if (m_posterIds[i]==index) {slot=i;break;}
    if (slot<0) for (int i=0;i<POSTER_CACHE_SIZE;++i) if (!m_posters[i]) {slot=i;break;}
    if (slot<0) {
        slot=0;
        for (int i=1;i<POSTER_CACHE_SIZE;++i) if (m_posterAge[i]<m_posterAge[slot]) slot=i;
    }
    if (m_posters[slot]) m_posters[slot]->Release();
    m_posters[slot] = tex; m_posterIds[slot]=index; m_posterAge[slot]=++m_cacheClock;
    return true;
}

bool NuvioGraphics::HasPoster(int index) const
{
    for (int i=0;i<POSTER_CACHE_SIZE;++i) if (m_posterIds[i]==index && m_posters[i]) return true;
    return false;
}

void NuvioGraphics::ClearPosters()
{
    Flush();
    for (int i = 0; i < POSTER_CACHE_SIZE; ++i) {
        if (m_posters[i]) { m_posters[i]->Release(); m_posters[i] = NULL; }
        m_posterIds[i]=-1; m_posterAge[i]=0;
    }
}

void NuvioGraphics::Poster(int index, float x, float y, float w, float h)
{
    for (int i=0;i<POSTER_CACHE_SIZE;++i) {
        if (m_posterIds[i]!=index || !m_posters[i]) continue;
        m_posterAge[i]=++m_cacheClock;
        Quad(x,y,w,h,0,0,1,1,m_posters[i],0xFFFFFFFF); return;
    }
}

bool NuvioGraphics::UploadAvatar(int index, const unsigned char *rgba, int width, int height)
{
    if (index < 0 || index >= 6 || !rgba || width < 32 || height < 32 || width > 256 || height > 256)
        return false;
    IDirect3DTexture9 *tex = CreateTexture(rgba, width, height, false);
    if (!tex) return false;
    if (m_avatars[index]) m_avatars[index]->Release();
    m_avatars[index] = tex;
    return true;
}

bool NuvioGraphics::HasAvatar(int index) const
{
    return index >= 0 && index < 6 && m_avatars[index] != NULL;
}

void NuvioGraphics::Avatar(int index, float x, float y, float size)
{
    if (!HasAvatar(index)) return;
    Quad(x, y, size, size, 0, 0, 1, 1, m_avatars[index], 0xFFFFFFFF);
}

void NuvioGraphics::ClearAvatars()
{
    for (int i = 0; i < 6; ++i)
        if (m_avatars[i]) { m_avatars[i]->Release(); m_avatars[i] = NULL; }
}

bool NuvioGraphics::UploadBackdrop(const unsigned char *rgba, int width, int height)
{
    if (!rgba || width < 64 || height < 64 || width > 1024 || height > 1024) return false;
    IDirect3DTexture9 *tex = CreateTexture(rgba, width, height, false);
    if (!tex) return false;
    ClearBackdrop();
    m_backdrop = tex;
    return true;
}

void NuvioGraphics::ClearBackdrop()
{
    if (m_backdrop) { m_backdrop->Release(); m_backdrop = NULL; }
}

bool NuvioGraphics::HasBackdrop() const { return m_backdrop != NULL; }

void NuvioGraphics::Backdrop(float x, float y, float w, float h)
{
    if (m_backdrop) Quad(x, y, w, h, 0, 0, 1, 1, m_backdrop, 0xFFFFFFFF);
}

void NuvioGraphics::End()
{
    Flush();
    m_device->EndScene();
    m_device->Present(NULL, NULL, NULL, NULL);
}
