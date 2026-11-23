
// NUVIO - Xbox 360 Streaming Interface
// Native 720p Smart TV UI with HTTPS, catalog and Nuvio cloud account on Xbox.
// Screens: Sign in, Who's watching, Create profile, Home, Discover, Search, Library,
//          Settings, Plugins, Install plugin (virtual keyboard), Details.

#include <xtl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "NuvioApp.h"
#include "NuvioCatalog.h"
#include "NuvioGraphics.h"
#include "NuvioInput.h"
#include "NuvioPlayback.h"
#include "NuvioTorrent.h"
#include "FFPlayer.h"
#include <vector>
#include <string>
#include "NuvioNetwork.h"
#include "NuvioNativeBackend.h"

enum {
    SCREEN_WIDTH = 1280,
    SCREEN_HEIGHT = 720,
    MAX_POSTERS = 240
};

static const char *kProfilesFile = "game:\\nuvio_profiles.cfg";
static const char *kP2PConfigFile = "game:\\nuvio_p2p.cfg";
static bool g_p2pEnabled = true;
static bool g_hideTorrentStats = false;
static bool g_playingTorrent = false;
static void LoadP2PSettings() {
    std::FILE *file=std::fopen(kP2PConfigFile,"rb");
    if(!file)return;
    char line[80];
    while(std::fgets(line,sizeof(line),file)) {
        if(!std::strncmp(line,"enabled=",8))g_p2pEnabled=line[8]=='1';
        if(!std::strncmp(line,"hide_stats=",11))g_hideTorrentStats=line[11]=='1';
    }
    std::fclose(file);
}
static bool SaveP2PSettings() {
    std::FILE *file=std::fopen(kP2PConfigFile,"wb");
    if(!file)return false;
    bool saved=std::fprintf(file,"enabled=%d\nhide_stats=%d\n",g_p2pEnabled?1:0,g_hideTorrentStats?1:0)>0;
    if(std::fclose(file)!=0)saved=false;
    return saved;
}

// Color palette - dark streaming theme
static const D3DCOLOR BG_DARK      = D3DCOLOR_ARGB(255, 13, 13, 13);
static const D3DCOLOR BG_CARD      = D3DCOLOR_ARGB(255, 34, 34, 34);
static const D3DCOLOR BG_HOVER     = D3DCOLOR_ARGB(255, 48, 48, 48);
static const D3DCOLOR BG_SIDEBAR   = D3DCOLOR_ARGB(255, 13, 13, 13);
static const D3DCOLOR ACCENT_RED   = D3DCOLOR_ARGB(255, 48, 48, 48);
static const D3DCOLOR ACCENT_BLUE  = D3DCOLOR_ARGB(255, 77, 77, 77);
static const D3DCOLOR ACCENT_GOLD  = D3DCOLOR_ARGB(255, 255, 215, 0);
static const D3DCOLOR ACCENT_GREEN = D3DCOLOR_ARGB(255, 76, 175, 80);
static const D3DCOLOR TEXT_WHITE   = D3DCOLOR_ARGB(255, 255, 255, 255);
static const D3DCOLOR TEXT_MUTED   = D3DCOLOR_ARGB(255, 179, 179, 179);
static const D3DCOLOR TEXT_DIM     = D3DCOLOR_ARGB(255, 128, 128, 128);
static const D3DCOLOR BORDER_SUBTLE= D3DCOLOR_ARGB(255, 51, 51, 51);
static const D3DCOLOR FOCUS_BLUE   = D3DCOLOR_ARGB(255, 255, 255, 255);

static inline D3DCOLOR Color(int r, int g, int b, int a = 255) {
    return D3DCOLOR_ARGB(a, r, g, b);
}

// Xbox face buttons use their standard colors even with the ASCII-only atlas.
static D3DCOLOR PadColor(char b) {
    if(b=='A')return Color(66,178,77);
    if(b=='B')return Color(215,58,64);
    if(b=='X')return Color(46,137,211);
    if(b=='Y')return Color(235,188,44);
    return TEXT_DIM;
}
static bool PadButton(char b) {return b=='A'||b=='B'||b=='X'||b=='Y';}
static void DrawPadHint(NuvioGraphics &g,float x,float y,char button,const char *label) {
    g.Circle(x+12,y+13,13,Color(14,14,14));
    g.Circle(x+12,y+12,11,PadColor(button));
    char letter[2]={button,0};
    g.Text(x+6,y+4,15,letter,TEXT_WHITE);
    g.Text(x+31,y+4,16,label,TEXT_MUTED,230);
}
static void DrawControllerFooter(NuvioGraphics &g,float x,float y,const char *hints) {
    const char *p=hints;
    while(*p&&x<SCREEN_WIDTH-45) {
        while(*p==' ')++p;
        if(!*p)break;
        const char *next=std::strstr(p,"  ");
        size_t n=next?(size_t)(next-p):std::strlen(p);
        char part[128];if(n>=sizeof(part))n=sizeof(part)-1;
        std::memcpy(part,p,n);part[n]=0;
        if(n>2&&part[1]==':'&&PadButton(part[0])) {
            DrawPadHint(g,x,y,part[0],part+2);
            x+=31+(float)std::strlen(part+2)*8.3f+20;
        } else {
            g.Text(x,y+4,16,part,TEXT_DIM,SCREEN_WIDTH-x-12);
            x+=(float)n*8.3f+20;
        }
        p=next?next+2:p+std::strlen(p);
    }
}
// Language is inferred only from addon metadata; unknown languages stay neutral.
static void DrawLanguageFlag(NuvioGraphics &g,float x,float y,const char *country) {
    g.RoundedRect(x-2,y-2,54,34,6,Color(74,74,74));
    g.Rect(x,y,50,30,Color(51,51,51));
    if(!std::strcmp(country,"BR")) {
        g.Rect(x,y,50,30,Color(24,145,61));
        g.Rect(x+10,y+6,30,18,Color(247,204,48));
        g.Circle(x+25,y+15,8,Color(37,83,181));
    } else if(!std::strcmp(country,"US")) {
        g.Rect(x,y,50,30,Color(250,250,250));
        for(int i=0;i<7;++i)g.Rect(x,y+i*4.3f,50,2.4f,Color(196,47,54));
        g.Rect(x,y,23,16,Color(47,65,140));
    } else if(!std::strcmp(country,"ES")) {
        g.Rect(x,y,50,30,Color(183,32,47));g.Rect(x,y+8,50,14,Color(255,201,43));
    } else if(!std::strcmp(country,"JP")) {
        g.Rect(x,y,50,30,Color(250,250,250));g.Circle(x+25,y+15,9,Color(188,30,63));
    } else if(!std::strcmp(country,"FR")||!std::strcmp(country,"IT")) {
        const bool it=country[0]=='I';
        g.Rect(x,y,17,30,it?Color(11,143,86):Color(38,59,149));
        g.Rect(x+17,y,16,30,Color(250,250,250));
        g.Rect(x+33,y,17,30,it?Color(205,42,52):Color(228,51,62));
    } else if(!std::strcmp(country,"DE")) {
        g.Rect(x,y,50,10,Color(15,15,15));
        g.Rect(x,y+10,50,10,Color(211,36,42));
        g.Rect(x,y+20,50,10,Color(248,202,44));
    } else if(!std::strcmp(country,"PT")) {
        g.Rect(x,y,21,30,Color(22,111,73));g.Rect(x+21,y,29,30,Color(202,43,51));
        g.Circle(x+21,y+15,7,Color(234,198,54));
    } else if(!std::strcmp(country,"KR")) {
        g.Rect(x,y,50,30,Color(250,250,250));g.Circle(x+25,y+13,8,Color(207,46,61));
        g.Rect(x+17,y+15,16,7,Color(38,76,168));
    } else g.Text(x+13,y+5,17,"--",TEXT_MUTED);
}

// Shared data (kept off the stack: the catalog alone is ~150 KB)
static NuvioCatalog g_catalog;
static FFPlayer *g_player=NULL;
static DWORD g_playerInputAt=0;
static NuvioAddonList g_addons;
static char g_status[120] = "";
static int g_controllerPort = -1; // -1 means no controller reported by XInput
static WORD g_lastInputButtons = 0;
static DWORD g_lastInputAt = 0;
static DWORD g_statusAt = 0;
static int BRIDGE_PORT = 0;
static bool g_catalogLoading = false;
static HANDLE g_artThread = NULL;
static WORD g_deferredButtons = 0;

// The card follows the focused controller item; its artwork is fetched once
// after the selection has rested, keeping D-pad movement responsive.
struct PreviewUi {
    char genres[96];
    char runtime[36];
    char rating[16];
    char description[420];
};
static PreviewUi g_preview = { "", "", "", "" };
static int g_previewIndex = -1;
static int g_previewFocusIndex = -1;
static DWORD g_previewFocusAt = 0;

// Official Nuvio device-code sign-in; generated and polled natively on Xbox.
// QR code max modules: version 40 = 177 modules, but we cap at 80 for display.
enum { QR_MAX_MODULES = 80 };
struct AccountUi {
    int state;          // ACCOUNT_* (see below)
    char email[64];
    char code[16];      // short code typed on nuvio.tv/link
    char link[64];
    char error[120];
    DWORD lastPoll;
};
static AccountUi g_account = { 0, "", "", "", "", 0 };
static unsigned char g_qrBits[QR_MAX_MODULES * QR_MAX_MODULES];
static void CopyField(char *target, int size, const char *value);
static bool BeginRenderRequest(int kind,int index,const char *path,int limit);
// The cloud profile indexes are authoritative, never the local array position.
static bool SyncRemoteProfiles(NuvioState &app, const char *ip) {
    NuvioHttpData response = { NULL, 0, 0 };
    if (!NuvioHttpGet(ip, BRIDGE_PORT, "/profiles.tsv", 4096, &response)) return false;
    NuvioProfile remote[NUVIO_MAX_PROFILES];
    int count = 0;
    char *text = reinterpret_cast<char *>(response.bytes);
    char *line = std::strtok(text, "\n");
    while (line && count < NUVIO_MAX_PROFILES) {
        int id, color, locked;
        char name[NUVIO_PROFILE_NAME_LEN] = "";
        if (std::sscanf(line, "%d\t%d\t%d\t%23[^\n]", &id, &color, &locked, name) == 4 &&
            id >= 1 && id <= NUVIO_MAX_PROFILES && name[0]) {
            NuvioProfile &profile = remote[count++];
            std::memset(&profile, 0, sizeof(profile));
            CopyField(profile.name, sizeof(profile.name), name);
            profile.remoteId = id;
            profile.locked = locked ? 1 : 0;
            profile.color = color % NUVIO_PROFILE_COLORS;
        }
        line = std::strtok(NULL, "\n");
    }
    NuvioHttpFree(&response);
    if (count <= 0) return false;
    for (int i = 0; i < count; ++i) app.profiles[i] = remote[i];
    app.profileCount = count;
    app.cloudProfiles = true;
    app.profileIndex = app.profileCursor = 0;
    CopyField(app.profileName, sizeof(app.profileName), app.profiles[0].name);
    return true;
}

static bool AddToCloudLibrary(const char *ip, const NuvioState &app, const NuvioCatalog &catalog) {
    if (app.profileIndex < 0 || app.profileIndex >= app.profileCount ||
        app.profiles[app.profileIndex].remoteId < 1 ||
        app.selectedCatalogIndex < 0 || app.selectedCatalogIndex >= catalog.count) return false;
    const int profileId = app.profiles[app.profileIndex].remoteId;
    const char *imdbId = catalog.items[app.selectedCatalogIndex].imdbId;
    char path[512];
    std::sprintf(path, "/library/add?profile=%d&id=%s", profileId, imdbId);
    return BeginRenderRequest(9,0,path,100);
}
static int g_avatarNext = 0;
static void FetchNextAvatar(NuvioGraphics &g, const NuvioState &app, const char *ip) {
    if (g_avatarNext >= app.profileCount) return;
    const int index = g_avatarNext++;
    if (app.profiles[index].remoteId <= 0 || g.HasAvatar(index)) return;
    char path[64]; std::sprintf(path, "/avatar/%d.rgba", app.profiles[index].remoteId);
    NuvioHttpData image = { NULL, 0, 0 };
    if (!NuvioHttpGet(ip, BRIDGE_PORT, path, 75000, &image)) return;
    if (image.length >= 8 && !std::memcmp(image.bytes, "NVPO", 4)) {
        int w = image.bytes[4] | (image.bytes[5] << 8);
        int h = image.bytes[6] | (image.bytes[7] << 8);
        if (w >= 32 && h >= 32 && image.length == 8 + w * h * 4)
            g.UploadAvatar(index, image.bytes + 8, w, h);
    }
    NuvioHttpFree(&image);
}
static int g_qrSize = 0;

static void SetStatus(const char *text) {
    g_statusAt = GetTickCount();
    std::strncpy(g_status, text, sizeof(g_status) - 1);
    g_status[sizeof(g_status) - 1] = '\0';
}

static D3DCOLOR ProfileColor(int index) {
    switch (index % NUVIO_PROFILE_COLORS) {
        case 0: return Color(229, 9, 20);
        case 1: return Color(0, 122, 255);
        case 2: return Color(0, 174, 160);
        case 3: return Color(255, 186, 8);
        case 4: return Color(150, 80, 220);
        default: return Color(60, 180, 75);
    }
}

static float TextWidth(const char *text, float size) {
    return (float)std::strlen(text) * size * 0.52f;
}

static void DrawCentered(NuvioGraphics &g, float cx, float y, float size,
                         const char *text, D3DCOLOR color) {
    g.Text(cx - TextWidth(text, size) / 2, y, size, text, color);
}

static void InitialOf(const char *name, char out[2]) {
    char c = (name && name[0]) ? name[0] : '?';
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    out[0] = c;
    out[1] = '\0';
}

// Draw the app logo
static void DrawNuvioLogo(NuvioGraphics &g, float x, float y, float scale) {
    const float size = 48.0f * scale;
    g.Logo(x, y, size);
}

// Profile avatar: coloured disc with the profile's initial
static void DrawProfileAvatar(NuvioGraphics &g, float cx, float cy, float radius,
                              D3DCOLOR color, const char *name, bool isAdmin) {
    g.Circle(cx, cy, radius, TEXT_WHITE);
    g.Circle(cx, cy, radius - 3, color);
    char init[2];
    InitialOf(name, init);
    const float size = radius * 1.0f;
    g.Text(cx - size * 0.3f, cy - size * 0.68f, size, init, TEXT_WHITE);
    if (isAdmin) {
        const float badgeR = radius * 0.3f;
        g.Circle(cx + radius * 0.7f, cy + radius * 0.7f, badgeR, ACCENT_GOLD);
        g.Text(cx + radius * 0.7f - 5, cy + radius * 0.7f - 11, 20, "*", BG_DARK);
    }
}

static void DrawGradientBackground(NuvioGraphics &g) {
    // NuvioTV: flat neutral950 background, no gradient and no accent line.
    g.Rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BG_DARK);
}

static void DrawButton(NuvioGraphics &g, float x, float y, float w, float h,
                       const char *label, bool focused, bool primary) {
    // NuvioTV White theme: 12 px radius, white focus ring, #303030 focus fill.
    if (focused) g.RoundedRect(x - 3, y - 3, w + 6, h + 6, 15, FOCUS_BLUE);
    const bool light = primary && !focused;
    g.RoundedRect(x, y, w, h, 12, focused ? BG_HOVER : (light ? Color(245, 245, 245) : BG_CARD));
    DrawCentered(g, x + w / 2, y + h / 2 - 15, 26, label, light ? Color(17, 17, 17) : TEXT_WHITE);
}

static void DrawStatusLine(NuvioGraphics &g, float) {
    if (!g_status[0] || GetTickCount()-g_statusAt>4500 ||
        !std::strncmp(g_status,"Synchronized",12)) return;
    g.RoundedRect(84,675,1100,32,5,Color(22,22,22,242));
    g.Text(98,680,15,g_status,TEXT_MUTED,1070);
}

enum { ACCOUNT_NONE = 0, ACCOUNT_PENDING, ACCOUNT_SIGNED_IN, ACCOUNT_ERROR };
static void SetAccountError(const char *text) {
    g_account.state = ACCOUNT_ERROR;
    std::strncpy(g_account.error, text, sizeof(g_account.error) - 1);
    g_account.error[sizeof(g_account.error) - 1] = '\0';
}

static void CopyField(char *target, int size, const char *value) {
    std::strncpy(target, value, size - 1);
    target[size - 1] = '\0';
}

// Reads the bridge's "KEY<TAB>value" lines: STATUS, EMAIL, CODE, LINK, ERROR.
static void ParseAccountStatus(const NuvioHttpData &data) {
    const char *text = reinterpret_cast<const char *>(data.bytes);
    char status[16] = "";
    g_account.error[0] = '\0';
    int pos = 0;
    while (pos < data.length) {
        int end = pos;
        while (end < data.length && text[end] != '\n') ++end;
        char line[256];
        int n = end - pos;
        if (n > (int)sizeof(line) - 1) n = (int)sizeof(line) - 1;
        std::memcpy(line, text + pos, n);
        line[n] = '\0';
        pos = end + 1;
        char *tab = std::strchr(line, '\t');
        if (!tab) continue;
        *tab = '\0';
        const char *value = tab + 1;
        if (!std::strcmp(line, "STATUS")) CopyField(status, sizeof(status), value);
        else if (!std::strcmp(line, "EMAIL")) CopyField(g_account.email, sizeof(g_account.email), value);
        else if (!std::strcmp(line, "CODE")) CopyField(g_account.code, sizeof(g_account.code), value);
        else if (!std::strcmp(line, "LINK")) CopyField(g_account.link, sizeof(g_account.link), value);
        else if (!std::strcmp(line, "ERROR")) CopyField(g_account.error, sizeof(g_account.error), value);
    }
    if (!std::strcmp(status, "signed_in")) g_account.state = ACCOUNT_SIGNED_IN;
    else if (g_account.error[0]) g_account.state = ACCOUNT_ERROR;
    else if (!std::strcmp(status, "pending")) g_account.state = ACCOUNT_PENDING;
    else g_account.state = ACCOUNT_NONE;
}

// GET an account route on the bridge and update g_account from its reply.
static bool AccountRequest(const char *ip, const char *path) {
    NuvioHttpData result = { NULL, 0, 0 };
    if (!NuvioHttpGet(ip, BRIDGE_PORT, path, 4096, &result)) {
        if (result.status == 404)
            SetAccountError("Nuvio account service is unavailable.");
        else if (result.status == 502)
            SetAccountError("Secure connection failed. Check console network.");
        else
            SetAccountError("Xbox cannot reach the Nuvio account service.");
        return false;
    }
    ParseAccountStatus(result);
    NuvioHttpFree(&result);
    return true;
}

// Downloads the QR code as a module matrix (see nuvio_account.make_qr_payload).
static bool FetchQrMatrix(const char *ip) {
    NuvioHttpData result;
    if (!NuvioHttpGet(ip, BRIDGE_PORT, "/account/qr.bin", 8192, &result)) {
        SetAccountError("Could not generate the QR code on the Xbox.");
        return false;
    }
    bool ok = result.status == 200 && result.length >= 5 &&
              !std::memcmp(result.bytes, "NVQM", 4);
    if (ok) {
        const int size = result.bytes[4];
        ok = size > 0 && size <= QR_MAX_MODULES && result.length == 5 + size * size;
        if (ok) {
            g_qrSize = size;
            std::memcpy(g_qrBits, result.bytes + 5, size * size);
        }
    }
    NuvioHttpFree(&result);
    if (!ok) {
        SetAccountError("QR code is not available. Retry sign-in.");
        return false;
    }
    return true;
}

// Called when QR sign-in is selected: requests a new Nuvio device session.
static void BeginQrSignIn(const char *ip, bool networkReady, NuvioState &app) {
    app.screen = NUVIO_SCREEN_QR_LOGIN;
    g_account.lastPoll = GetTickCount();
    g_qrSize = 0;
    g_account.error[0] = '\0';
    if (!networkReady) {
        SetAccountError("Network is not ready on this console. Press A to retry.");
        return;
    }
    if (!AccountRequest(ip, "/account/restart")) return;
    if (g_account.state == ACCOUNT_SIGNED_IN) {
        if (!SyncRemoteProfiles(app, ip)) SetStatus("Signed in. Could not sync Nuvio profiles yet.");
        else if (g_account.error[0]) SetStatus(g_account.error);
        app.screen = NUVIO_SCREEN_PROFILE;
        app.profileCursor = app.profileIndex;
        return;
    }
    if (g_account.state == ACCOUNT_PENDING) FetchQrMatrix(ip);
}

static void DrawQrLoginScreen(NuvioGraphics &g, const NuvioState &state) {
    (void)state;
    DrawGradientBackground(g);

    // Left panel - Instructions (NuvioTV style)
    const float leftX = 80;
    const float leftY = 120;
    DrawNuvioLogo(g, leftX, leftY, 1.6f);
    g.Text(leftX + 80, leftY + 5, 55, "nuvio", TEXT_WHITE);
    g.Text(leftX, leftY + 110, 36, "Sign in to Nuvio", TEXT_WHITE);
    g.Text(leftX, leftY + 170, 22, "Use your phone to scan the QR code", TEXT_MUTED);
    g.Text(leftX, leftY + 205, 22, "or visit the link below.", TEXT_MUTED);

    // Link and code
    g.Text(leftX, leftY + 270, 18, "nuvio.tv/link", TEXT_MUTED);
    if (g_account.code[0]) {
        g.Text(leftX, leftY + 310, 48, g_account.code, TEXT_WHITE);
        g.Text(leftX, leftY + 370, 18, "Enter this code on your phone", TEXT_DIM);
    }

    // Right panel - QR Code
    const float rightX = 760;
    const float rightY = 100;
    const float panelW = 440;
    const float panelH = 520;
    g.RoundedRect(rightX, rightY, panelW, panelH, 20, BG_CARD);
    DrawCentered(g, rightX + panelW / 2, rightY + 40, 26, "Connect your account", TEXT_WHITE);

    const float box = 320;
    const float boxX = rightX + (panelW - box) / 2;
    const float boxY = rightY + 100;

    // White border around QR
    g.RoundedRect(boxX - 16, boxY - 16, box + 32, box + 32, 12, TEXT_WHITE);
    g.RoundedRect(boxX - 8, boxY - 8, box + 16, box + 16, 10, BG_CARD);

    if (g_qrSize > 0) {
        const float cell = box / g_qrSize;
        for (int r = 0; r < g_qrSize; ++r) {
            for (int c = 0; c < g_qrSize; ++c) {
                if (g_qrBits[r * g_qrSize + c])
                    g.Rect(boxX + c * cell, boxY + r * cell, cell + 0.5f, cell + 0.5f, BG_DARK);
            }
        }
    } else {
        g.RoundedRect(boxX, boxY, box, box, 8, BG_CARD);
        DrawCentered(g, boxX + box / 2, boxY + box / 2 - 10, 20, "Loading QR code...", TEXT_MUTED);
    }

    // Status text
    if (g_account.state == ACCOUNT_PENDING) {
        DrawCentered(g, rightX + panelW / 2, boxY + box + 60, 20, "Waiting for your approval...", ACCENT_GREEN);
    } else if (g_account.state == ACCOUNT_ERROR) {
        DrawCentered(g, rightX + panelW / 2, boxY + box + 50, 18, g_account.error, Color(207, 102, 121));
        DrawCentered(g, rightX + panelW / 2, boxY + box + 80, 18, "Press A to generate a new QR code", TEXT_WHITE);
    }

    // Bottom hint
    DrawControllerFooter(g,44,SCREEN_HEIGHT-40,"A: Retry     B: Back");
    g.Text(1150, SCREEN_HEIGHT - 40, 12, "v0.9", TEXT_DIM);
}

// ============ LOGIN SCREEN ============
static void DrawLoginScreen(NuvioGraphics &g, const NuvioState &state, bool online) {
    DrawGradientBackground(g);
    for (int x = 0; x < 670; x += 4) {
        const int r = 38 - x / 50;
        const int b = 72 - x / 21;
        g.Rect((float)x, 0, 4, SCREEN_HEIGHT, Color(r, 16, b > 18 ? b : 18));
    }
    g.Rect(670, 0, 610, 720, Color(12, 12, 16));
    DrawNuvioLogo(g, 90, 195, 1.55f);
    g.Text(181, 201, 55, "nuvio", TEXT_WHITE);
    g.Text(90, 328, 39, "Watch anything, anywhere", TEXT_WHITE, 560);
    g.Text(90, 397, 22, "Movies and series on Xbox 360.", TEXT_MUTED);

    g.Text(735, 107, 46, "Welcome back", TEXT_WHITE);
    g.Text(735, 182, 22, "Your Nuvio account, directly on Xbox 360.", TEXT_MUTED);
    const float bw = 455, bh = 72;
    const float bx = 735;
    DrawButton(g, bx, 286, bw, bh, "Sign in with QR code", true, true);
    g.Text(735, 396, 21, "Scan and approve on nuvio.tv", TEXT_MUTED);
    if (online) {
        g.Circle(743, 572, 6, ACCENT_GREEN);
        g.Text(759, 559, 17, "Xbox internet connected", ACCENT_GREEN);
    } else {
        const char *networkError = NuvioNativeLastError();
        if (networkError && networkError[0]) {
            g.Text(735, 554, 17, "Network or sign-in connection failed:", ACCENT_GOLD);
            g.Text(735, 582, 16, networkError, TEXT_MUTED, 465);
        } else {
            g.Text(735, 554, 17, "Press A to connect your Nuvio account", TEXT_MUTED);
        }
    }
    g.Text(90, SCREEN_HEIGHT - 36, 13, "NUVIO", TEXT_DIM);
    DrawControllerFooter(g,735,SCREEN_HEIGHT-40,"A: Connect    B: Back");
}

// ============ PROFILE SELECTION ============
static void DrawProfileScreen(NuvioGraphics &g, const NuvioState &state, bool online) {
    DrawGradientBackground(g);
    DrawNuvioLogo(g, SCREEN_WIDTH / 2 - 36, 40, 1.5f);
    DrawCentered(g, SCREEN_WIDTH / 2, 140, 44, "Who's Watching?", TEXT_WHITE);

    const int tiles = state.profileCount + (!state.cloudProfiles && state.profileCount < NUVIO_MAX_PROFILES ? 1 : 0);
    // NuvioTV-style grid: four cards per row accommodates the six cloud profiles.
    const int columns = tiles <= 4 ? tiles : 4;
    const float tileW = 206, gap = 30;
    const float totalW = columns * tileW + (columns - 1) * gap;
    const float startX = (SCREEN_WIDTH - totalW) / 2;
    const float tileY = tiles <= 4 ? 242 : 207;

    for (int i = 0; i < tiles; ++i) {
        const float x = startX + (i % columns) * (tileW + gap);
        const float y = tileY + (i / columns) * 222;
        const float cx = x + tileW / 2;
        const bool focused = state.profileCursor == i;
        if (focused) g.RoundedRect(x - 3, y - 3, tileW + 6, 204, 20, FOCUS_BLUE);
        g.RoundedRect(x, y, tileW, 198, 16, focused ? BG_HOVER : BG_CARD);

        if (i < state.profileCount) {
            if (g.HasAvatar(i)) {
                g.Circle(cx, y + 74, 53, focused ? FOCUS_BLUE : BORDER_SUBTLE);
                g.Avatar(i, cx - 50, y + 24, 100);
            } else {
                DrawProfileAvatar(g, cx, y + 74, 50, ProfileColor(state.profiles[i].color),
                                  state.profiles[i].name, i == 0);
            }
            DrawCentered(g, cx, y + 140, 23, state.profiles[i].name, TEXT_WHITE);
            if (state.profiles[i].locked) DrawCentered(g, cx, y + 170, 15, "PIN protected", TEXT_MUTED);
            else if (i == 0) DrawCentered(g, cx, y + 170, 15, "Primary profile", TEXT_MUTED);
        } else {
            g.Circle(cx, y + 74, 50, BORDER_SUBTLE);
            g.Circle(cx, y + 74, 46, BG_DARK);
            DrawCentered(g, cx, y + 40, 58, "+", TEXT_MUTED);
            DrawCentered(g, cx, y + 140, 22, "Add local profile", TEXT_MUTED);
        }
    }

    DrawStatusLine(g, 624);
    DrawControllerFooter(g,40,SCREEN_HEIGHT-40,"Left/Right: Choose    A: Select    X: Remove profile    B: Back");
}

// ============ VIRTUAL KEYBOARD ============
static void DrawTextBox(NuvioGraphics &g, float x, float y, float w, float h,
                        const char *text, const char *placeholder) {
    g.RoundedRect(x - 2, y - 2, w + 4, h + 4, 12, FOCUS_BLUE);
    g.RoundedRect(x, y, w, h, 10, BG_HOVER);
    const float size = 24;
    const int maxChars = (int)((w - 50) / (size * 0.52f));
    const int len = (int)std::strlen(text);
    if (len == 0) {
        g.Text(x + 20, y + h / 2 - 15, size, placeholder, TEXT_DIM);
        if ((GetTickCount() / 500) % 2 == 0) g.Rect(x + 20, y + 10, 2, h - 20, TEXT_WHITE);
        return;
    }
    const char *shown = text;
    if (len > maxChars) shown = text + (len - maxChars);
    g.Text(x + 20, y + h / 2 - 15, size, shown, TEXT_WHITE);
    if ((GetTickCount() / 500) % 2 == 0) {
        const float cx = x + 20 + (float)std::strlen(shown) * size * 0.52f;
        g.Rect(cx, y + 10, 2, h - 20, TEXT_WHITE);
    }
}

static void DrawKeyboard(NuvioGraphics &g, const NuvioState &s, float y0) {
    const float keyW = 78, keyH = 52, gap = 8;
    const float totalW = NUVIO_KEY_COLS * keyW + (NUVIO_KEY_COLS - 1) * gap;
    const float x0 = (SCREEN_WIDTH - totalW) / 2;

    for (int r = 0; r < NUVIO_KEY_ROWS; ++r) {
        for (int c = 0; c < NUVIO_KEY_COLS; ++c) {
            const float x = x0 + c * (keyW + gap);
            const float y = y0 + r * (keyH + gap);
            const bool focused = s.keyboardRow == r && s.keyboardCol == c;
            if (focused) g.RoundedRect(x - 2, y - 2, keyW + 4, keyH + 4, 10, FOCUS_BLUE);
            g.RoundedRect(x, y, keyW, keyH, 8, focused ? BG_HOVER : BG_CARD);
            char label[2];
            label[0] = NuvioKeyboardKeyChar(&s, r, c);
            label[1] = '\0';
            DrawCentered(g, x + keyW / 2, y + 9, 28, label, TEXT_WHITE);
        }
    }

    const int n = NuvioKeyboardActionCount(&s);
    const float actionW = (totalW - (n - 1) * gap) / n;
    const float ay = y0 + NUVIO_KEY_ROWS * (keyH + gap);
    for (int i = 0; i < n; ++i) {
        const float x = x0 + i * (actionW + gap);
        const char *label = NuvioKeyboardActionLabel(&s, i);
        const bool focused = s.keyboardRow == NUVIO_KEY_ROWS && s.keyboardCol == i;
        D3DCOLOR bg = focused ? BG_HOVER : BG_CARD;
        if (!focused && s.keyboardShift && std::strcmp(label, "Shift") == 0) bg = Color(120, 90, 0);
        if (focused) g.RoundedRect(x - 2, ay - 2, actionW + 4, keyH + 4, 10, FOCUS_BLUE);
        g.RoundedRect(x, ay, actionW, keyH, 8, bg);
        DrawCentered(g, x + actionW / 2, ay + 13, 20, label, TEXT_WHITE);
    }
}

// ============ CREATE PROFILE ============
static void DrawCreateProfileScreen(NuvioGraphics &g, const NuvioState &state) {
    DrawGradientBackground(g);
    DrawCentered(g, SCREEN_WIDTH / 2, 28, 40, "Create Profile", TEXT_WHITE);

    DrawTextBox(g, 330, 100, 620, 56, state.textBuffer, "Profile name");

    // Avatar colour row
    g.Text(330, 178, 18, "Avatar colour", TEXT_MUTED);
    for (int i = 0; i < NUVIO_PROFILE_COLORS; ++i) {
        const float cx = 560 + i * 70;
        const bool picked = state.newProfileColor == i;
        const bool focused = state.keyboardRow == -1 && picked;
        if (focused) g.Circle(cx, 200, 28, TEXT_WHITE);
        else if (picked) g.Circle(cx, 200, 26, TEXT_MUTED);
        g.Circle(cx, 200, 22, ProfileColor(i));
    }
    char init[2];
    InitialOf(state.textBuffer, init);
    DrawProfileAvatar(g, 460, 205, 34, ProfileColor(state.newProfileColor),
                      state.textBuffer[0] ? state.textBuffer : "?", false);

    DrawKeyboard(g, state, 262);
    DrawStatusLine(g, 645);
    DrawControllerFooter(g,110,SCREEN_HEIGHT-36,"A: Type    X: Delete    Y: Space    LB/RB: Shift    Start: Create    B: Cancel");
}

// ============ SIDEBAR ============
static void DrawSidebar(NuvioGraphics &g, const NuvioState &state) {
    const bool expanded=state.sidebarFocused;
    const float width=expanded?260.0f:88.0f;
    g.Rect(0,0,width,720,Color(13,13,13));
    if(g.HasAvatar(state.profileIndex))g.Avatar(state.profileIndex,24,24,40);
    else {g.Circle(44,44,20,Color(65,65,65));g.Text(35,29,24,"N",TEXT_WHITE);}
    if(expanded)g.Text(82,31,22,state.profileName,TEXT_WHITE,170);
    const char *labels[]={"Inicio","Filmes e series","Busca","Biblioteca","Configuracoes"};
    for(int i=0;i<5;++i) {
        const float y=222.0f+i*76.0f;
        const bool active=state.sidebarItem==i;
        if(active)g.RoundedRect(14,y-10,expanded?232.0f:60.0f,60,16,expanded?TEXT_WHITE:Color(35,35,35));
        g.Icon(i,24,y,40,active&&expanded?BG_DARK:active?TEXT_WHITE:TEXT_DIM);
        if(expanded)g.Text(84,y+7,21,labels[i],active?BG_DARK:TEXT_MUTED,165);
    }
}

// ============ USER BADGE (Top Right) ============
static void DrawUserBadge(NuvioGraphics &, const NuvioState &, bool) {}

// ============ POSTER CARD ============
static void DrawWrapped(NuvioGraphics &g,float x,float y,float size,const char *text,int columns,int lines) {
    if(!text)return;
    while(*text && lines-->0) {
        char line[180];int n=0,lastSpace=-1;
        while(text[n] && text[n]!='\n' && n<columns && n<178) {if(text[n]==' ')lastSpace=n;++n;}
        if(text[n] && text[n]!='\n' && lastSpace>0)n=lastSpace;
        std::memcpy(line,text,n);line[n]=0;g.Text(x,y,size,line,TEXT_MUTED);
        text+=n;while(*text==' '||*text=='\n')++text;y+=size*1.3f;
    }
}
static void DrawPosterCard(NuvioGraphics &g,const NuvioCatalog &catalog,int index,
                          float x,float y,bool focused,bool) {
    if(index<0||index>=catalog.count)return;
    const NuvioCatalogItem &item=catalog.items[index];
    const float w=180,h=240;
    if(focused)g.Rect(x-3,y-3,w+6,h+6,TEXT_WHITE);
    if(g.HasPoster(index))g.Poster(index,x,y,w,h);
    else {g.Rect(x,y,w,h,Color(28,28,28));g.Text(x+14,y+104,14,"Nuvio",TEXT_DIM);}
    g.Text(x,y+h+8,17,item.title,TEXT_WHITE,w);
    if(item.year){char year[12];std::sprintf(year,"%d",item.year);g.Text(x,y+h+31,14,year,TEXT_DIM);}
}
static void DrawHeroBanner(NuvioGraphics &g,const NuvioCatalog &catalog,int index) {
    if(index<0||index>=catalog.count)return;
    const NuvioCatalogItem &item=catalog.items[index];
    if(g.HasBackdrop()&&g_previewIndex==index)g.Backdrop(88,0,1192,365);
    else g.Rect(88,0,1192,365,Color(17,17,17));
    for(int i=0;i<16;++i)g.Rect(88+i*64.0f,0,64,365,Color(13,13,13,220-i*11));
    for(int i=0;i<8;++i)g.Rect(88,285+i*10.0f,1192,10,Color(13,13,13,35+i*28));
    g.Text(112,100,38,item.title,TEXT_WHITE,1010);
    char meta[160];std::sprintf(meta,"%s  |  %d  %s  %s",item.type=='S'?"Series":"Filme",item.year,
        g_previewIndex==index?g_preview.runtime:"",g_previewIndex==index?g_preview.genres:"");
    g.Text(112,158,17,meta,TEXT_MUTED,1000);
    if(g_previewIndex==index)DrawWrapped(g,112,196,18,g_preview.description,90,3);
    g.RoundedRect(112,292,158,40,20,TEXT_WHITE);g.Text(130,301,18,"A  Ver detalhes",BG_DARK);
}

// ============ ROW SECTION ============
static void DrawRowSection(NuvioGraphics &g, const NuvioCatalog &catalog, const NuvioState &state,
                          int row, const char *title, int startIdx, int count, float startY) {
    const float startX = 112;
    const float cardW = 180;
    const float gap = 20;

    g.Text(startX, startY - 31, 22, title, TEXT_WHITE);

    const int first = state.selectedRow == row ? (state.selectedColumn / 5) * 5 : 0;
    for (int i = 0; i < 5 && first + i < count; i++) {
        int catalogIdx = startIdx + first + i;
        if (catalogIdx >= catalog.count) break;

        float x = startX + i * (cardW + gap);
        float y = startY;

        bool isSelected = (!state.sidebarFocused &&
                          state.selectedRow == row &&
                          state.selectedColumn == first + i);
        bool isSeries = catalog.items[catalogIdx].type == 'S';

        DrawPosterCard(g, catalog, catalogIdx, x, y, isSelected, isSeries);
    }
}

// ============ HOME SCREEN ============
static void DrawHomeScreen(NuvioGraphics &g,const NuvioState &state,const NuvioCatalog &catalog,bool online) {
    g.Rect(0,0,1280,720,BG_DARK);
    if(!catalog.railCount) {
        g.Text(112,120,30,g_catalogLoading?"Carregando catalogo...":"Nenhum titulo disponivel",TEXT_WHITE);
        DrawStatusLine(g,680);return;
    }
    const int row=state.selectedRow<catalog.railCount?state.selectedRow:0;
    const NuvioCatalogRail &rail=catalog.rails[row];
    const int hero=NuvioCatalogIndexForRow(&catalog,row,state.selectedColumn);
    if(row==0)DrawHeroBanner(g,catalog,hero);
    else g.Text(112,32,30,"Inicio",TEXT_WHITE);
    DrawRowSection(g,catalog,state,row,rail.name,rail.start,rail.count,row==0?407:115);
    if(row>0&&row+1<catalog.railCount) {
        const NuvioCatalogRail &next=catalog.rails[row+1];
        DrawRowSection(g,catalog,state,row+1,next.name,next.start,next.count,455);
    }
    DrawStatusLine(g,680);
}
static int DiscoverItem(const NuvioState &state,const NuvioCatalog &catalog,int ordinal) {
    for(int i=0;i<catalog.count;++i) {
        if(state.catalogMode==NUVIO_CATALOG_TRENDING && catalog.items[i].type!='M')continue;
        if(state.catalogMode==NUVIO_CATALOG_POPULAR && catalog.items[i].type!='S')continue;
        if(ordinal--==0)return i;
    }
    return -1;
}
static int DiscoverCount(const NuvioState &state,const NuvioCatalog &catalog) {
    int n=0;while(DiscoverItem(state,catalog,n)>=0)++n;return n;
}
static void DrawDiscoverScreen(NuvioGraphics &g,const NuvioState &state,const NuvioCatalog &catalog,bool) {
    g.Rect(0,0,1280,720,BG_DARK);
    g.Text(112,24,32,"Filmes e series",TEXT_WHITE);
    const char *filters[3]={"Filmes","Series","Todos"};
    for(int i=0;i<3;++i) {g.RoundedRect(112+i*146.0f,82,132,36,10,state.catalogMode==i?TEXT_WHITE:BG_CARD);
        g.Text(128+i*146.0f,89,17,filters[i],state.catalogMode==i?BG_DARK:TEXT_MUTED);}
    DrawControllerFooter(g,550,87,"Y: trocar filtro    Start: buscar");
    const int top=(state.selectedRow/2)*2;
    for(int r=0;r<2;++r)for(int c=0;c<5;++c) {
        int index=DiscoverItem(state,catalog,(top+r)*5+c);
        if(index>=0)DrawPosterCard(g,catalog,index,112+c*200.0f,160+r*275.0f,
            !state.sidebarFocused&&state.selectedRow==top+r&&state.selectedColumn==c,false);
    }
    DrawStatusLine(g,680);
}

// Keep artwork and selection in sync for both Home and Discover.
static int FocusedMovieIndex(const NuvioState &state, const NuvioCatalog &catalog) {
    if (state.sidebarFocused || (state.screen != NUVIO_SCREEN_HOME &&
        state.screen != NUVIO_SCREEN_DISCOVER)) return -1;
    if(state.screen==NUVIO_SCREEN_DISCOVER)return DiscoverItem(state,catalog,state.selectedRow*5+state.selectedColumn);
    return NuvioCatalogIndexForRow(&catalog, state.selectedRow, state.selectedColumn);
}

static void DrawPreviewText(NuvioGraphics &g, float x, float y, const char *source) {
    // Word wrapping is handled on the console, not by the bitmap font atlas.
    const char *cursor = source;
    for (int lineNo = 0; lineNo < 2 && *cursor; ++lineNo) {
        int length = (int)std::strlen(cursor);
        if (length > 52) {
            length = 52;
            while (length > 25 && cursor[length] != ' ') --length;
        }
        char line[64];
        std::memcpy(line, cursor, length);
        line[length] = '\0';
        g.Text(x, y + 23 * lineNo, 17, line, TEXT_WHITE, 425);
        cursor += length;
        while (*cursor == ' ') ++cursor;
    }
}

static void DrawFocusPreview(NuvioGraphics &g, const NuvioState &state,
                             const NuvioCatalog &catalog) {
    const int index = FocusedMovieIndex(state, catalog);
    if (index < 0 || index != g_previewIndex) return;
    const NuvioCatalogItem &item = catalog.items[index];
    const float w = 465, h = 404;
    float x = 90.0f + state.selectedColumn * 200.0f - 12;
    if (x + w > SCREEN_WIDTH - 20) x = SCREEN_WIDTH - 20 - w;
    const float y = state.screen == NUVIO_SCREEN_HOME && state.selectedRow == 0 ? 256 : 192;

    g.RoundedRect(x + 9, y + 11, w, h, 18, Color(0, 0, 0, 230));
    g.RoundedRect(x - 2, y - 2, w + 4, h + 4, 17, Color(78, 78, 86));
    g.RoundedRect(x, y, w, h, 15, Color(20, 20, 24));
    if (g.HasBackdrop()) {
        g.Backdrop(x + 3, y + 3, w - 6, 202);
        g.Rect(x + 3, y + 140, w - 6, 65, Color(0, 0, 0, 105));
    } else {
        g.RoundedRect(x + 3, y + 3, w - 6, 202, 13, Color(24, 35, 50));
        g.Text(x + 25, y + 76, 23, item.title, TEXT_WHITE, w - 50);
    }
    g.Text(x + 20, y + 214, 24, item.title, TEXT_WHITE, w - 35);

    char line[125];
    std::sprintf(line, "%s  |  %d  |  IMDb %s", item.type == 'S' ? "Series" : "Movie",
                 item.year, g_preview.rating[0] ? g_preview.rating : "--");
    g.Text(x + 20, y + 247, 17, line, TEXT_MUTED, w - 30);
    g.Text(x + 20, y + 275, 17, g_preview.genres[0] ? g_preview.genres : "Discover this title",
           TEXT_MUTED, w - 30);
    DrawPreviewText(g, x + 20, y + 303, g_preview.description[0]
                    ? g_preview.description : "Press A to learn more about this title.");

    g.RoundedRect(x + 20, y + h - 48, 290, 36, 12, TEXT_WHITE);
    DrawCentered(g, x + 165, y + h - 45, 18, "A  View details", BG_DARK);
    g.Circle(x + w - 46, y + h - 30, 17, Color(55, 55, 62));
    g.Text(x + w - 53, y + h - 43, 24, "+", TEXT_WHITE);
}

// ============ SEARCH SCREEN ============
static void DrawSearchScreen(NuvioGraphics &g, const NuvioState &state, bool online) {
    DrawGradientBackground(g);
    DrawUserBadge(g, state, online);

    g.Text(110, 40, 36, "Search", TEXT_WHITE);

    float boxX = 110;
    float boxY = 110;
    float boxW = 600;
    float boxH = 56;

    g.RoundedRect(boxX, boxY, boxW, boxH, 10, state.searchActive ? BG_HOVER : BG_CARD);
    if (state.searchActive) {
        g.RoundedRect(boxX - 2, boxY - 2, boxW + 4, boxH + 4, 12, FOCUS_BLUE);
        g.RoundedRect(boxX, boxY, boxW, boxH, 10, BG_HOVER);
    }

    g.Icon(2, boxX + 16, boxY + 4, 48, state.searchActive ? TEXT_WHITE : TEXT_MUTED);

    if (state.searchQuery[0]) {
        g.Text(boxX + 72, boxY + 12, 22, state.searchQuery, TEXT_WHITE);
        if (state.searchActive && (GetTickCount() / 500) % 2 == 0) {
            int textW = (int)std::strlen(state.searchQuery) * 14;
            g.Rect(boxX + 72 + textW, boxY + 14, 2, 28, TEXT_WHITE);
        }
    } else {
        g.Text(boxX + 72, boxY + 12, 22, "Search movies, series...", TEXT_DIM);
    }

    if (state.searchActive) DrawKeyboard(g, state, 205);
    else if (g_catalog.railCount) {
        const int r = state.selectedRow < g_catalog.railCount ? state.selectedRow : 0;
        const NuvioCatalogRail &rail = g_catalog.rails[r];
        DrawRowSection(g, g_catalog, state, r, rail.name, rail.start, rail.count, 245);
    } else g.Text(110, 220, 24, g_catalogLoading ? "Searching addons..." : "No search results", TEXT_MUTED);
    DrawStatusLine(g, 642);
    DrawControllerFooter(g,110,681,state.searchActive ? "A: Key   X: Delete   Y: Space   Start: Search   B: Back" : "A: Details   B: Edit search   X: Refresh");
}

// ============ LIBRARY SCREEN ============
static void DrawLibraryScreen(NuvioGraphics &g, const NuvioState &state, bool online) {
    DrawGradientBackground(g);
    DrawUserBadge(g, state, online);

    g.Text(110, 40, 36, "My Library", TEXT_WHITE);
    g.Text(110, 90, 18, "Your saved movies and series", TEXT_MUTED);

    if (g_catalog.railCount) {
        const int r = state.selectedRow < g_catalog.railCount ? state.selectedRow : 0;
        const NuvioCatalogRail &rail = g_catalog.rails[r];
        DrawRowSection(g, g_catalog, state, r, rail.name, rail.start, rail.count, 200);
    } else g.Text(110, 180, 24, g_catalogLoading ? "Loading library..." : "No items saved to this profile", TEXT_MUTED);
    DrawStatusLine(g, 642);
    DrawControllerFooter(g,110,681,"A: Details   X: Refresh   B: Profiles");
}

// ============ SETTINGS SCREEN ============
static void DrawMenuItem(NuvioGraphics &g, float x, float y, float w, const char *title,
                         const char *subtitle, bool focused) {
    if (focused) g.RoundedRect(x - 3, y - 3, w + 6, 78, 15, FOCUS_BLUE);
    g.RoundedRect(x, y, w, 72, 12, focused ? BG_HOVER : BG_CARD);
    g.Text(x + 24, y + 10, 24, title, TEXT_WHITE);
    g.Text(x + 24, y + 42, 16, subtitle, TEXT_MUTED, w - 80);
    g.Text(x + w - 40, y + 16, 32, ">", focused ? TEXT_WHITE : TEXT_DIM);
}

static void DrawSettingsScreen(NuvioGraphics &g, const NuvioState &state, bool online) {
    DrawGradientBackground(g);
    DrawUserBadge(g, state, online);

    g.Text(110, 40, 36, "Settings", TEXT_WHITE);

    // Profile card
    g.RoundedRect(110, 110, 520, 140, 12, BG_CARD);
    const NuvioProfile &p = state.profiles[state.profileIndex];
    DrawProfileAvatar(g, 180, 180, 40, ProfileColor(p.color), state.profileName, state.profileIndex == 0);
    g.Text(250, 150, 26, state.profileName, TEXT_WHITE);
    g.Text(250, 188, 16, state.profileIndex == 0 ? "Primary profile" : "Profile", TEXT_MUTED);
    if (online) g.Text(250, 214, 16, "Nuvio cloud: connected", ACCENT_GREEN);
    else g.Text(250, 214, 16, "Nuvio cloud: awaiting catalog", Color(200, 180, 0));

    // Menu
    char sub[96];
    std::sprintf(sub, "%d installed, %d enabled%s", g_addons.count,
                 NuvioAddonsEnabledCount(&g_addons),
                 g_addons.fromCloud ? "" : "  (not synchronized)");
    const bool menuActive = !state.sidebarFocused;
    DrawMenuItem(g, 110, 260, 520, "Addons", sub, menuActive && state.selectedRow == 0);
    DrawMenuItem(g, 110, 338, 520, "Switch profile", state.profileName, menuActive && state.selectedRow == 1);
    DrawMenuItem(g, 110, 416, 520, "Streaming P2P",
                 g_p2pEnabled?"Torrent streams enabled":"Torrent streams disabled",menuActive && state.selectedRow == 2);
    DrawMenuItem(g, 110, 494, 520, "Sync account", "Reload cloud addons and catalog",menuActive && state.selectedRow == 3);
    DrawMenuItem(g, 110, 572, 520, "Sign out", "Erase account session",menuActive && state.selectedRow == 4);

    // Application card
    g.RoundedRect(680, 110, 490, 240, 12, BG_CARD);
    g.Text(704, 126, 22, "Application", TEXT_WHITE);
    g.Text(704, 164, 16, "NUVIO 0.1.0-alpha.2", TEXT_WHITE);
    g.Text(704, 192, 16, "Xbox 360 native streaming interface", TEXT_MUTED);
    g.Text(704, 220, 16, "Catalog: Nuvio and enabled cloud addons", TEXT_MUTED);
    g.Text(704, 248, 16, "Artwork: original addon metadata", TEXT_MUTED);
    g.Text(704, 276, 16, "Network: Xbox-native TLS / direct internet", TEXT_MUTED);
    g.Text(704, 316, 14, "Player nativo FFmpeg / XAudio2 (ate 720p)", TEXT_DIM);

    DrawStatusLine(g, 646);
    DrawControllerFooter(g,110,SCREEN_HEIGHT-36,"Up/Down: Choose    A: Open    B: Home    Y: Sign out");
}

// ============ STREAMING P2P SETTINGS ============
static void DrawP2PSettingsScreen(NuvioGraphics &g,const NuvioState &state,bool online) {
    DrawGradientBackground(g);
    DrawUserBadge(g,state,online);
    g.Text(110,42,38,"Streaming P2P",TEXT_WHITE);
    g.Text(110,98,18,"Configuracoes de torrent salvas no proprio Xbox 360",TEXT_MUTED);
    const float x=110,w=1060;
    const char *titles[]={"Streaming P2P","Ocultar estatisticas de torrent","Limpar cache de torrent"};
    const char *subtitles[]={
        "Permitir streams ponto a ponto (torrent), sem PC",
        "Esconder detalhes tecnicos durante o carregamento",
        "Apagar metadados salvos (max. 1 MiB); buffer de video temporario"
    };
    for(int i=0;i<3;++i) {
        float y=168+i*136;
        bool active=state.selectedRow==i;
        if(active)g.RoundedRect(x-3,y-3,w+6,119,16,FOCUS_BLUE);
        g.RoundedRect(x,y,w,113,13,active?BG_HOVER:BG_CARD);
        g.Text(x+28,y+22,26,titles[i],TEXT_WHITE);
        g.Text(x+28,y+64,17,subtitles[i],TEXT_MUTED,w-220);
        if(i<2) {
            bool enabled=i==0?g_p2pEnabled:g_hideTorrentStats;
            g.RoundedRect(x+w-132,y+35,92,48,24,enabled?ACCENT_GREEN:BG_SIDEBAR);
            g.RoundedRect(x+w-127+(enabled?44:0),y+41,36,36,18,TEXT_WHITE);
        } else g.Text(x+w-88,y+37,26,"A",TEXT_WHITE);
    }
    DrawStatusLine(g,638);
    DrawControllerFooter(g,110,681,"Up/Down: escolher    A: alternar/limpar    B: voltar");
}

// ============ PLUGINS SCREEN ============
static void DrawAddonsScreen(NuvioGraphics &g, const NuvioState &state, bool online) {
    DrawGradientBackground(g);
    DrawUserBadge(g, state, online);

    g.Text(110, 36, 36, "Addons", TEXT_WHITE);
    g.Text(110, 84, 18, "Nuvio cloud addons. A: enable/disable. Y: synchronize.", TEXT_MUTED);

    const int total = g_addons.count + 1; // last row = install new plugin
    const int visible = 6;
    int top = 0;
    if (state.selectedRow >= visible) top = state.selectedRow - visible + 1;

    const float rowH = 74;
    for (int n = 0; n < visible; ++n) {
        const int i = top + n;
        if (i >= total) break;
        const float y = 130 + n * (rowH + 8);
        const bool focused = state.selectedRow == i;
        if (focused) g.RoundedRect(104, y - 3, SCREEN_WIDTH - 208, rowH + 6, 14, FOCUS_BLUE);
        g.RoundedRect(108, y, SCREEN_WIDTH - 216, rowH, 12, focused ? BG_HOVER : BG_CARD);

        if (i == g_addons.count) {
            g.Text(140, y + 20, 26, "+  Install plugin from URL", focused ? TEXT_WHITE : TEXT_MUTED);
            continue;
        }
        const NuvioAddon &a = g_addons.items[i];
        const bool on = a.enabled != 0;
        g.RoundedRect(130, y + 20, 70, 34, 17, on ? Color(0, 140, 60) : Color(70, 70, 80));
        g.Circle(on ? 183.0f : 147.0f, y + 37, 12, TEXT_WHITE);
        g.Text(224, y + 8, 24, a.name, TEXT_WHITE, 380);
        g.Text(224, y + 40, 15, a.desc, TEXT_MUTED, 520);
        g.Text(760, y + 8, 18, on ? "ON" : "OFF", on ? ACCENT_GREEN : TEXT_DIM);
        char shortUrl[56];
        std::strncpy(shortUrl, a.url, 52);
        shortUrl[52] = '\0';
        if (std::strlen(a.url) > 52) std::strcpy(shortUrl + 49, "...");
        g.Text(760, y + 40, 14, "Configured via Nuvio cloud", TEXT_DIM, SCREEN_WIDTH - 790);
    }

    if (total > visible) {
        char pos[24];
        std::sprintf(pos, "%d / %d", state.selectedRow + 1, total);
        g.Text(SCREEN_WIDTH - 200, 90, 16, pos, TEXT_DIM);
    }

    if (!g_addons.fromCloud)
        g.Text(110, 612, 18, "Not synchronized. Y: retry cloud sync.", Color(200, 180, 0));
    DrawStatusLine(g, 640);
    DrawControllerFooter(g,110,SCREEN_HEIGHT-36,"Up/Down: Choose    A: On/Off or Install    X: Remove    B: Back");
}

// ============ INSTALL PLUGIN (URL) ============
static void DrawAddAddonScreen(NuvioGraphics &g, const NuvioState &state) {
    DrawGradientBackground(g);
    DrawCentered(g, SCREEN_WIDTH / 2, 24, 38, "Install Plugin", TEXT_WHITE);
    DrawCentered(g, SCREEN_WIDTH / 2, 72, 18, "Type the add-on manifest URL (use the https:// and /manifest.json keys)", TEXT_MUTED);

    DrawTextBox(g, 110, 108, SCREEN_WIDTH - 220, 56, state.textBuffer, "https://...");
    DrawKeyboard(g, state, 200);
    DrawStatusLine(g, 612);
    DrawControllerFooter(g,110,SCREEN_HEIGHT-36,"A: Type    X: Delete    Y: Space    LB/RB: Shift    Start: Install    B: Cancel");
}

// ============ DETAILS SCREEN ============
static void DrawDetailsScreen(NuvioGraphics &g,const NuvioCatalog &catalog,int index) {
    g.Rect(0,0,1280,720,BG_DARK);
    if(index<0||index>=catalog.count)return;
    const NuvioCatalogItem &item=catalog.items[index];
    if(g.HasBackdrop()&&g_previewIndex==index)g.Backdrop(0,0,1280,720);
    for(int i=0;i<16;++i)g.Rect(i*80.0f,0,80,720,Color(13,13,13,225-i*9));
    g.Rect(0,590,1280,130,Color(13,13,13,190));
    g.Text(30,24,17,"B  Voltar",TEXT_WHITE);
    g.Text(30,180,42,item.title,TEXT_WHITE,1100);
    char meta[130];std::sprintf(meta,"%d  %s  IMDb %s",item.year,g_previewIndex==index?g_preview.runtime:"",
        g_previewIndex==index&&g_preview.rating[0]?g_preview.rating:"--");
    g.Text(30,252,18,meta,TEXT_WHITE);
    g.Text(30,286,17,g_previewIndex==index?g_preview.genres:"",TEXT_MUTED,950);
    if(g_previewIndex==index)DrawWrapped(g,30,326,19,g_preview.description,84,4);
    else g.Text(30,326,18,"Carregando informacoes...",TEXT_DIM);
    g.RoundedRect(30,455,380,44,22,TEXT_WHITE);
    g.Text(65,465,21,item.type=='S'?"A  Escolher episodio":"A  Reproduzir",BG_DARK);
    g.RoundedRect(425,455,300,44,22,Color(38,38,38));
    g.Text(447,467,17,"X  Salvar na biblioteca",TEXT_WHITE);
    DrawStatusLine(g,680);
}

// Addon stream discovery and series episode selection.
static char g_sourceRows[240][160];
static int g_sourceCount = 0, g_sourceCursor = 0, g_sourceEpisode = -1;
static bool g_episodePicker = false;
static char g_sourceId[128];
static bool ReadSourceRows(const char *path,bool episodes) {
    g_sourceCount=g_sourceCursor=0;g_episodePicker=episodes;
    return BeginRenderRequest(episodes?6:5,0,path,60000);
}
static bool LoadSourceRows(bool episodes) {
    char encoded[400], path[512];
    NuvioUrlEncode(encoded,sizeof(encoded),g_sourceId);
    if (episodes) std::sprintf(path,"/videos?id=%s",encoded);
    else std::sprintf(path,"/sources?id=%s&episode=%d",encoded,g_sourceEpisode);
    return ReadSourceRows(path,episodes);
}
static void DrawSourcesScreen(NuvioGraphics &g) {
    DrawGradientBackground(g);
    g.Text(110,40,36,g_episodePicker ? "Choose an episode" : "Addon sources",TEXT_WHITE);
    if (!g_sourceCount) g.Text(110,140,23,g_artThread?"Carregando episodios/fontes...":g_episodePicker?"Nenhum episodio disponivel.":"Nenhuma fonte retornada.",TEXT_MUTED);
    const int top = g_sourceCursor >= 6 ? g_sourceCursor-5 : 0;
    for (int n=0;n<6 && top+n<g_sourceCount;++n) {
        int i=top+n;
        g.RoundedRect(110,120+n*72,1050,62,10,i==g_sourceCursor ? BG_HOVER : BG_CARD);
        if (i==g_sourceCursor) g.Rect(110,120+n*72,4,62,TEXT_WHITE);
        const char *label=g_sourceRows[i];
        char country[3]="--";
        if(label[0]=='['&&label[3]==']'&&label[4]==' ') {
            country[0]=label[1];country[1]=label[2];label+=5;
        }
        if(!g_episodePicker)DrawLanguageFlag(g,130,136+n*72,country);
        g.Text(g_episodePicker?130:200,136+n*72,18,label,TEXT_WHITE,g_episodePicker?1000:920);
    }
    g.Text(110,590,17,"Idioma informado pelo addon; -- = idioma nao identificado.",TEXT_MUTED);
    if (!g_episodePicker) g.Text(110,620,17,"MP4/MKV/AVI podem tocar com codec disponivel no Xbox (ate 720p).",TEXT_MUTED);
    DrawStatusLine(g,650);
    DrawControllerFooter(g,110,681,"Up/Down: Choose   A: Open   X: Refresh   B: Back");
}

// ============ MAIN DRAW FUNCTION ============
static IDirect3DDevice9 *g_videoDevice=NULL;
static void DrawScreen(NuvioGraphics &g, const NuvioState &state,
                       const NuvioCatalog &catalog, bool online) {
    g.Begin();

    if(state.screen==NUVIO_SCREEN_PLAYER&&g_player) {
        g_player->Update();
        g_videoDevice->SetRenderState(D3DRS_VIEWPORTENABLE,TRUE);
        D3DRECT screen={0,0,1280,720};g_player->RenderFrame(screen);
        g.ResumeUI();
        if(g_player->IsBuffering()&&!g_player->Failed()) {
            std::wstring wide=g_player->StatusText();std::string status(wide.begin(),wide.end());
            if(g_playingTorrent&&g_hideTorrentStats)status="Carregando video P2P...";
            const int percent=g_player->StartupPercent();
            g.RoundedRect(242,258,796,168,19,Color(12,12,12,230));
            g.Text(282,284,28,"Preparando video",TEXT_WHITE);
            char amount[24];std::sprintf(amount,"%d%%",percent);
            g.Text(940,286,24,amount,ACCENT_GREEN);
            g.RoundedRect(282,344,718,14,7,Color(65,65,65));
            if(percent>0)g.RoundedRect(282,344,718.0f*percent/100.0f,14,7,ACCENT_GREEN);
            g.Text(282,374,18,status.c_str(),TEXT_MUTED,716);
            g.Text(282,402,14,"Buffer inicial; o torrent continua em streaming.",TEXT_DIM);
        }
        if(g_player->Failed())g.Text(40,90,18,g_player->Error().c_str(),TEXT_WHITE,1140);
        if(GetTickCount()-g_playerInputAt<4000||g_player->IsPaused()||g_player->Failed()) {
            g.Rect(0,655,1280,65,Color(0,0,0,210));
            char time[64];std::sprintf(time,"%d:%02d / %d:%02d",
                (int)g_player->Position()/60,(int)g_player->Position()%60,(int)g_player->Duration()/60,(int)g_player->Duration()%60);
            g.Text(40,678,21,time,TEXT_WHITE,280);
            DrawControllerFooter(g,365,675,g_player->IsPaused()?"A: Continuar    B: Voltar":"A: Pausar    B: Voltar");
            g.Text(755,679,16,"Left/Right: 10s",TEXT_DIM);
        }
        g.End();return;
    }
    switch (state.screen) {
        case NUVIO_SCREEN_LOGIN:          DrawLoginScreen(g, state, online); break;
        case NUVIO_SCREEN_PROFILE:        DrawProfileScreen(g, state, online); break;
        case NUVIO_SCREEN_CREATE_PROFILE: DrawCreateProfileScreen(g, state); break;
        case NUVIO_SCREEN_QR_LOGIN:       DrawQrLoginScreen(g, state); break;
        case NUVIO_SCREEN_HOME:           DrawHomeScreen(g, state, catalog, online); break;
        case NUVIO_SCREEN_DISCOVER:       DrawDiscoverScreen(g, state, catalog, online); break;
        case NUVIO_SCREEN_SEARCH:
        case NUVIO_SCREEN_SEARCH_RESULTS: DrawSearchScreen(g, state, online); break;
        case NUVIO_SCREEN_LIBRARY:        DrawLibraryScreen(g, state, online); break;
        case NUVIO_SCREEN_SETTINGS:       DrawSettingsScreen(g, state, online); break;
        case NUVIO_SCREEN_P2P_SETTINGS:   DrawP2PSettingsScreen(g, state, online); break;
        case NUVIO_SCREEN_SOURCES:        DrawSourcesScreen(g); break;
        case NUVIO_SCREEN_ADDONS:         DrawAddonsScreen(g, state, online); break;
        case NUVIO_SCREEN_ADD_ADDON:      DrawAddAddonScreen(g, state); break;
        case NUVIO_SCREEN_DETAILS:        DrawDetailsScreen(g, catalog, state.selectedCatalogIndex); break;
    }

    if(state.screen==NUVIO_SCREEN_HOME||state.screen==NUVIO_SCREEN_DISCOVER||
       state.screen==NUVIO_SCREEN_SEARCH||state.screen==NUVIO_SCREEN_SEARCH_RESULTS||
       state.screen==NUVIO_SCREEN_LIBRARY||state.screen==NUVIO_SCREEN_SETTINGS)DrawSidebar(g,state);
    g.End();
}

// ============ BRIDGE CONNECTION ============
static int g_nextPoster=0;
static int g_artKind=0,g_artIndex=-1,g_artLimit=0;
static char g_artPath[1024];
static NuvioHttpData g_artReply={NULL,0,0};
static bool g_artOk=false,g_mutationOk=false;
static DWORD g_artRetryAt[5][NUVIO_MAX_CATALOG_ITEMS]={0};
static DWORD WINAPI RenderRequestWorker(void*) {
    g_artOk=NuvioHttpGet("native",0,g_artPath,g_artLimit,&g_artReply);
    if(g_artKind==8) {
        g_mutationOk=g_artOk;
        NuvioHttpFree(&g_artReply);
        if(g_artOk)g_artOk=NuvioHttpGet("native",0,"/addons",180000,&g_artReply);
    }
    return 0;
}
static bool BeginRenderRequest(int kind,int index,const char *path,int limit) {
    if(g_artThread||g_catalogLoading)return false;
    g_artKind=kind;g_artIndex=index;g_artLimit=limit;g_artOk=false;
    CopyField(g_artPath,sizeof(g_artPath),path);
    g_artThread=CreateThread(NULL,128*1024,RenderRequestWorker,NULL,0,NULL);
    return g_artThread!=NULL;
}
static void FinishArtwork(NuvioGraphics &gui,const NuvioState &app) {
    if(!g_artThread||WaitForSingleObject(g_artThread,0)!=WAIT_OBJECT_0)return;
    CloseHandle(g_artThread);g_artThread=NULL;
    if(!g_artOk&&g_artKind>0&&g_artKind<5&&g_artIndex>=0&&g_artIndex<NUVIO_MAX_CATALOG_ITEMS) {
        g_artRetryAt[g_artKind][g_artIndex]=GetTickCount()+30000;
    }
    if(g_artOk&&g_artKind==2&&g_artIndex==g_previewFocusIndex) {
        char *cursor=(char*)g_artReply.bytes;
        char *fields[]={g_preview.genres,g_preview.runtime,g_preview.rating,g_preview.description};
        const int sizes[]={sizeof(g_preview.genres),sizeof(g_preview.runtime),sizeof(g_preview.rating),sizeof(g_preview.description)};
        for(int i=0;i<4&&cursor;++i) {
            char *end=std::strchr(cursor,'\n');if(end)*end=0;
            CopyField(fields[i],sizes[i],cursor);cursor=end?end+1:NULL;
        }
        g_previewIndex=g_artIndex;
    } else if(g_artOk&&(g_artKind==1||g_artKind==3||g_artKind==4)&&g_artReply.length>=8&&
        !std::memcmp(g_artReply.bytes,"NVPO",4)) {
        const unsigned char *b=g_artReply.bytes;
        int w=b[4]|b[5]<<8,h=b[6]|b[7]<<8;
        if(w>0&&h>0&&w<=1024&&h<=1024&&g_artReply.length==8+w*h*4) {
            if(g_artKind==1)gui.UploadPoster(g_artIndex,b+8,w,h);
            if(g_artKind==3&&g_artIndex==g_previewFocusIndex)gui.UploadBackdrop(b+8,w,h);
            if(g_artKind==4)gui.UploadAvatar(g_artIndex,b+8,w,h);
        }
    } else if(g_artKind==5||g_artKind==6) {
        if(app.screen!=NUVIO_SCREEN_SOURCES){NuvioHttpFree(&g_artReply);return;}
        if(g_artOk) {
            g_sourceCount=g_sourceCursor=0;
            char *line=std::strtok((char*)g_artReply.bytes,"\n");
            while(line&&g_sourceCount<240) {
                if(line[0]!='#')CopyField(g_sourceRows[g_sourceCount++],160,line);
                line=std::strtok(NULL,"\n");
            }
            SetStatus(g_sourceCount?"":"Nenhuma fonte retornada pelos addons habilitados.");
        } else SetStatus(NuvioNativeLastError());
    } else if(g_artKind==7||g_artKind==8) {
        if(g_artOk) {
            NuvioAddonsParse(&g_addons,(const char*)g_artReply.bytes,g_artReply.length);
            SetStatus(g_artKind==8?"Alteracao salva na nuvem.":"Addons sincronizados.");
        } else SetStatus(NuvioNativeLastError());
    } else if(g_artKind==9)SetStatus(g_artOk?"Adicionado a biblioteca.":NuvioNativeLastError());
    NuvioHttpFree(&g_artReply);
}
static bool ArtworkRetry(int kind,int index) {
    return index<0||index>=NUVIO_MAX_CATALOG_ITEMS||!g_artRetryAt[kind][index]||
        (LONG)(GetTickCount()-g_artRetryAt[kind][index])>=0;
}
static void ScheduleArtwork(NuvioGraphics &gui,const NuvioState &app,const NuvioCatalog &catalog) {
    if(g_artThread||g_catalogLoading)return;
    char path[512];
    if(app.screen==NUVIO_SCREEN_PROFILE) {
        for(int i=0;i<app.profileCount;++i)if(!gui.HasAvatar(i)&&app.profiles[i].remoteId>0&&ArtworkRetry(4,i)) {
            std::sprintf(path,"/avatar/%d.rgba",app.profiles[i].remoteId);BeginRenderRequest(4,i,path,75000);return;
        }
        return;
    }
    int focused=app.screen==NUVIO_SCREEN_DETAILS?app.selectedCatalogIndex:
        app.screen==NUVIO_SCREEN_HOME||app.screen==NUVIO_SCREEN_DISCOVER?FocusedMovieIndex(app,catalog):-1;
    if(focused!=g_previewFocusIndex) {
        g_previewFocusIndex=focused;g_previewFocusAt=GetTickCount();g_previewIndex=-1;
        std::memset(&g_preview,0,sizeof(g_preview));gui.ClearBackdrop();
    }
    if(focused>=0&&focused<catalog.count&&GetTickCount()-g_previewFocusAt>250&&
        g_previewIndex!=focused&&ArtworkRetry(2,focused)) {
        std::sprintf(path,"/preview/%s.tsv",catalog.items[focused].imdbId);BeginRenderRequest(2,focused,path,4096);return;
    }
    int visible[12],n=0;
    if(app.screen==NUVIO_SCREEN_DETAILS&&focused>=0)visible[n++]=focused;
    else if(app.screen==NUVIO_SCREEN_DISCOVER) {
        int first=(app.selectedRow/2)*10;
        for(int i=0;i<10;++i){int ix=DiscoverItem(app,catalog,first+i);if(ix>=0)visible[n++]=ix;}
    } else if(app.screen==NUVIO_SCREEN_HOME||app.screen==NUVIO_SCREEN_LIBRARY||app.screen==NUVIO_SCREEN_SEARCH_RESULTS) {
        int last=app.screen==NUVIO_SCREEN_HOME&&app.selectedRow>0?2:1;
        for(int r=0;r<last&&n<10;++r)for(int c=0;c<5&&n<10;++c) {
            int ix=NuvioCatalogIndexForRow(&catalog,app.selectedRow+r,(r?0:(app.selectedColumn/5)*5)+c);
            if(ix>=0)visible[n++]=ix;
        }
    }
    for(int i=0;i<n;++i)if(!gui.HasPoster(visible[i])&&ArtworkRetry(1,visible[i])) {
        int ix=visible[i];std::sprintf(path,"/poster/%s.rgba",catalog.items[ix].imdbId);
        BeginRenderRequest(1,ix,path,180000);return;
    }
    if(focused>=0&&focused==g_previewIndex&&!gui.HasBackdrop()&&ArtworkRetry(3,focused)) {
        std::sprintf(path,"/preview/%s.rgba",catalog.items[focused].imdbId);BeginRenderRequest(3,focused,path,1000000);
    }
}

// Loads a catalog page and its posters (old posters are dropped so they never mismatch).
// Network work stays off the render thread; no second request runs concurrently.
static HANDLE g_catalogThread = NULL;
static NuvioHttpData g_catalogReply = {NULL, 0, 0};
static NuvioHttpData g_addonReply = {NULL, 0, 0};
static bool g_catalogOk = false;
static int g_requestPage = 0, g_requestMode = 0;
static char g_requestQuery[512];
static bool g_refreshCloud = false;
static DWORD WINAPI CatalogWorker(void *) {
    if (g_refreshCloud) NuvioHttpGet("native", 0, "/addons", 180000, &g_addonReply);
    char path[600];
    if (g_requestMode == 1) std::sprintf(path, "/search.tsv?q=%s", g_requestQuery);
    else if (g_requestMode == 2) std::strcpy(path, "/library/catalog.tsv");
    else std::sprintf(path, "/catalog.tsv?skip=%d", g_requestPage * 100);
    g_catalogOk = NuvioHttpGet("native", 0, path, 250000, &g_catalogReply);
    return 0;
}
static bool ReloadCatalog(NuvioGraphics &, const char *, int page,
                          int mode = 0, const char *query = "", bool refreshCloud = false) {
    if (g_catalogThread) return false;
    g_requestPage = page; g_requestMode = mode; g_refreshCloud = refreshCloud;
    NuvioUrlEncode(g_requestQuery, sizeof(g_requestQuery), query);
    g_catalogOk = false;
    g_catalogThread = CreateThread(NULL, 128 * 1024, CatalogWorker, NULL, 0, NULL);
    g_catalogLoading = g_catalogThread != NULL;
    return g_catalogLoading;
}
static bool FinishCatalog(NuvioGraphics &gui, NuvioState &app, bool &online) {
    if (!g_catalogThread || WaitForSingleObject(g_catalogThread, 0) != WAIT_OBJECT_0) return false;
    CloseHandle(g_catalogThread); g_catalogThread = NULL; g_catalogLoading = false;
    if (g_addonReply.bytes) {
        NuvioAddonsParse(&g_addons, (const char*)g_addonReply.bytes, g_addonReply.length);
        NuvioHttpFree(&g_addonReply);
    }
    bool ok = g_catalogOk && NuvioCatalogLoadText(&g_catalog,
        (const char*)g_catalogReply.bytes, g_catalogReply.length);
    NuvioHttpFree(&g_catalogReply);
    if (ok) {
        gui.ClearPosters(); gui.ClearBackdrop();
        std::memset(g_artRetryAt,0,sizeof(g_artRetryAt));
        g_previewIndex = g_previewFocusIndex = -1; g_nextPoster = 0;
        std::memset(&g_preview, 0, sizeof(g_preview));
        app.selectedRow = app.selectedColumn = 0;
        online = true;
        SetStatus(g_catalog.count ? "Synchronized with Nuvio." : "No titles in this catalog. X: refresh / RB: more categories");
    } else {
        SetStatus(NuvioNativeLastError());
    }
    return true;
}

// ---- Plugins (add-ons live on the bridge; the console mirrors the list) ----
static bool RefreshAddons(const char *) {
    return BeginRenderRequest(7,0,"/addons",180000);
}

static bool BridgeCommand(const char *ip, const char *path) {
    NuvioHttpData result;
    if (!NuvioHttpGet(ip, BRIDGE_PORT, path, 4096, &result)) return false;
    NuvioHttpFree(&result);
    return true;
}

static bool InstallAddon(const char *ip, const char *url) {
    char encoded[900];
    NuvioUrlEncode(encoded, sizeof(encoded), url);
    char path[1000];
    std::sprintf(path, "/addons/add?url=%s", encoded);
    return BeginRenderRequest(8,0,path,4096);
}

// ============ MAIN ============
int main() {
    NuvioState app;
    NuvioInitialize(&app);
    NuvioProfilesLoad(&app, kProfilesFile);
    LoadP2PSettings();
    NuvioAddonsLoadDefaults(&g_addons);
    g_catalog.count = 0;
    g_catalog.isDemo = false;

    IDirect3D9 *graphics = Direct3DCreate9(D3D_SDK_VERSION);
    if (!graphics) return 1;

    D3DPRESENT_PARAMETERS present;
    ZeroMemory(&present, sizeof(present));
    present.BackBufferWidth = SCREEN_WIDTH;
    present.BackBufferHeight = SCREEN_HEIGHT;
    present.BackBufferFormat = D3DFMT_A8R8G8B8;
    present.BackBufferCount = 1;
    present.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present.Windowed = FALSE;
    present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    IDirect3DDevice9 *device = NULL;
    if (FAILED(graphics->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &present, &device)) || !device) {
        graphics->Release();
        return 2;
    }

    g_videoDevice=device;
    NuvioGraphics gui;
    if (!gui.Initialize(device)) {
        OutputDebugStringA("NUVIO - GPU/font initialization failed\n");
        gui.Shutdown();
        device->Release();
        graphics->Release();
        return 3;
    }

    char bridgeIp[32];
    // IP is ignored by the native router; no PC process or bridge is required.
    std::strcpy(bridgeIp, "native");
    const bool networkReady = NuvioNetworkStartup();
    // NuvioTV launches QR pairing without requiring an initial gamepad click.
    app.screen = NUVIO_SCREEN_QR_LOGIN;
    bool autoQrPending = true;

    OutputDebugStringA("NUVIO - Streaming UI started\n");

    bool online = false;
    int lastPosterFetch = 0;
    int posterPasses = 0;
    int currentCatalogPage = 0;
    bool catalogDirty = false;
    NuvioScreen lastScreen = app.screen;

    // Keep independent edge state for every Xbox controller. Xenia may assign
    // a keyboard/gamepad to a slot other than zero.
    WORD previousButtons[4] = { 0, 0, 0, 0 };
    NuvioInputRepeat inputRepeat[4];
    bool running = true;
    DWORD lastFrameTime = GetTickCount();

    while (running) {
        WORD pressed = 0;
        g_controllerPort = -1;
        for (DWORD port = 0; port < 4; ++port) {
            XINPUT_STATE input;
            ZeroMemory(&input, sizeof(input));
            if (XInputGetState(port, &input) != ERROR_SUCCESS) {
                previousButtons[port] = 0;
                continue;
            }
            if (g_controllerPort < 0) g_controllerPort = (int)port;
            WORD buttons = input.Gamepad.wButtons;
            WORD directions=NuvioStickDirections(input.Gamepad.sThumbLX,input.Gamepad.sThumbLY);
            if(!directions)directions=NuvioStickDirections(input.Gamepad.sThumbRX,input.Gamepad.sThumbRY);
            directions|=buttons&0xF;
            pressed|=NuvioRepeatDirections(inputRepeat[port],directions,GetTickCount());
            pressed|=(buttons&~0xF)&static_cast<WORD>(~previousButtons[port]);
            previousButtons[port] = buttons;
        }

        FinishArtwork(gui,app);
        if(FinishCatalog(gui,app,online))posterPasses=0;
        if(g_artThread||g_catalogLoading) {
            WORD localMask=0xF|XINPUT_GAMEPAD_BACK;
            const bool browse=app.screen==NUVIO_SCREEN_HOME||app.screen==NUVIO_SCREEN_DISCOVER||
                app.screen==NUVIO_SCREEN_LIBRARY||app.screen==NUVIO_SCREEN_SEARCH_RESULTS;
            if(!g_catalogLoading&&browse&&!app.sidebarFocused)localMask|=XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_Y;
            localMask|=XINPUT_GAMEPAD_B;
            g_deferredButtons|=pressed&~localMask;
            pressed&=localMask;
        } else {pressed|=g_deferredButtons;g_deferredButtons=0;}
        if (pressed) {
            g_lastInputButtons = pressed;
            g_lastInputAt = GetTickCount();
        }
        const bool bUp    = (pressed & XINPUT_GAMEPAD_DPAD_UP) != 0;
        const bool bDown  = (pressed & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
        const bool bLeft  = (pressed & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
        const bool bRight = (pressed & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
        const bool bA     = (pressed & XINPUT_GAMEPAD_A) != 0;
        const bool bB     = (pressed & XINPUT_GAMEPAD_B) != 0;
        const bool bX     = (pressed & XINPUT_GAMEPAD_X) != 0;
        const bool bY     = (pressed & XINPUT_GAMEPAD_Y) != 0;
        const bool bStart = (pressed & XINPUT_GAMEPAD_START) != 0;
        const bool bShift = (pressed & (XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER)) != 0;

        if (pressed & XINPUT_GAMEPAD_BACK) running = false;

        // Present the QR loading screen before a potentially slow HTTPS call.
        // This also avoids depending on Xenia's keyboard/gamepad mapping for sign-in.
        if (autoQrPending) {
            autoQrPending = false;
            DrawScreen(gui, app, g_catalog, online);
            BeginQrSignIn(bridgeIp, networkReady, app);
        }


        // Sign in with QR code: check the approval every 3 seconds.
        if (app.screen == NUVIO_SCREEN_QR_LOGIN && g_account.state == ACCOUNT_PENDING &&
            GetTickCount() - g_account.lastPoll > 3000) {
            g_account.lastPoll = GetTickCount();
            if (AccountRequest(bridgeIp, "/account/poll") && g_account.state == ACCOUNT_SIGNED_IN) {
                if (!SyncRemoteProfiles(app, bridgeIp))
                    SetStatus("Signed in, but cloud profile sync failed. Retry login.");
                g_avatarNext = 0;
                gui.ClearAvatars();
                app.screen = NUVIO_SCREEN_PROFILE;
                app.profileCursor = app.profileIndex;
                if (!g_status[0]) SetStatus("Signed in. Profiles synchronized from Nuvio.");
            }
        }

        switch (app.screen) {
        case NUVIO_SCREEN_PLAYER:
            if(pressed)g_playerInputAt=GetTickCount();
            if(bA||bStart)g_player->TogglePause();
            if(bLeft)g_player->SeekRelative(-10);
            if(bRight)g_player->SeekRelative(10);
            if(bB) {delete g_player;g_player=NULL;g_playingTorrent=false;app.screen=NUVIO_SCREEN_SOURCES;}
            break;

        case NUVIO_SCREEN_P2P_SETTINGS:
            if(bUp&&app.selectedRow>0)--app.selectedRow;
            if(bDown&&app.selectedRow<2)++app.selectedRow;
            if(bB)NuvioBack(&app);
            else if(bA) {
                if(app.selectedRow==0)g_p2pEnabled=!g_p2pEnabled;
                else if(app.selectedRow==1)g_hideTorrentStats=!g_hideTorrentStats;
                else {
                    SetStatus(NuvioTorrentClearCache()?"Cache de torrent limpo.":"Falha ao limpar cache de torrent.");
                    break;
                }
                if(!SaveP2PSettings())SetStatus("Falha ao salvar configuracoes P2P.");
            }
            break;

        case NUVIO_SCREEN_LOGIN:
            if (bA || bStart) {
                NuvioLoginSelect(&app);
                DrawScreen(gui, app, g_catalog, online);
                BeginQrSignIn(bridgeIp, networkReady, app);
            }
            break;

        case NUVIO_SCREEN_QR_LOGIN:
            if (bB) NuvioBack(&app);
            else if ((bA || bStart) && g_account.state == ACCOUNT_ERROR)
                BeginQrSignIn(bridgeIp, networkReady, app);
            break;

        case NUVIO_SCREEN_PROFILE:
            if (bLeft) NuvioProfileMove(&app, -1);
            if (bRight) NuvioProfileMove(&app, 1);
            if (bUp && app.profileCursor >= 4) NuvioProfileMove(&app, -4);
            if (bDown && app.profileCursor + 4 < app.profileCount + (!app.cloudProfiles && app.profileCount < NUVIO_MAX_PROFILES ? 1 : 0))
                NuvioProfileMove(&app, 4);
            if (bB) NuvioBack(&app);
            if (bX && app.profileCursor > 0 && app.profileCursor < app.profileCount) {
                if (app.profiles[app.profileCursor].remoteId >= 1)
                    SetStatus("Manage cloud profiles in Nuvio account settings.");
                else {
                    NuvioProfileRemove(&app, app.profileCursor);
                    NuvioProfilesSave(&app, kProfilesFile);
                }
            }
            if (bA && app.profileCursor < app.profileCount && app.profiles[app.profileCursor].locked)
                SetStatus("PIN protected profile. Unlock via Nuvio first.");
            if (bA && NuvioProfileSelect(&app) == 1) {
                char profilePath[64];
                std::sprintf(profilePath, "/profile/select?id=%d", app.profiles[app.profileIndex].remoteId);
                if (!networkReady || !BridgeCommand(bridgeIp, profilePath)) {
                    app.screen = NUVIO_SCREEN_PROFILE;
                    SetStatus("Select an unlocked cloud profile and retry.");
                } else {
                    NuvioCatalogClear(&g_catalog);
                    gui.ClearPosters(); gui.ClearBackdrop();
                    online = false;
                    ReloadCatalog(gui, bridgeIp, 0, 0, "", true);
                    currentCatalogPage = 0; posterPasses = 0;
                }
            }
            break;

        case NUVIO_SCREEN_CREATE_PROFILE:
        case NUVIO_SCREEN_ADD_ADDON: {
            int result = NUVIO_KEY_NONE;
            if (bUp) NuvioKeyboardMove(&app, 0, -1);
            if (bDown) NuvioKeyboardMove(&app, 0, 1);
            if (bLeft) NuvioKeyboardMove(&app, -1, 0);
            if (bRight) NuvioKeyboardMove(&app, 1, 0);
            if (bA) result = NuvioKeyboardPress(&app);
            if (bX) NuvioKeyboardBackspace(&app);
            if (bY) NuvioKeyboardSpace(&app);
            if (bShift) NuvioKeyboardToggleShift(&app);
            if (bStart) result = NUVIO_KEY_DONE;
            if (bB) result = NUVIO_KEY_CANCEL;

            if (result == NUVIO_KEY_CANCEL) {
                NuvioBack(&app);
            } else if (result == NUVIO_KEY_DONE) {
                if (app.screen == NUVIO_SCREEN_CREATE_PROFILE) {
                    if (NuvioProfileCreateConfirm(&app)) {
                        NuvioProfilesSave(&app, kProfilesFile);
                    } else {
                        SetStatus("Type a name that is not empty and not already used.");
                    }
                } else {
                    // Install plugin through the bridge
                    if (!networkReady) {
                        SetStatus("Network is not available.");
                    } else if (std::strncmp(app.textBuffer, "http", 4) != 0) {
                        SetStatus("The URL must start with http:// or https://");
                    } else {
                        SetStatus("Installing plugin... please wait");
                        DrawScreen(gui, app, g_catalog, online);
                        if (InstallAddon(bridgeIp, app.textBuffer)) {

                            catalogDirty = true;
                            SetStatus("Instalando addon na nuvem...");
                            NuvioBack(&app);
                            app.selectedRow = g_addons.count > 0 ? g_addons.count - 1 : 0;
                        } else {
                            SetStatus(NuvioNativeLastError());
                        }
                    }
                }
            }
            break;
        }

        case NUVIO_SCREEN_ADDONS: {
            if (bY) {
                if (RefreshAddons(bridgeIp)) { catalogDirty = true; SetStatus("Cloud addons synchronized."); }
                else SetStatus(NuvioNativeLastError());
            }
            const int rows = g_addons.count + 1;
            if (bUp) NuvioMoveVertical(&app, -1, rows);
            if (bDown) NuvioMoveVertical(&app, 1, rows);
            if (bB) NuvioBack(&app);
            if (bA) {
                if (app.selectedRow == g_addons.count) {
                    if (g_addons.count >= NUVIO_MAX_ADDONS) {
                        SetStatus("Plugin limit reached. Remove one first.");
                    } else {
                        NuvioKeyboardBegin(&app, NUVIO_KEYBOARD_URL, "https://");
                        app.screen = NUVIO_SCREEN_ADD_ADDON;
                    }
                } else if (!g_addons.fromCloud || !networkReady) {
                    SetStatus("Synchronize cloud addons first (Y).");
                } else {
                    char path[48];
                    std::sprintf(path, "/addons/toggle?id=%d", g_addons.items[app.selectedRow].id);
                    if (BeginRenderRequest(8,0,path,4096)) {
                        catalogDirty = true;
                        SetStatus("Salvando alteracao na nuvem...");
                    } else {
                        SetStatus(NuvioNativeLastError());
                    }
                }
            }
            if (bX && app.selectedRow < g_addons.count) {
                if (!g_addons.fromCloud || !networkReady) {
                    SetStatus("Synchronize cloud addons first (Y).");
                } else {
                    char path[48];
                    std::sprintf(path, "/addons/remove?id=%d", g_addons.items[app.selectedRow].id);
                    if (BeginRenderRequest(8,0,path,4096)) {
                        catalogDirty = true;
                        if (app.selectedRow > g_addons.count) app.selectedRow = g_addons.count;
                        SetStatus("Removendo addon da nuvem...");
                    } else {
                        SetStatus(NuvioNativeLastError());
                    }
                }
            }
            break;
        }

        case NUVIO_SCREEN_SOURCES: {
            if (bUp && g_sourceCursor > 0) --g_sourceCursor;
            if (bDown && g_sourceCursor+1 < g_sourceCount) ++g_sourceCursor;
            if (bB) {
                if(g_artThread) {app.screen=NUVIO_SCREEN_DETAILS;break;}
                if (!g_episodePicker && g_sourceEpisode >= 0) {
                    LoadSourceRows(true); g_sourceEpisode = -1;
                } else app.screen = NUVIO_SCREEN_DETAILS;
            }
            if (bX && !LoadSourceRows(g_episodePicker)) SetStatus(NuvioNativeLastError());
            if (bA && g_sourceCount) {
                if (g_episodePicker) {
                    g_sourceEpisode = g_sourceCursor;
                    DrawScreen(gui,app,g_catalog,online);
                    if (!LoadSourceRows(false)) SetStatus(NuvioNativeLastError());
                } else {
                    NuvioPlaybackSource source;
                    if(!NuvioNativePlaybackSource(g_sourceCursor,&source))SetStatus(NuvioNativeLastError());
                    else if(source.url.find("magnet:?")==0&&!g_p2pEnabled)
                        SetStatus("Ative Streaming P2P em Configuracoes para abrir torrents.");
                    else {
                        if(!g_player) {g_player=new FFPlayer;if(!g_player->Startup(device)){delete g_player;g_player=NULL;}}
                        if(!g_player)SetStatus("Falha ao iniciar decoder ou audio.");
                        else {
                            gui.ClearPosters();gui.ClearBackdrop();gui.ClearAvatars();
                            std::vector<std::string> urls;urls.push_back(source.url);
                            std::vector<std::wstring> labels;labels.push_back(L"Nuvio");
                            g_playingTorrent=source.url.find("magnet:?")==0;
                            g_player->Open(urls,labels,source.headers,0,0,0,source.prepareBody);
                            app.screen=NUVIO_SCREEN_PLAYER;g_playerInputAt=GetTickCount();
                        }
                    }
                }
            }
            break;
        }

        case NUVIO_SCREEN_SEARCH: {
            if (!app.searchActive) app.searchActive = true;
            if (bUp) NuvioKeyboardMove(&app, 0, -1);
            if (bDown) NuvioKeyboardMove(&app, 0, 1);
            if (bLeft) NuvioKeyboardMove(&app, -1, 0);
            if (bRight) NuvioKeyboardMove(&app, 1, 0);
            int key = bA ? NuvioKeyboardPress(&app) : NUVIO_KEY_NONE;
            if (bX) NuvioKeyboardBackspace(&app);
            if (bY) NuvioKeyboardSpace(&app);
            if (bShift) NuvioKeyboardToggleShift(&app);
            if (bB || key == NUVIO_KEY_CANCEL) NuvioBack(&app);
            else if (bStart || key == NUVIO_KEY_DONE) {
                CopyField(app.searchQuery, sizeof(app.searchQuery), app.textBuffer);
                if (app.searchQuery[0]) {
                    NuvioSearchSubmit(&app);
                    NuvioCatalogClear(&g_catalog); gui.ClearPosters(); gui.ClearBackdrop();
                    ReloadCatalog(gui, bridgeIp, 0, 1, app.searchQuery);
                }
            }
            break;
        }

        default: {
            if (app.screen == NUVIO_SCREEN_SETTINGS && (bY || (bA && !app.sidebarFocused && app.selectedRow == 4))) {
                if (AccountRequest(bridgeIp, "/account/logout")) {
                    NuvioInitialize(&app);
                    NuvioCatalogClear(&g_catalog);
                    gui.ClearAvatars(); gui.ClearPosters(); gui.ClearBackdrop();
                    online = false;
                    std::memset(&g_addons, 0, sizeof(g_addons));
                    g_status[0] = 0;
                    app.screen = NUVIO_SCREEN_LOGIN;
                    g_qrSize = 0;
                }
                break;
            }
            const int rowCount = app.screen == NUVIO_SCREEN_SETTINGS ? 5 :
                app.screen==NUVIO_SCREEN_DISCOVER?(DiscoverCount(app,g_catalog)+4)/5:NuvioCatalogRowCount(&g_catalog);
            if (bLeft) NuvioMoveHorizontal(&app, -1);
            if (bRight) NuvioMoveHorizontal(&app, 1);
            if (bUp) NuvioMoveVertical(&app, -1, rowCount);
            if (bDown) NuvioMoveVertical(&app, 1, rowCount);
            if (g_catalog.railCount && app.screen != NUVIO_SCREEN_SETTINGS && app.screen!=NUVIO_SCREEN_DISCOVER && !app.sidebarFocused) {
                const int rail = app.selectedRow < g_catalog.railCount ? app.selectedRow : 0;
                const int count = g_catalog.rails[rail].count;
                if (app.selectedColumn >= count) app.selectedColumn = count > 0 ? count - 1 : 0;
            }
            if(app.screen==NUVIO_SCREEN_DISCOVER && !app.sidebarFocused) {
                if(app.selectedColumn>4)app.selectedColumn=4;
                while(app.selectedColumn>0&&DiscoverItem(app,g_catalog,app.selectedRow*5+app.selectedColumn)<0)--app.selectedColumn;
            }
            if (bB) NuvioBack(&app);
            if (bA && app.screen == NUVIO_SCREEN_SETTINGS && !app.sidebarFocused && app.selectedRow == 3) {
                ReloadCatalog(gui, bridgeIp, 0, 0, "", true);
                currentCatalogPage = 0;
            } else if (bA) NuvioSelect(&app,
                (app.screen == NUVIO_SCREEN_HOME || app.screen == NUVIO_SCREEN_DISCOVER)
                ? FocusedMovieIndex(app, g_catalog)
                : NuvioCatalogIndexForRow(&g_catalog, app.selectedRow, app.selectedColumn));

            if (bA && app.screen == NUVIO_SCREEN_DETAILS && lastScreen == NUVIO_SCREEN_DETAILS &&
                app.selectedCatalogIndex >= 0 && app.selectedCatalogIndex < g_catalog.count) {
                const NuvioCatalogItem &title = g_catalog.items[app.selectedCatalogIndex];
                CopyField(g_sourceId,sizeof(g_sourceId),title.imdbId);
                g_sourceEpisode = -1;
                DrawScreen(gui,app,g_catalog,online);
                if (LoadSourceRows(title.type == 'S')) app.screen = NUVIO_SCREEN_SOURCES;
                else SetStatus(NuvioNativeLastError());
            }
            // X saves the selected title on the actual Nuvio cloud profile.
            if (bX && app.screen == NUVIO_SCREEN_DETAILS) {
                if (g_account.state != ACCOUNT_SIGNED_IN)
                    SetStatus("Sign in to your Nuvio account to save titles.");
                else if (AddToCloudLibrary(bridgeIp, app, g_catalog))
                    SetStatus("Salvando na biblioteca...");
                else SetStatus("Save failed. Check Nuvio connectivity and try again.");
            } else if (bX && networkReady && app.screen != NUVIO_SCREEN_DETAILS) {
                ReloadCatalog(gui, bridgeIp, currentCatalogPage,
                    app.screen == NUVIO_SCREEN_SEARCH_RESULTS ? 1 : app.screen == NUVIO_SCREEN_LIBRARY ? 2 : 0, app.searchQuery, true);
                posterPasses = 0;
            }

            // Y - search
            if (bY && app.screen==NUVIO_SCREEN_DISCOVER) {
                NuvioSetCatalogMode(&app,(NuvioCatalogMode)((app.catalogMode+1)%3));
            } else if (bY || (bStart && app.screen==NUVIO_SCREEN_DISCOVER)) {
                if (app.screen != NUVIO_SCREEN_SEARCH) NuvioOpenScreen(&app, NUVIO_SCREEN_SEARCH);
                else NuvioSearchSubmit(&app);
            }

            // Shoulders - next / previous catalog page
            if ((pressed & XINPUT_GAMEPAD_RIGHT_SHOULDER) && networkReady && online && (app.screen == NUVIO_SCREEN_HOME || app.screen == NUVIO_SCREEN_DISCOVER)) {
                ++currentCatalogPage;
                if (!ReloadCatalog(gui, bridgeIp, currentCatalogPage)) --currentCatalogPage;
                posterPasses = 0;
            }
            if ((pressed & XINPUT_GAMEPAD_LEFT_SHOULDER) && networkReady && online && currentCatalogPage > 0 && (app.screen == NUVIO_SCREEN_HOME || app.screen == NUVIO_SCREEN_DISCOVER)) {
                --currentCatalogPage;
                if (!ReloadCatalog(gui, bridgeIp, currentCatalogPage)) ++currentCatalogPage;
                posterPasses = 0;
            }
            break;
        }
        }

        // ---- Screen transitions ----
        if (app.screen != lastScreen) {
            if (!(lastScreen == NUVIO_SCREEN_ADD_ADDON && app.screen == NUVIO_SCREEN_ADDONS))
                g_status[0] = '\0';

            // Native plugins can be configured locally on the Xbox.
            if (app.screen == NUVIO_SCREEN_ADDONS && lastScreen != NUVIO_SCREEN_ADD_ADDON) {
                if (networkReady && RefreshAddons(bridgeIp)) {
                    // list is now live
                } else {
                    g_addons.fromCloud = 0;
                }
            }

            if (app.screen == NUVIO_SCREEN_SEARCH) NuvioKeyboardBegin(&app, NUVIO_KEYBOARD_SEARCH, app.searchQuery);
            if (app.screen == NUVIO_SCREEN_LIBRARY && lastScreen != NUVIO_SCREEN_DETAILS && networkReady) {
                NuvioCatalogClear(&g_catalog); gui.ClearPosters(); gui.ClearBackdrop();
                ReloadCatalog(gui, bridgeIp, 0, 2);
            }
            if ((app.screen == NUVIO_SCREEN_HOME || app.screen == NUVIO_SCREEN_DISCOVER) &&
                lastScreen != NUVIO_SCREEN_DETAILS && lastScreen != NUVIO_SCREEN_HOME &&
                lastScreen != NUVIO_SCREEN_DISCOVER && lastScreen != NUVIO_SCREEN_PROFILE && !g_catalogLoading) {
                NuvioCatalogClear(&g_catalog); gui.ClearPosters(); gui.ClearBackdrop();
                ReloadCatalog(gui, bridgeIp, 0);
            }

            // Leaving the plugin list after changes: rebuild the catalog from enabled plugins.
            if (lastScreen == NUVIO_SCREEN_ADDONS && app.screen == NUVIO_SCREEN_SETTINGS &&
                catalogDirty && networkReady) {
                SetStatus("");
                DrawScreen(gui, app, g_catalog, online);
                ReloadCatalog(gui, bridgeIp, 0);
                currentCatalogPage = 0;
                posterPasses = 0;
                catalogDirty = false;
            }
            lastScreen = app.screen;
        }

        if(networkReady)ScheduleArtwork(gui,app,g_catalog);
        DrawScreen(gui, app, g_catalog, online);

        DWORD frameTime = GetTickCount() - lastFrameTime;
        if (frameTime < 16) Sleep(16 - frameTime);
        lastFrameTime = GetTickCount();
    }

    if (g_catalogThread) { WaitForSingleObject(g_catalogThread, INFINITE); CloseHandle(g_catalogThread); }
    if(g_artThread){WaitForSingleObject(g_artThread,INFINITE);CloseHandle(g_artThread);}
    NuvioHttpFree(&g_artReply);
    NuvioHttpFree(&g_catalogReply); NuvioHttpFree(&g_addonReply);
    if(g_player){delete g_player;g_player=NULL;}
    NuvioNetworkShutdown();
    gui.Shutdown();
    device->Release();
    graphics->Release();
    return 0;
}
