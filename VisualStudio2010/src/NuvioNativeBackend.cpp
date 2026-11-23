// NUVIO.XEX: native HTTPS, catalog, device login and profile synchronization.
// NuvioTV's public API protocol is reimplemented for the Xbox 360 XDK.
// Dependency: HG-ERIK/x360-homebrew-libs (MIT HTTPS + Apache-2.0 mbedTLS).
// Dependency: kazuho/picojson (BSD-2-Clause), nothings/stb (MIT/Public Domain),
//             nayuki/QR-Code-generator (MIT).
#include "NuvioNativeBackend.h"
#include "NuvioNetwork.h"
#include "NuvioPlayback.h"
#include "Http.h"
#if defined(_MSC_VER) && _MSC_VER < 1900
// picojson's current header uses C++11 noexcept; XDK/VS2010 speaks C++03.
#define noexcept
#endif
#include "picojson.h"
#if defined(_MSC_VER) && _MSC_VER < 1900
#undef noexcept
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 2048
#include "stb_image.h"

#include "qrcodegen.h"

#include <xtl.h>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

namespace {

static const char kApi[] = "https://api.nuvio.tv";
static const char kKey[] = "sb_publishable_1Clq8rlTVACkdcZuqr6_AD__xUUC_EN"; // public key
static const char kDefaultCatalog[] = "https://catalog.nuvio.tv";
static std::string s_error;
static bool s_started = false;
static std::string s_anon, s_access, s_refresh, s_email;
static std::string s_nonce, s_deviceCode, s_userCode, s_link, s_qrUrl;
static unsigned long s_tokenExpires = 0;
static int s_accountState = 0; // 0 signed out, 1 pending, 2 signed in
static std::string s_sessionPath;
static int s_apiStatus = 0;
static std::string s_apiCode;
static bool s_invalidRefresh = false;
static bool RefreshToken();

struct Art { std::string id, poster, background, kind, title, base, description; picojson::value metadata; };
static std::vector<Art> s_art;
struct CloudAddon {
    std::string base, name;
    bool enabled;
    picojson::value manifest;
};
static std::vector<picojson::value> s_sources;
static std::vector<CloudAddon> s_addons;
static int s_profile = 0, s_addonProfile = 0;
static bool s_primaryAddons[7] = {false};
static bool s_profileAllowed[7] = {false};
static bool s_addonsLoaded = false, s_addonsComplete = false;
static std::string s_owner;
static std::string s_avatarUrls[7];
static bool HttpJson(const std::string &url, picojson::value &v);
static std::string Encode(const std::string &s);
static std::string Canonical(const std::string &input);
static std::string AddonUrl(const std::string &base, const std::string &path);
static void LoadManifest(CloudAddon &a);
static void ClearProfileData() {
    s_sources.clear();
    s_art.clear(); s_addons.clear(); s_owner.clear();
    s_profile = s_addonProfile = 0;
    s_addonsLoaded = s_addonsComplete = false;
    for (int i = 0; i < 7; ++i) {
        s_primaryAddons[i] = s_profileAllowed[i] = false;
        s_avatarUrls[i].clear();
    }
}
static Art *Artwork(const std::string &id);
static std::string Image(const std::string &url, int width, int height, bool roundAvatar);

static void Error(const std::string& reason) {
    // Error messages never contain authentication tokens or API headers.
    s_error = reason.substr(0, 125);
}

static bool ReturnBody(const std::string &body, int maxBody, NuvioHttpData *out) {
    if (body.empty() || (int)body.size() > maxBody) return false;
    out->bytes = new unsigned char[body.size() + 1];
    if (!out->bytes) return false;
    std::memcpy(out->bytes, body.data(), body.size());
    out->bytes[body.size()] = 0;
    out->length = (int)body.size();
    out->status = 200;
    return true;
}

static const picojson::value &Field(const picojson::value &v, const char *key) {
    static const picojson::value kNull;
    if (!v.is<picojson::object>()) return kNull;
    const picojson::object &o = v.get<picojson::object>();
    picojson::object::const_iterator it = o.find(key);
    return it == o.end() ? kNull : it->second;
}

static std::string AsString(const picojson::value &v) {
    if (v.is<std::string>()) return v.get<std::string>();
    if (v.is<double>()) { char s[40]; std::sprintf(s, "%.0f", v.get<double>()); return s; }
    return std::string();
}

static int AsInt(const picojson::value &v, int fallback) {
    return v.is<double>() ? (int)v.get<double>() : fallback;
}

static bool Parse(const std::string &raw, picojson::value &out) {
    return picojson::parse(out, raw).empty();
}

static std::string SafeText(const std::string &value, size_t maxLen) {
    std::string s = value.substr(0, maxLen);
    for (size_t i = 0; i < s.size(); ++i) if (s[i] == '\t' || s[i] == '\r' || s[i] == '\n') s[i] = ' ';
    return s;
}

static bool Api(const char *method, const std::string &path, const std::string &bearer,
                const std::string &payload, picojson::value &reply, bool retryAuth = true) {
    std::string headers = "apikey: "; headers += kKey;
    // Official NuvioTV requests device-login sessions with the public key
    // only. A publishable key is not a JWT and must not be used as a bearer.
    if (!bearer.empty()) headers += "\r\nAuthorization: Bearer " + bearer;
    headers += "\r\nContent-Type: application/json\r\nAccept: application/json\r\nUser-Agent: NUVIO/0.7\r\n";
    Http::Response r = Http::Request(method, std::string(kApi) + path, headers, payload, 16000);
    // A token can be revoked/expire before its local deadline. Retry once with
    // the rotated token, as NuvioTV does for authenticated sync operations.
    if (r.status == 401 && retryAuth && !bearer.empty() && !s_refresh.empty() &&
        path.find("/auth/v1/token") != 0) {
        if (!RefreshToken()) return false;
        return Api(method, path, s_access, payload, reply, false);
    }
    s_apiStatus = r.status;
    s_apiCode.clear();
    s_invalidRefresh = false;
    if (r.status < 200 || r.status >= 300) {
        picojson::value failure;
        if (Parse(r.body, failure)) {
            s_apiCode = AsString(Field(failure, "code"));
            if (s_apiCode.empty()) s_apiCode = AsString(Field(failure, "error_code"));
        }
        if (path.find("/auth/v1/token?grant_type=refresh_token") == 0 &&
            (r.status == 400 || r.status == 401 || r.status == 403)) {
            std::string normalized = r.body;
            for (size_t i = 0; i < normalized.size(); ++i)
                if (normalized[i] >= 'A' && normalized[i] <= 'Z') normalized[i] += 'a' - 'A';
            const char *markers[] = {"invalid refresh token", "refresh token is not valid",
                "refresh token not found", "refresh_token_not_found", "invalid_grant",
                "session not found", "session_not_found", "invalid session",
                "refresh_token_already_used", "session_expired", "user_not_found"};
            for (size_t i = 0; i < sizeof(markers) / sizeof(markers[0]); ++i)
                if (normalized.find(markers[i]) != std::string::npos) s_invalidRefresh = true;
        }
        std::string endpoint = path.substr(path.find_last_of('/') + 1);
        endpoint = endpoint.substr(0, endpoint.find('?'));
        char num[100]; std::sprintf(num, "Nuvio API HTTP %d (%s)", r.status, endpoint.c_str());
        // Only report transport-provided diagnostics; never include response bodies,
        // request headers, user credentials or query strings on the TV screen.
        Error(r.status ? num : (r.error.empty() ? "Nuvio API transport failed" : r.error));
        return false;
    }
    if (r.body.empty()) { reply = picojson::value(); return true; }
    if (!Parse(r.body, reply)) { Error("Unexpected Nuvio API response"); return false; }
    return true;
}

static std::string Object(const char *key, const std::string &value) {
    picojson::object o; o[key] = picojson::value(value);
    return picojson::value(o).serialize();
}

static const picojson::value &FirstRow(const picojson::value &v) {
    static const picojson::value kNull;
    if (!v.is<picojson::array>()) return kNull;
    const picojson::array &a = v.get<picojson::array>();
    return a.empty() ? kNull : a[0];
}

static std::string HexNonce() {
    // The device nonce must not be predictable; Xbox XNet provides the RNG.
    unsigned char bytes[24];
    if (XNetRandom(bytes, sizeof(bytes)) != 0) return std::string();
    static const char hex[] = "0123456789abcdef";
    std::string result;
    for (int i = 0; i < 24; ++i) {
        result += hex[bytes[i] >> 4];
        result += hex[bytes[i] & 15];
    }
    return result;
}

// Only the renewable credential is persisted; passwords, QR secrets and access
// tokens never go to disk. The app folder must be writable (USB/HDD or Xenia).
// Like Android's session store, this file is private account data, not an asset.
static const char *const kSessionPaths[] = {
#ifdef NUVIO_BACKEND_TEST
    "obj/AuthTests/nuvio_session.dat"
#else
    "game:\\nuvio_session.dat", "D:\\nuvio_session.dat", "nuvio_session.dat"
#endif
};

static void ClearSession() {
    ClearProfileData();
    for (size_t i = 0; i < sizeof(kSessionPaths) / sizeof(kSessionPaths[0]); ++i) {
        std::remove(kSessionPaths[i]);
        std::remove((std::string(kSessionPaths[i]) + ".tmp").c_str());
    }
    s_sessionPath.clear();
    s_access.clear(); s_refresh.clear(); s_email.clear();
    s_accountState = 0; s_tokenExpires = 0;
}

static bool SaveSession() {
    if (s_refresh.empty()) return false;
    picojson::object record;
    record["version"] = picojson::value(1.0);
    record["backend"] = picojson::value(kApi);
    record["refresh_token"] = picojson::value(s_refresh);
    record["email"] = picojson::value(s_email);
    const std::string data = picojson::value(record).serialize();
    const size_t count = sizeof(kSessionPaths) / sizeof(kSessionPaths[0]);
    for (size_t i = 0; i < count; ++i) {
        const std::string path = s_sessionPath.empty() ? kSessionPaths[i] : s_sessionPath;
        const std::string temp = path + ".tmp";
        std::FILE *f = std::fopen(temp.c_str(), "wb");
        if (f) {
            bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
            if (std::fflush(f) != 0) ok = false;
            if (std::fclose(f) != 0) ok = false;
            if (ok && MoveFileExA(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                s_sessionPath = path;
                return true;
            }
            std::remove(temp.c_str());
        }
        if (!s_sessionPath.empty()) break;
    }
    Error("Signed in, but session could not be saved. Check app folder write access.");
    return false;
}

static void LoadSession() {
    for (size_t i = 0; i < sizeof(kSessionPaths) / sizeof(kSessionPaths[0]); ++i) {
        std::FILE *f = std::fopen(kSessionPaths[i], "rb");
        if (!f) continue;
        char buffer[8193];
        const size_t size = std::fread(buffer, 1, sizeof(buffer), f);
        const bool readOk = !std::ferror(f);
        std::fclose(f);
        picojson::value data;
        if (!readOk || size == 0 || size >= sizeof(buffer) ||
            !Parse(std::string(buffer, size), data) ||
            AsInt(Field(data, "version"), 0) != 1 || AsString(Field(data, "backend")) != kApi) continue;
        const std::string token = AsString(Field(data, "refresh_token"));
        if (token.empty() || token.size() > 4096) continue;
        s_refresh = token;
        s_email = AsString(Field(data, "email"));
        s_sessionPath = kSessionPaths[i];
        s_accountState = 2;
        // Access token deliberately absent: refresh before making any API call.
        s_access.clear(); s_tokenExpires = 0;
        return;
    }
}

static void TokenDeadline(int lifetime) {
    // Bound deadlines to the signed-tick range, including short-lived tokens.
    if (lifetime < 1) lifetime = 1;
    if (lifetime > 86400) lifetime = 86400;
    const int margin = lifetime > 180 ? 90 : lifetime / 2;
    s_tokenExpires = GetTickCount() + (unsigned long)(lifetime - margin) * 1000;
}

static bool RefreshToken() {
    if (s_refresh.empty()) return false;
    picojson::value v;
    if (!Api("POST", "/auth/v1/token?grant_type=refresh_token", std::string(),
             Object("refresh_token", s_refresh), v)) {
        // NuvioTV only discards a session on a definitive invalid-refresh
        // response. Offline, 406, rate limits and 5xx must keep the credential.
        if (s_invalidRefresh) ClearSession();
        return false;
    }
    const std::string token = AsString(Field(v, "access_token"));
    if (token.empty()) { Error("Nuvio refresh response is missing the access token"); return false; }
    s_access = token;
    std::string updated = AsString(Field(v, "refresh_token"));
    if (!updated.empty()) s_refresh = updated;
    const std::string email = AsString(Field(Field(v, "user"), "email"));
    if (!email.empty()) s_email = email;
    TokenDeadline(AsInt(Field(v, "expires_in"), 3600));
    s_accountState = 2;
    s_error.clear();
    SaveSession(); // Save rotated refresh tokens immediately, not only at exit.
    return true;
}

static bool EnsureAuth() {
    if (s_accountState != 2) return false;
    // GetTickCount wrap-around safe as long as tokens live for <49 days.
    if (s_access.empty() || (LONG)(GetTickCount() - s_tokenExpires) >= 0) return RefreshToken();
    return true;
}

static void StartLogin() {
    if (s_accountState == 2) {
        if (EnsureAuth()) return;
        if (s_accountState == 2) return; // transient: retry saved session, no QR
    }
    if (s_accountState != 0) return;
    s_error.clear();
    // Device pairing does not require a newly created anonymous account.
    // NuvioTV/AuthManager.kt starts it using the publishable API key.
    s_nonce = HexNonce();
    if (s_nonce.empty()) { Error("Could not generate a secure device nonce"); return; }
    picojson::object payload;
    payload["p_device_nonce"] = picojson::value(s_nonce);
    payload["p_redirect_base_url"] = picojson::value("https://nuvio.tv/link");
    payload["p_device_type"] = picojson::value("tv");
    payload["p_device_name"] = picojson::value("NUVIO");
    picojson::value response;
    if (!Api("POST", "/rest/v1/rpc/start_device_login_session", "",
             picojson::value(payload).serialize(), response)) return;
    const picojson::value &row = FirstRow(response);
    s_deviceCode = AsString(Field(row, "device_code"));
    s_userCode = AsString(Field(row, "user_code"));
    s_link = AsString(Field(row, "verification_uri"));
    s_qrUrl = AsString(Field(row, "verification_uri_complete"));
    if (s_link.empty()) s_link = "https://nuvio.tv/link";
    if (s_qrUrl.empty()) s_qrUrl = s_link;
    if (s_deviceCode.empty()) { Error("Server did not provide a device code"); return; }
    s_accountState = 1;
}

static void PollLogin() {
    if (s_accountState != 1) return;
    picojson::object payload;
    payload["p_code"] = picojson::value(s_deviceCode);
    payload["p_device_nonce"] = picojson::value(s_nonce);
    picojson::value response;
    if (!Api("POST", "/rest/v1/rpc/poll_tv_login_session", "",
             picojson::value(payload).serialize(), response)) return;
    const std::string status = AsString(Field(FirstRow(response), "status"));
    if (status == "pending") return;
    if (status != "approved") { s_accountState = 0; Error("Device code expired. Please sign in again."); return; }
    picojson::object exchange;
    exchange["code"] = picojson::value(s_deviceCode);
    exchange["device_nonce"] = picojson::value(s_nonce);
    picojson::value tokens;
    if (!Api("POST", "/functions/v1/tv-logins-exchange", "",
             picojson::value(exchange).serialize(), tokens)) return;
    s_access = AsString(Field(tokens, "access_token"));
    s_refresh = AsString(Field(tokens, "refresh_token"));
    if (s_access.empty() || s_refresh.empty()) { Error("Login approved, but session tokens are missing"); return; }
    // Prefer the embedded user, like Auth.importTokenResponse in NuvioTV.
    // Persist before an optional network request: pairing codes are single-use.
    s_email = AsString(Field(Field(tokens, "user"), "email"));
    TokenDeadline(AsInt(Field(tokens, "expires_in"), 3600));
    s_anon.clear(); s_nonce.clear(); s_deviceCode.clear();
    s_userCode.clear(); s_qrUrl.clear();
    s_accountState = 2;
    s_error.clear();
    SaveSession();
    if (s_email.empty()) {
        picojson::value user;
        if (Api("GET", "/auth/v1/user", s_access, "", user)) {
            s_email = AsString(Field(user, "email"));
            SaveSession();
        }
    }
}

static std::string AccountStatus() {
    std::string out = "STATUS\t";
    out += s_accountState == 1 ? "pending\n" : (s_accountState == 2 && !s_access.empty() ? "signed_in\n" : "signed_out\n");
    out += "EMAIL\t" + SafeText(s_email, 63) + "\n";
    out += "CODE\t" + SafeText(s_userCode, 30) + "\n";
    std::string link = s_link;
    if (link.find("https://") == 0) link.erase(0, 8);
    out += "LINK\t" + SafeText(link, 63) + "\n";
    out += "ERROR\t" + SafeText(s_error, 120) + "\n";
    return out;
}

static std::string QrBytes() {
    if (s_accountState != 1 || s_qrUrl.empty()) return "";
    uint8_t temp[qrcodegen_BUFFER_LEN_MAX], qr[qrcodegen_BUFFER_LEN_MAX];
    if (!qrcodegen_encodeText(s_qrUrl.c_str(), temp, qr, qrcodegen_Ecc_MEDIUM,
                             qrcodegen_VERSION_MIN, 40, qrcodegen_Mask_AUTO, true)) return "";
    const int core = qrcodegen_getSize(qr);
    const int quiet = 4; // ISO QR codes require a four-module white quiet zone.
    const int total = core + quiet * 2;
    if (total > 80) return ""; // Cap at 80 modules for display
    std::string result = "NVQM";
    result += (char)total;
    for (int y = -quiet; y < core + quiet; ++y)
        for (int x = -quiet; x < core + quiet; ++x)
            result += (char)(x >= 0 && y >= 0 && x < core && y < core &&
                qrcodegen_getModule(qr, x, y) ? 1 : 0);
    return result;
}

static std::string Profiles() {
    if (!EnsureAuth()) return std::string();
    for (int i = 0; i < 7; ++i) s_profileAllowed[i] = false;
    picojson::value data;
    if (!Api("POST", "/rest/v1/rpc/sync_pull_profiles", s_access, "{}", data) ||
        !data.is<picojson::array>()) return "";
    std::string out;
    for (int i = 0; i < 7; ++i) s_avatarUrls[i].clear();
    const picojson::array &rows = data.get<picojson::array>();
    picojson::value catalog;
    const bool gotAvatarCatalog = Api("POST", "/rest/v1/rpc/get_avatar_catalog", s_access, "{}", catalog)
                               && catalog.is<picojson::array>();
    // Match NuvioTV's lock check: do not silently bypass a profile PIN.
    picojson::value locks;
    if (!Api("POST", "/rest/v1/rpc/sync_pull_profile_locks", s_access, "{}", locks) ||
        !locks.is<picojson::array>()) {
        Error("Cannot check profile PIN locks. Try again later.");
        return "";
    }
    for (size_t i = 0; i < rows.size() && i < 6; ++i) {
        const picojson::value &p = rows[i];
        const int id = AsInt(Field(p, "profile_index"), -1);
        const std::string name = AsString(Field(p, "name"));
        if (id < 1 || id > 6 || name.empty()) continue;
        std::string avatarUrl = AsString(Field(p, "avatar_url"));
        const std::string avatarId = AsString(Field(p, "avatar_id"));
        if (avatarUrl.empty() && !avatarId.empty() && gotAvatarCatalog) {
            const picojson::array &a = catalog.get<picojson::array>();
            for (size_t j = 0; j < a.size(); ++j) {
                if (AsString(Field(a[j], "id")) == avatarId) {
                    avatarUrl = AsString(Field(a[j], "image_url"));
                    if (avatarUrl.empty()) avatarUrl = AsString(Field(a[j], "storage_path"));
                    if (!avatarUrl.empty() && avatarUrl.find("http") != 0)
                        avatarUrl = std::string(kApi) + "/storage/v1/object/public/avatars/" + avatarUrl;
                    break;
                }
            }
        }
        s_avatarUrls[id] = avatarUrl;
        s_primaryAddons[id] = id != 1 && Field(p, "uses_primary_addons").is<bool>() &&
            Field(p, "uses_primary_addons").get<bool>();
        int color = (id - 1) % 6;
        int locked = 0;
        const picojson::array &lockRows = locks.get<picojson::array>();
        for (size_t j = 0; j < lockRows.size(); ++j) {
            if (AsInt(Field(lockRows[j], "profile_index"), -1) == id &&
                Field(lockRows[j], "pin_enabled").is<bool>()) {
                locked = Field(lockRows[j], "pin_enabled").get<bool>() ? 1 : 0;
            }
        }
        char line[24]; std::sprintf(line, "%d\t%d\t%d\t", id, color, locked);
        s_profileAllowed[id] = !locked;
        out += line + SafeText(name, 23) + "\n";
    }
    return out;
}

static std::string Library(int profileIndex) {
    if (!EnsureAuth() || profileIndex < 1 || profileIndex > 6) return "";
    char args[90]; std::sprintf(args, "{\"p_profile_id\":%d,\"p_limit\":50,\"p_offset\":0}", profileIndex);
    picojson::value data;
    if (!Api("POST", "/rest/v1/rpc/sync_pull_library", s_access, args, data) ||
        !data.is<picojson::array>()) return "";
    std::string out;
    const picojson::array &a = data.get<picojson::array>();
    for (size_t i = 0; i < a.size() && i < 50; ++i) {
        const picojson::value &p = a[i];
        const std::string id = AsString(Field(p, "content_id"));
        const std::string name = AsString(Field(p, "name"));
        if (id.empty() || name.empty()) continue;
        out += SafeText(id, 30) + "\t" + SafeText(name, 90) + "\n";
    }
    return out;
}

static std::string AddLibrary(int profileIndex, const std::string &imdbId) {
    if (!EnsureAuth() || profileIndex < 1 || profileIndex > 6 ||
        imdbId.empty() || imdbId.size() >= 128) return "";
    const Art *item = Artwork(imdbId);
    if (!item) return "";
    picojson::object record;
    record["content_id"] = picojson::value(imdbId);
    record["content_type"] = picojson::value(item->kind);
    record["name"] = picojson::value(item->title);
    record["poster"] = picojson::value(item->poster);
    record["poster_shape"] = picojson::value("POSTER");
    record["background"] = picojson::value(item->background);
    record["description"] = picojson::value("");
    record["release_info"] = picojson::value("");
    record["imdb_rating"] = picojson::value();
    record["genres"] = picojson::value(picojson::array());
    record["addon_base_url"] = picojson::value(item->base);
    record["added_at"] = picojson::value((double)std::time(NULL) * 1000.0);
    picojson::array entries; entries.push_back(picojson::value(record));
    picojson::object request;
    request["p_items"] = picojson::value(entries);
    request["p_profile_id"] = picojson::value((double)profileIndex);
    picojson::value reply;
    if (!Api("POST", "/rest/v1/rpc/sync_push_library_items", s_access,
             picojson::value(request).serialize(), reply)) return "";
    return "OK\n";
}

static std::string LibraryCatalog() {
    if (!EnsureAuth() || s_profile < 1) return "";
    char args[90]; std::sprintf(args, "{\"p_profile_id\":%d,\"p_limit\":240,\"p_offset\":0}", s_profile);
    picojson::value data;
    if (!Api("POST", "/rest/v1/rpc/sync_pull_library", s_access, args, data) ||
        !data.is<picojson::array>()) return "";
    const picojson::array &rows = data.get<picojson::array>();
    std::vector<Art> art;
    std::string out = "#catalog\n";
    for (size_t i = 0; i < rows.size() && art.size() < 240; ++i) {
        const picojson::value &p = rows[i];
        Art item;
        item.id = AsString(Field(p, "content_id"));
        item.kind = AsString(Field(p, "content_type"));
        item.title = AsString(Field(p, "name"));
        if (item.id.empty() || item.id.size() >= 128 || item.title.empty() ||
            (item.kind != "movie" && item.kind != "series")) continue;
        item.poster = AsString(Field(p, "poster"));
        item.background = AsString(Field(p, "background"));
        item.base = AsString(Field(p, "addon_base_url"));
        if (item.base.empty()) item.base = kDefaultCatalog;
        item.description = AsString(Field(p, "description"));
        if (art.size() % 20 == 0) out += "#row\tMy Library\n";
        out += SafeText(item.id, 127) + "\t" + item.kind + "\t0\t" + SafeText(item.title, 71) + "\n";
        art.push_back(item);
    }
    s_art.swap(art);
    return out;
}

struct VideoEntry { std::string id, name; };
static std::vector<VideoEntry> s_videos;
static std::string Videos(const std::string &id) {
    Art *item = Artwork(id);
    s_videos.clear();
    if (!item) return "";
    if (item->kind == "movie") return "#movie\n";
    picojson::value reply;
    if (!HttpJson(AddonUrl(item->base, "/meta/" + item->kind + "/" + Encode(id) + ".json"), reply)) return "";
    const picojson::value &values = Field(Field(reply, "meta"), "videos");
    if (!values.is<picojson::array>()) return "#episodes\n";
    const picojson::array &a = values.get<picojson::array>();
    std::string out = "#episodes\n";
    for (size_t i = 0; i < a.size() && s_videos.size() < 240; ++i) {
        VideoEntry v;
        v.id = AsString(Field(a[i], "id"));
        if (v.id.empty()) continue;
        char label[40]; std::sprintf(label, "S%02d E%02d - ", AsInt(Field(a[i],"season"),0), AsInt(Field(a[i],"episode"),0));
        v.name = std::string(label) + AsString(Field(a[i], "title"));
        out += SafeText(v.name, 100) + "\n"; s_videos.push_back(v);
    }
    return out;
}

static bool Provides(const picojson::value &manifest, const char *resource, const std::string &type) {
    const picojson::value &resources = Field(manifest, "resources");
    if (!resources.is<picojson::array>()) return false;
    const picojson::array &a = resources.get<picojson::array>();
    for (size_t i = 0; i < a.size(); ++i) {
        if (AsString(a[i]) == resource) return true;
        if (AsString(Field(a[i], "name")) != resource) continue;
        const picojson::value &types = Field(a[i], "types");
        if (!types.is<picojson::array>()) return true;
        const picojson::array &t = types.get<picojson::array>();
        for (size_t j = 0; j < t.size(); ++j) if (AsString(t[j]) == type) return true;
    }
    return false;
}

// Known incompatible sources should fail before metadata download/decoder allocation.
static bool UnsupportedVideo(const picojson::value &stream) {
    std::string hint=AsString(Field(stream,"name"))+" "+AsString(Field(stream,"title"))+" "+AsString(Field(stream,"description"));
    for(size_t i=0;i<hint.size();++i)if(hint[i]>='A'&&hint[i]<='Z')hint[i]+='a'-'A';
    const char *tokens[]={"2160","1080","4k","8k","hevc","h.265","h265","x265","av1","dolby vision","hdr"};
    for(size_t i=0;i<sizeof(tokens)/sizeof(tokens[0]);++i)if(hint.find(tokens[i])!=std::string::npos)return true;
    return false;
}

static std::string Sources(const std::string &id, int episode) {
    s_sources.clear();
    Art *item = Artwork(id);
    if (!item || !EnsureAuth()) return "";
    const std::string video = item->kind == "movie" ? id :
        episode >= 0 && episode < (int)s_videos.size() ? s_videos[episode].id : "";
    if (video.empty()) { Error("Choose an episode first."); return ""; }
    std::string out = "#sources\n";
    int count = 0, failed = 0, queried = 0;
    for (size_t i = 0; i < s_addons.size() && count < 64; ++i) {
        CloudAddon &a = s_addons[i];
        if (!a.enabled) continue;
        LoadManifest(a);
        if (!Provides(a.manifest, "stream", item->kind)) continue;
        ++queried;
        const std::string base = Canonical(a.base);
        if (base.empty()) { ++failed; continue; }
        picojson::value reply;
        if (!HttpJson(AddonUrl(base, "/stream/" + item->kind + "/" + Encode(video) + ".json"), reply)) {
            ++failed; continue;
        }
        const picojson::value &values = Field(reply, "streams");
        if (!values.is<picojson::array>()) { ++failed; continue; }
        const picojson::array &streams = values.get<picojson::array>();
        for (size_t j = 0; j < streams.size() && count < 64; ++j) {
            const picojson::value &stream = streams[j];
            std::string label = AsString(Field(stream, "name"));
            if (label.empty()) label = a.name.empty() ? AsString(Field(a.manifest,"name")) : a.name;
            std::string desc = AsString(Field(stream, "title"));
            if (desc.empty()) desc = AsString(Field(stream, "description"));
            const bool torrent = !AsString(Field(stream, "infoHash")).empty() ||
                !AsString(Field(Field(stream,"clientResolve"),"infoHash")).empty() ||
                !AsString(Field(Field(stream,"clientResolve"),"magnetUri")).empty() ||
                AsString(Field(stream,"url")).find("magnet:")==0 ||
                AsString(Field(stream,"url")).find("torrent://")==0 ||
                AsString(Field(stream,"externalUrl")).find("magnet:")==0 ||
                AsString(Field(stream,"externalUrl")).find("torrent://")==0;
            const std::string url = AsString(Field(stream, "url"));
            if (!torrent && url.empty() && AsString(Field(stream, "externalUrl")).empty()) continue;
            // Source URLs and provider keys remain off the television and logs.
            out += SafeText(label, 50) + " | " + SafeText(desc, 85) +
                (UnsupportedVideo(stream)?" [Requer conversao para 720p H.264]":torrent ? " [Torrent]" : " [Video]") + "\n";
            ++count;
            s_sources.push_back(stream);
        }
    }
    if (!count && failed && failed == queried) { Error("Stream addons unavailable. Retry with X."); return ""; }
    return out;
}

static bool HttpJson(const std::string &url, picojson::value &v) {
    const Http::Response r = Http::Get(url, "User-Agent: NUVIO/0.7\r\nAccept: application/json\r\n", 14000);
    if (r.status != 200) { Error("Movie catalog is unavailable over HTTPS"); return false; }
    return Parse(r.body, v);
}

static std::string Encode(const std::string &s) {
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = (unsigned char)s[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') out += c;
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}

static std::string Decode(const std::string &s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            char h[3] = {s[i+1], s[i+2], 0}; char *end = NULL;
            const long value = std::strtol(h, &end, 16);
            if (*end) return "";
            out += (char)value; i += 2;
        } else out += s[i] == '+' ? ' ' : s[i];
    }
    return out;
}

// Preserve configured paths, percent escapes and query parameters verbatim.
static std::string Canonical(const std::string &input) {
    std::string u = input;
    if (u.compare(0, 10, "stremio://") == 0) u = "https://" + u.substr(10);
    if (u.size() > 2048 || u.compare(0, 8, "https://") != 0 ||
        u.find_first_of("\r\n\t #") != std::string::npos) return "";
    const size_t authorityEnd = u.find_first_of("/?", 8);
    const std::string authority = u.substr(8, authorityEnd == std::string::npos ?
        std::string::npos : authorityEnd - 8);
    if (authority.empty() || authority.find('@') != std::string::npos) return "";
    const size_t q = u.find('?');
    std::string path = u.substr(0, q);
    while (!path.empty() && path[path.size()-1] == '/') path.erase(path.size()-1);
    const std::string suffix = "/manifest.json";
    if (path.size() >= suffix.size() && path.substr(path.size()-suffix.size()) == suffix)
        path.erase(path.size()-suffix.size());
    return path + (q == std::string::npos ? "" : u.substr(q));
}

static std::string AddonUrl(const std::string &base, const std::string &path) {
    const size_t q = base.find('?');
    return base.substr(0, q) + path + (q == std::string::npos ? "" : base.substr(q));
}

static bool SelectProfile(int id) {
    if (!EnsureAuth() || id < 1 || id > 6 || !s_profileAllowed[id]) {
        Error("Select an unlocked Nuvio cloud profile first."); return false;
    }
    s_profile = id;
    s_addonProfile = s_primaryAddons[id] ? 1 : id;
    s_addonsLoaded = s_addonsComplete = false;
    s_addons.clear(); s_art.clear();
    return true;
}

static bool PullAddons() {
    if (!EnsureAuth() || s_profile < 1) return false;
    if (s_owner.empty()) {
        picojson::value owner;
        if (!Api("POST", "/rest/v1/rpc/get_sync_owner", s_access, "{}", owner)) return false;
        s_owner = AsString(owner);
        if (s_owner.empty()) { Error("Could not resolve Nuvio sync owner."); return false; }
    }
    char number[16]; std::sprintf(number, "%d", s_addonProfile);
    picojson::value rows;
    const std::string path = "/rest/v1/addons?select=url,name,enabled,sort_order&user_id=eq." +
        Encode(s_owner) + "&profile_id=eq." + number + "&order=sort_order.asc&limit=65";
    if (!Api("GET", path, s_access, "", rows) || !rows.is<picojson::array>()) return false;
    const picojson::array &a = rows.get<picojson::array>();
    std::vector<CloudAddon> loaded;
    for (size_t i = 0; i < a.size(); ++i) {
        CloudAddon addon;
        // Keep even unsupported remote URLs: a later push must never delete them.
        addon.base = AsString(Field(a[i], "url"));
        addon.name = AsString(Field(a[i], "name"));
        addon.enabled = !Field(a[i], "enabled").is<bool>() || Field(a[i], "enabled").get<bool>();
        if (addon.base.empty()) { Error("Invalid cloud addon record. Changes blocked."); return false; }
        for (size_t j = 0; j < s_addons.size(); ++j)
            if (s_addons[j].base == addon.base) addon.manifest = s_addons[j].manifest;
        loaded.push_back(addon);
    }
    s_addons.swap(loaded);
    s_addonsLoaded = true;
    s_addonsComplete = a.size() <= 64;
    return true;
}

static void LoadManifest(CloudAddon &a) {
    if (a.manifest.is<picojson::object>()) return;
    const std::string base = Canonical(a.base);
    if (base.empty()) return;
    picojson::value manifest;
    if (HttpJson(AddonUrl(base, "/manifest.json"), manifest) &&
        !AsString(Field(manifest, "id")).empty()) a.manifest = manifest;
}

static std::string Addons() {
    if (!PullAddons()) return "";
    std::string out = "#cloud\n"; // valid empty remote lists must clear old rows
    for (size_t i = 0; i < s_addons.size() && i < 64; ++i) {
        CloudAddon &a = s_addons[i];
        LoadManifest(a);
        std::string name = a.name.empty() ? AsString(Field(a.manifest, "name")) : a.name;
        if (name.empty()) name = "Addon " + AsString(picojson::value((double)i + 1));
        std::string desc = s_profile != s_addonProfile ? "Inherited from primary profile" :
            AsString(Field(a.manifest, "description"));
        if (!a.manifest.is<picojson::object>()) desc = "Manifest unavailable; URL preserved in cloud";
        char prefix[32]; std::sprintf(prefix, "%d\t%d\t", (int)i+1, a.enabled ? 1 : 0);
        out += prefix + SafeText(name, 39) + "\t" + SafeText(desc, 71) + "\t" + a.base + "\n";
    }
    return out;
}

static bool PushAddons(const std::vector<CloudAddon> &next) {
    if (!s_addonsComplete || s_profile != s_addonProfile) {
        Error("Manage inherited addons in the primary profile. Large lists are read-only."); return false;
    }
    picojson::array a;
    for (size_t i = 0; i < next.size(); ++i) {
        picojson::object entry;
        entry["url"] = picojson::value(next[i].base);
        entry["sort_order"] = picojson::value((double)i);
        entry["enabled"] = picojson::value(next[i].enabled);
        if (!next[i].name.empty()) entry["name"] = picojson::value(next[i].name);
        a.push_back(picojson::value(entry));
    }
    picojson::object args;
    args["p_profile_id"] = picojson::value((double)s_profile);
    args["p_addons"] = picojson::value(a);
    picojson::value reply;
    if (!Api("POST", "/rest/v1/rpc/sync_push_addons", s_access,
        picojson::value(args).serialize(), reply)) return false;
    s_addons = next;
    return true;
}

static std::string ChangeAddon(const std::string &operation, int id, const std::string &url) {
    // Re-read before every mutation; keep disabled and unsupported remote entries.
    if (id > 0 && (!s_addonsLoaded || id > (int)s_addons.size())) return "";
    const std::string target = id > 0 ? s_addons[id-1].base : "";
    if (!PullAddons()) return "";
    std::vector<CloudAddon> next = s_addons;
    if (operation == "add") {
        const std::string base = Canonical(url);
        if (base.empty()) { Error("Use a valid HTTPS addon manifest URL."); return ""; }
        if (next.size() >= 64) { Error("Addon limit reached."); return ""; }
        for (size_t i = 0; i < next.size(); ++i)
            if (Canonical(next[i].base) == base) { Error("Addon already installed."); return ""; }
        CloudAddon a; a.base = base; a.enabled = true;
        LoadManifest(a);
        if (!a.manifest.is<picojson::object>()) { Error("Could not load addon manifest."); return ""; }
        next.push_back(a);
    } else {
        size_t i = 0;
        for (; i < next.size(); ++i) if (next[i].base == target) break;
        if (i == next.size()) { Error("Addon changed in cloud. Refresh the list."); return ""; }
        if (operation == "toggle") next[i].enabled = !next[i].enabled;
        else next.erase(next.begin() + i);
    }
    return PushAddons(next) ? "OK\n" : "";
}

static bool HasExtra(const picojson::value &catalog, const char *name) {
    const picojson::value &extra = Field(catalog, "extra");
    if (extra.is<picojson::array>()) {
        const picojson::array &a = extra.get<picojson::array>();
        for (size_t i = 0; i < a.size(); ++i)
            if (AsString(a[i]) == name || AsString(Field(a[i], "name")) == name) return true;
    }
    const picojson::value &supported = Field(catalog, "extraSupported");
    if (supported.is<picojson::array>()) {
        const picojson::array &a = supported.get<picojson::array>();
        for (size_t i = 0; i < a.size(); ++i) if (AsString(a[i]) == name) return true;
    }
    return false;
}

static bool RequiresFilter(const picojson::value &catalog, bool search) {
    const picojson::value &required = Field(catalog, "extraRequired");
    if (required.is<picojson::array>()) {
        const picojson::array &r = required.get<picojson::array>();
        for (size_t i = 0; i < r.size(); ++i)
            if (!search || AsString(r[i]) != "search") return true;
    }
    const picojson::value &extra = Field(catalog, "extra");
    if (!extra.is<picojson::array>()) return false;
    const picojson::array &a = extra.get<picojson::array>();
    for (size_t i = 0; i < a.size(); ++i)
        if (Field(a[i], "isRequired").is<bool>() && Field(a[i], "isRequired").get<bool>() &&
            (!search || AsString(Field(a[i], "name")) != "search")) return true;
    return false;
}

static std::string Catalog(int page, const std::string &query) {
    if (s_profile < 1 || !EnsureAuth()) return "";
    if (!s_addonsLoaded && !PullAddons()) return "";
    // The default catalog is a local fallback, never silently pushed to the account.
    std::vector<CloudAddon> sources = s_addons;
    bool hasCatalog = false, officialInstalled = false;
    for (size_t i = 0; i < sources.size(); ++i) {
        if (Canonical(sources[i].base).find(kDefaultCatalog) == 0) officialInstalled = true;
        if (!sources[i].enabled) continue;
        LoadManifest(sources[i]);
        const picojson::value &cs = Field(sources[i].manifest, "catalogs");
        if (cs.is<picojson::array>() && !cs.get<picojson::array>().empty()) hasCatalog = true;
    }
    if (sources.empty() || (!hasCatalog && !officialInstalled)) {
        CloudAddon a; a.base = kDefaultCatalog; a.enabled = true; sources.push_back(a);
    }
    std::vector<Art> art;
    std::string out = "#catalog\n";
    int rails = 0, attempted = 0, succeeded = 0;
    const bool search = !query.empty();
    for (size_t source = 0; source < sources.size() && rails < 24 && art.size() < 240; ++source) {
        CloudAddon &addon = sources[source];
        if (!addon.enabled) continue;
        LoadManifest(addon);
        const std::string base = Canonical(addon.base);
        const picojson::value &catalogs = Field(addon.manifest, "catalogs");
        if (base.empty() || !catalogs.is<picojson::array>()) continue;
        const picojson::array &cs = catalogs.get<picojson::array>();
        for (size_t c = 0; c < cs.size() && rails < 24 && art.size() < 240; ++c) {
            const picojson::value &descriptor = cs[c];
            const std::string type = AsString(Field(descriptor, "type"));
            const std::string id = AsString(Field(descriptor, "id"));
            if ((type != "movie" && type != "series") || id.empty() || RequiresFilter(descriptor, search)) continue;
            if (search && !HasExtra(descriptor, "search")) continue;
            if (!search && Field(descriptor, "showInHome").is<bool>() &&
                !Field(descriptor, "showInHome").get<bool>()) continue;
            // Page through descriptor groups as well as their items to keep boot bounded.
            if (!search && attempted++ / 8 != page) continue;
            if (search && attempted++ >= 8) break;
            std::string path = "/catalog/" + Encode(type) + "/" + Encode(id);
            if (search) path += "/search=" + Encode(query);
            path += ".json";
            picojson::value data;
            if (!HttpJson(AddonUrl(base, path), data)) continue;
            const picojson::value &metas = Field(data, "metas");
            if (!metas.is<picojson::array>()) continue;
            ++succeeded;
            const picojson::array &a = metas.get<picojson::array>();
            std::string row;
            for (size_t i = 0; i < a.size() && i < 20 && art.size() < 240; ++i) {
                const picojson::value &item = a[i];
                Art entry;
                entry.id = AsString(Field(item, "id"));
                entry.title = AsString(Field(item, "name"));
                entry.kind = type; entry.base = base;
                if (entry.id.empty() || entry.id.size() >= 128 || entry.title.empty() ||
                    entry.id.find_first_of("\r\n\t") != std::string::npos) continue;
                entry.poster = AsString(Field(item, "poster"));
                entry.background = AsString(Field(item, "background"));
                entry.description = AsString(Field(item, "description"));
                std::string year = AsString(Field(item, "year"));
                if (year.empty()) year = AsString(Field(item, "releaseInfo"));
                int yr = std::atoi(year.c_str());
                if (yr < 1888 || yr > 2100) yr = 0;
                char num[16]; std::sprintf(num, "%d", yr);
                row += entry.id + "\t" + type + "\t" + num + "\t" + SafeText(entry.title, 71) + "\n";
                art.push_back(entry);
            }
            if (!row.empty()) {
                std::string label = AsString(Field(descriptor, "name"));
                if (label.empty()) label = id;
                out += "#row\t" + SafeText(label, 71) + "\n" + row;
                ++rails;
            }
        }
        // Save fetched manifests without changing account URL/name/enabled state.
        if (source < s_addons.size()) s_addons[source].manifest = addon.manifest;
    }
    if (!succeeded && attempted > 0) { Error("Addon catalogs unavailable. Press X to retry."); return ""; }
    s_art.swap(art);
    return out;
}

static Art *Artwork(const std::string &id) {
    for (size_t i = 0; i < s_art.size(); ++i) if (s_art[i].id == id) return &s_art[i];
    return NULL;
}

static std::string Image(const std::string &url, int width, int height, bool roundAvatar) {
    if (url.compare(0, 8, "https://") != 0) return "";
#ifndef NUVIO_BACKEND_TEST
    MEMORYSTATUS memory; GlobalMemoryStatus(&memory);
    if(memory.dwAvailPhys<32*1024*1024)return "";
#endif
    std::string resized=url;
    if (url.find("https://image.tmdb.org/t/p/")==0) {
        const size_t begin=27, slash=url.find('/',begin);
        if (slash!=std::string::npos)
            resized=url.substr(0,begin)+(width>400?"w780":"w185")+url.substr(slash);
    }
    const Http::Response result = Http::Get(resized, "User-Agent: NUVIO\r\n", 4500);
    if (result.status != 200 || result.body.empty() || result.body.size() > 1500000) return "";
    int iw = 0, ih = 0, channels = 0;
    if (!stbi_info_from_memory((const stbi_uc*)result.body.data(),(int)result.body.size(),&iw,&ih,&channels) ||
        iw<=0 || ih<=0 || iw>2048 || ih>2048 || iw>2000000/ih) return "";
    unsigned char *rgba = stbi_load_from_memory((const stbi_uc*)result.body.data(),
                       (int)result.body.size(), &iw, &ih, &channels, 4);
    if (!rgba || iw <= 0 || ih <= 0 || iw > 4096 || ih > 4096) { stbi_image_free(rgba); return ""; }
    std::string data;
    data.resize(8 + width * height * 4);
    data[0] = 'N'; data[1] = 'V'; data[2] = 'P'; data[3] = 'O';
    data[4] = (char)width; data[5] = (char)(width >> 8);
    data[6] = (char)height; data[7] = (char)(height >> 8);
    unsigned char *dst = (unsigned char*)&data[8];
    for (int y = 0; y < height; ++y) {
        const int sy = (y * ih) / height;
        for (int x = 0; x < width; ++x) {
            const int sx = (x * iw) / width;
            std::memcpy(dst + (y * width + x) * 4, rgba + (sy * iw + sx) * 4, 4);
            if (roundAvatar) {
                const int dx = 2 * x + 1 - width;
                const int dy = 2 * y + 1 - height;
                if (dx * dx + dy * dy > width * width)
                    dst[(y * width + x) * 4 + 3] = 0;
            }
        }
    }
    stbi_image_free(rgba);
    return data;
}

static std::string Preview(const std::string &id, bool bitmap) {
    Art *a = Artwork(id);
    if (!a) return "";
    // Keep metadata for one focused title, rather than every visited series.
    if(!a->metadata.is<picojson::object>())
        for(size_t i=0;i<s_art.size();++i)if(&s_art[i]!=a)s_art[i].metadata=picojson::value();
    picojson::value metadata;
    const std::string url = AddonUrl(a->base, "/meta/" + a->kind + "/" + Encode(id) + ".json");
    if(a->metadata.is<picojson::object>())metadata=a->metadata;
    else if (!HttpJson(url, metadata)) {
        if (bitmap) return Image(a->background, 640, 360, false);
        return "\n\n\n" + SafeText(a->description, 380) + "\n";
    }
    a->metadata=metadata;
    const picojson::value &m = Field(metadata, "meta");
    if (bitmap) {
        std::string background = AsString(Field(m, "background"));
        if (background.empty()) background = a->background;
        return Image(background, 640, 360, false);
    }
    std::string genres;
    const picojson::value &g = Field(m, "genres").is<picojson::array>()?Field(m,"genres"):Field(m, "genre");
    if (g.is<picojson::array>()) {
        const picojson::array &values = g.get<picojson::array>();
        for (size_t i = 0; i < values.size() && i < 3; ++i) {
            if (i) genres += " / ";
            genres += AsString(values[i]);
        }
    }
    const std::string runtime = AsString(Field(m, "runtime"));
    const std::string rating = AsString(Field(m, "imdbRating"));
    const std::string description = AsString(Field(m, "description"));
    return SafeText(genres, 90) + "\n" + SafeText(runtime, 25) + "\n" +
           SafeText(rating, 16) + "\n" + SafeText(description, 380) + "\n";
}

} // namespace

bool NuvioNativePlaybackSource(int index,NuvioPlaybackSource *source) {
    if(!source || index<0 || index>=(int)s_sources.size())return false;
    const picojson::value &stream=s_sources[index];
    source->url=AsString(Field(stream,"url"));
    source->name=AsString(Field(stream,"name"));
    source->headers.clear();source->prepareBody.clear();
    const picojson::value &resolve=Field(stream,"clientResolve");
    std::string hash=AsString(Field(stream,"infoHash"));
    if(hash.empty())hash=AsString(Field(resolve,"infoHash"));
    std::string magnet=AsString(Field(resolve,"magnetUri"));
    const std::string external=AsString(Field(stream,"externalUrl"));
    if(source->url.find("magnet:?")==0)magnet=source->url;
    else if(source->url.empty()&&external.find("magnet:?")==0)magnet=external;
    // Nuvio streams may encode a v1 hash and zero-based file index as:
    // torrent://<40-hex-or-32-base32-hash>/<fileIdx>
    int torrentFileIndex=-1;
    std::string torrentUri=source->url.find("torrent://")==0?source->url:source->url.empty()?external:"";
    if(torrentUri.find("torrent://")==0) {
        std::string path=torrentUri.substr(10);
        size_t sep=path.find_first_of("/?");
        std::string id=path.substr(0,sep);
        bool valid=id.size()==40||id.size()==32;
        for(size_t c=0;c<id.size()&&valid;++c) {
            char x=id[c];
            if(id.size()==40)valid=(x>='0'&&x<='9')||(x>='a'&&x<='f')||(x>='A'&&x<='F');
            else valid=(x>='2'&&x<='7')||(x>='a'&&x<='z')||(x>='A'&&x<='Z');
        }
        if(!valid){Error("Hash torrent:// invalido.");return false;}
        if(hash.empty())hash=id;
        if(sep!=std::string::npos&&path[sep]=='/') {
            std::string selected=path.substr(sep+1);
            selected=selected.substr(0,selected.find('?'));
            char *end=NULL;long idx=std::strtol(selected.c_str(),&end,10);
            if(selected.empty()||*end||idx<0||idx>2047){Error("Indice torrent:// invalido.");return false;}
            torrentFileIndex=(int)idx;
        }
    }
    if(magnet.empty()&&!hash.empty())magnet="magnet:?xt=urn:btih:"+hash;
    if(!magnet.empty()) {
        std::string server;
        const char *paths[]={"game:\\nuvio.cfg","D:\\nuvio.cfg","nuvio.cfg"};
        for(int i=0;i<3&&server.empty();++i) {
            std::FILE *f=std::fopen(paths[i],"rb");if(!f)continue;
            char line[2200];
            while(std::fgets(line,sizeof(line),f))if(!std::strncmp(line,"torrent_server=",15)) {
                server=line+15;
                while(!server.empty()&&(server[server.size()-1]=='\r'||server[server.size()-1]=='\n'||server[server.size()-1]==' '))
                    server.erase(server.size()-1);
            }
            std::fclose(f);
        }
        while(!server.empty()&&server[server.size()-1]=='/')server.erase(server.size()-1);
        if(!server.empty()&&((server.find("https://")!=0&&server.find("http://")!=0)||server.find_first_of("\r\n\t @?#")!=std::string::npos)) {
            Error("Endereco torrent_server invalido em nuvio.cfg.");return false;
        }
        int file=AsInt(Field(stream,"fileIdx"),AsInt(Field(resolve,"fileIdx"),-1));
        if(file<0)file=torrentFileIndex;
        const picojson::value *lists[]={&Field(stream,"sources"),&Field(resolve,"sources")};
        for(int l=0;l<2;++l)if(lists[l]->is<picojson::array>()) {
            const picojson::array &items=lists[l]->get<picojson::array>();
            for(size_t i=0;i<items.size()&&i<12;++i) {
                std::string tracker=AsString(items[i]);
                if(tracker.find("tracker:")==0)tracker.erase(0,8);
                if((tracker.find("udp://")==0||tracker.find("http://")==0||tracker.find("https://")==0)&&tracker.size()<2048)
                    magnet+="&tr="+Encode(tracker);
            }
        }
        char suffix[40]="";
        if(file>=0)std::sprintf(suffix,"&index=%d",file+1);
        if(server.empty()) {
            if(file>=0&&magnet.find("&so=")==std::string::npos){std::sprintf(suffix,"&so=%d",file);magnet+=suffix;}
            source->url=magnet;
        } else source->url=server+"/stream/nuvio?link="+Encode(magnet)+suffix+"&play";
    } else {
        const picojson::value &headers=Field(Field(Field(stream,"behaviorHints"),"proxyHeaders"),"request");
        if(headers.is<picojson::object>()) {
            const picojson::object &h=headers.get<picojson::object>();
            for(picojson::object::const_iterator it=h.begin();it!=h.end();++it) {
                std::string name=it->first,value=AsString(it->second);
                if(name.empty()||name.size()>64||value.size()>4096||
                    name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-")!=std::string::npos||
                    value.find_first_of("\r\n")!=std::string::npos)continue;
                std::string lower=name;
                for(size_t j=0;j<lower.size();++j)if(lower[j]>='A'&&lower[j]<='Z')lower[j]+='a'-'A';
                if(lower=="host"||lower=="range"||lower=="connection"||lower=="content-length")continue;
                if(source->headers.size()+name.size()+value.size()+4>8192)break;
                source->headers+=name+": "+value+"\r\n";
            }
        }
    }
    if(source->url.size()>16384 || (source->url.find("http://")!=0&&source->url.find("https://")!=0&&source->url.find("magnet:?")!=0) ||
        source->url.find_first_of("\r\n")!=std::string::npos) {
        Error("Esse addon nao retornou uma fonte de video HTTP compativel.");return false;
    }
    std::string converter,token,mode="auto";
    const char *configPaths[]={"game:\\nuvio.cfg","D:\\nuvio.cfg","nuvio.cfg"};
    for(int p=0;p<3&&converter.empty();++p) {
        std::FILE *cfg=std::fopen(configPaths[p],"rb");if(!cfg)continue;
        char buffer[2200];
        while(std::fgets(buffer,sizeof(buffer),cfg)) {
            std::string line=buffer;
            while(!line.empty()&&(line[line.size()-1]=='\r'||line[line.size()-1]=='\n'||line[line.size()-1]==' '))line.erase(line.size()-1);
            if(line.find("transcode_server=")==0)converter=line.substr(17);
            if(line.find("transcode_token=")==0)token=line.substr(16);
            if(line.find("transcode_mode=")==0)mode=line.substr(15);
        }
        std::fclose(cfg);
    }
    if((UnsupportedVideo(stream)||mode=="always")&&!converter.empty()) {
        while(!converter.empty()&&converter[converter.size()-1]=='/')converter.erase(converter.size()-1);
        if((converter.find("http://")!=0&&converter.find("https://")!=0)||converter.find_first_of("\r\n\t @?#")!=std::string::npos||
           token.size()<32||token.size()>256||token.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos) {
            Error("Configure transcode_server e transcode_token validos em nuvio.cfg.");return false;
        }
        picojson::object body;body["url"]=picojson::value(source->url);body["headers"]=picojson::value(source->headers);
        int file=AsInt(Field(stream,"fileIdx"),AsInt(Field(resolve,"fileIdx"),-1));
        if(file>=0)body["file_index"]=picojson::value((double)file);
        source->prepareBody=picojson::value(body).serialize();
        source->url=converter+"/v1/jobs";
        source->headers="Authorization: Bearer "+token+"\r\nContent-Type: application/json\r\n";
    } else if(UnsupportedVideo(stream)) {
        Error("Fonte requer conversao. Configure transcode_server/token ou escolha 720p H.264/AVC sem HDR.");return false;
    }
    return true;
}

bool NuvioNativeStartup() {
    if (s_started) return true;
    std::string ip;
    if (!Http::Startup(s_error, ip)) return false;

    // Some Xenia builds have stubbed XNetRandom with constant bytes. That is
    // unsuitable for TLS session keys or the secret device-login nonce.
    // Never use a real account until the emulator provides actual entropy.
    // Allow override via nuvio.cfg "allow_emulator_rng=1" for development.
    unsigned char first[32], second[32];
    bool rngOk = (XNetRandom(first, sizeof(first)) == 0 &&
                  XNetRandom(second, sizeof(second)) == 0 &&
                  std::memcmp(first, second, sizeof(first)) != 0);

    // Check for emulator RNG override in config
    bool allowEmulatorRng = false;
    const char *cfgPaths[] = { "game:\\nuvio.cfg", "D:\\nuvio.cfg", "nuvio.cfg" };
    for (int i = 0; i < 3 && !allowEmulatorRng; ++i) {
        std::FILE *f = std::fopen(cfgPaths[i], "rb");
        if (f) {
            char line[256];
            while (std::fgets(line, sizeof(line), f)) {
                if (std::strncmp(line, "allow_emulator_rng=1", 20) == 0) {
                    allowEmulatorRng = true;
                    break;
                }
            }
            std::fclose(f);
        }
    }

    if (!rngOk && !allowEmulatorRng) {
        Error("Insecure emulator RNG. Add 'allow_emulator_rng=1' to nuvio.cfg for testing.");
        return false;
    }
    if (!rngOk && allowEmulatorRng) {
        OutputDebugStringA("NUVIO - WARNING: Using insecure emulator RNG (allow_emulator_rng=1)\n");
    }

    s_started = true;
    LoadSession();
    return true;
}

void NuvioNativeShutdown() {
    // Current refresh tokens were already saved at exchange/rotation time.
    s_started = false;
    s_access.clear(); s_refresh.clear(); s_email.clear(); s_sessionPath.clear();
    s_accountState = 0; s_tokenExpires = 0;
    ClearProfileData();
}
const char *NuvioNativeLastError() { return s_error.c_str(); }

bool NuvioNativeGet(const char *path, int maxBody, NuvioHttpData *out) {
    if (!out) return false;
    out->bytes = NULL; out->length = out->status = 0;
    if (!s_started || !path) return false;
    std::string p(path), body;
    if (p == "/account/start") { StartLogin(); body = AccountStatus(); }
    else if (p == "/account/restart") {
        // An expired or failed device code must never leave the user stuck.
        // Keep an already authenticated account; restart only pending logins.
        if (s_accountState != 2) {
            s_accountState = 0;
            s_anon.clear(); s_nonce.clear(); s_deviceCode.clear();
            s_userCode.clear(); s_link.clear(); s_qrUrl.clear();
            s_error.clear();
        }
        StartLogin();
        body = AccountStatus();
    }
    else if (p == "/account/poll") { PollLogin(); body = AccountStatus(); }
    else if (p == "/account/status") { body = AccountStatus(); }
    else if (p == "/account/logout") {
        ClearSession();
        s_nonce.clear(); s_deviceCode.clear(); s_userCode.clear(); s_qrUrl.clear();
        s_error.clear();
        body = AccountStatus();
    }
    else if (p == "/account/qr.bin") body = QrBytes();
    else if (p == "/profiles.tsv") body = Profiles();
    else if (p.find("/profile/select?id=") == 0) {
        if (SelectProfile(std::atoi(p.c_str() + 19))) body = "OK\n";
    }
    else if (p == "/addons") body = Addons();
    else if (p == "/library/catalog.tsv") body = LibraryCatalog();
    else if (p.find("/videos?id=") == 0) body = Videos(Decode(p.substr(11)));
    else if (p.find("/sources?id=") == 0) {
        const size_t at = p.find("&episode=");
        body = Sources(Decode(p.substr(12, at == std::string::npos ? std::string::npos : at - 12)),
            at == std::string::npos ? -1 : std::atoi(p.c_str() + at + 9));
    }
    else if (p.find("/addons/add?url=") == 0)
        body = ChangeAddon("add", 0, Decode(p.substr(16)));
    else if (p.find("/addons/toggle?id=") == 0)
        body = ChangeAddon("toggle", std::atoi(p.c_str() + 18), "");
    else if (p.find("/addons/remove?id=") == 0)
        body = ChangeAddon("remove", std::atoi(p.c_str() + 18), "");
    else if (p.find("/library/add?profile=") == 0) {
        const size_t sep = p.find("&id=");
        if (sep != std::string::npos)
            body = AddLibrary(std::atoi(p.c_str() + 21), Decode(p.substr(sep + 4)));
    }
    else if (p.find("/avatar/") == 0 && p.find(".rgba") != std::string::npos) {
        const int id = std::atoi(p.c_str() + 8);
        if (id >= 1 && id <= 6) body = Image(s_avatarUrls[id], 128, 128, true);
    }
    else if (p.find("/library.tsv?profile=") == 0) body = Library(std::atoi(p.c_str() + 21));
    else if (p.find("/catalog.tsv") == 0) {
        const size_t at = p.find("skip=");
        body = Catalog(at == std::string::npos ? 0 : std::atoi(p.c_str() + at + 5) / 100, "");
    }
    else if (p.find("/search.tsv?q=") == 0) body = Catalog(0, Decode(p.substr(14)));
    else if (p.find("/poster/") == 0 && p.find(".rgba") != std::string::npos) {
        Art *a = Artwork(p.substr(8, p.size() - 13));
        if (a) body = Image(a->poster, 160, 240, false);
    }
    else if (p.find("/preview/") == 0 && p.size() > 17) {
        const bool bitmap = p.find(".rgba") != std::string::npos;
        body = Preview(p.substr(9, p.find_last_of('.') - 9), bitmap);
    }
    else { out->status = 404; return false; }
    if (!ReturnBody(body, maxBody, out)) { out->status = 502; return false; }
    return true;
}
