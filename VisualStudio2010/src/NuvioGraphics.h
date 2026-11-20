#ifndef NUVIO_GRAPHICS_H
#define NUVIO_GRAPHICS_H

#include <xtl.h>
#include "NuvioCatalog.h"

// A native Xbox 360 2D renderer: anti-aliased glyph atlas + textured quads.
// Independent implementation following the public Xbox XDK Direct3D 9 API.
class NuvioGraphics {
public:
    NuvioGraphics();
    ~NuvioGraphics();

    bool Initialize(IDirect3DDevice9 *device);
    void Shutdown();
    void Begin();
    void ResumeUI();
    void End();
    void Rect(float x, float y, float w, float h, D3DCOLOR color);
    void RoundedRect(float x, float y, float w, float h, float radius, D3DCOLOR color);
    void Circle(float x, float y, float radius, D3DCOLOR color);
    void Text(float x, float y, float fontSize, const char *text, D3DCOLOR color, float maxWidth = 9999.0f);
    void Icon(int index, float x, float y, float size, D3DCOLOR color);
    void Logo(float x, float y, float size);
    void Poster(int index, float x, float y, float w, float h);
    void Avatar(int index, float x, float y, float size);
    bool UploadAvatar(int index, const unsigned char *rgba, int width, int height);
    bool HasAvatar(int index) const;
    void ClearAvatars();
    void Backdrop(float x, float y, float w, float h);
    bool UploadPoster(int index, const unsigned char *rgba, int width, int height);
    bool UploadBackdrop(const unsigned char *rgba, int width, int height);
    bool HasBackdrop() const;
    void ClearBackdrop();
    bool HasPoster(int index) const;
    void ClearPosters();

private:
    struct SpriteVertex {
        float x, y, z, w;
        float u, v;
        float r, g, b, a;
    };
    enum { MAX_BATCH_QUADS = 512 };
    void Flush();
    void Quad(float x, float y, float w, float h,
              float u0, float v0, float u1, float v1,
              IDirect3DTexture9 *texture, D3DCOLOR color);
    IDirect3DTexture9 *CreateTexture(const unsigned char *rgba, int width, int height, bool grayscale);

    IDirect3DDevice9 *m_device;
    IDirect3DVertexShader9 *m_vertexShader;
    IDirect3DPixelShader9 *m_pixelShader;
    IDirect3DVertexDeclaration9 *m_vertexDeclaration;
    IDirect3DTexture9 *m_white;
    IDirect3DTexture9 *m_font;
    IDirect3DTexture9 *m_logo;
    IDirect3DTexture9 *m_backdrop;
    enum { POSTER_CACHE_SIZE = 12 };
    IDirect3DTexture9 *m_posters[POSTER_CACHE_SIZE];
    int m_posterIds[POSTER_CACHE_SIZE];
    unsigned m_posterAge[POSTER_CACHE_SIZE], m_cacheClock;
    IDirect3DTexture9 *m_avatars[6];
    IDirect3DTexture9 *m_batchTexture;
    SpriteVertex *m_batch;
    int m_batchQuads;
};

#endif
