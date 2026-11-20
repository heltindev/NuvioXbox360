// Test the actual backend against an isolated mock HTTP server/transport.
#define NUVIO_BACKEND_TEST
#include "../src/NuvioNativeBackend.cpp"
#include "../ThirdParty/x360-homebrew-libs/https/HttpRequestHeaders.h"
#include <cassert>
#include <deque>
#include <fstream>

struct Expected {
    std::string endpoint, payload;
    int status;
    Expected(const char *e, int s, const char *p) : endpoint(e), payload(p), status(s) {}
};
static std::deque<Expected> requests;
static void Reply(const char *endpoint, int status, const char *payload) {
    requests.push_back(Expected(endpoint, status, payload));
}
bool Http::Startup(std::string &, std::string &) { return true; }
Http::Response Http::Request(const char *, const std::string &url, const std::string &headers,
                            const std::string &, unsigned long) {
    assert(!requests.empty());
    Expected expected = requests.front(); requests.pop_front();
    assert(url.find(expected.endpoint) != std::string::npos);
    assert(headers.find("Accept: application/json\r\n") != std::string::npos);
    Http::Response r; r.status = expected.status; r.body = expected.payload;
    return r;
}
static std::string ReadCache() {
    std::ifstream f(kSessionPaths[0]);
    return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}
static void Boot() { NuvioNativeShutdown(); assert(NuvioNativeStartup()); }

int main() {
    CreateDirectoryA("obj", NULL); CreateDirectoryA("obj/AuthTests", NULL);
    ClearSession();
    // The original 406 regression: exactly one Accept when caller sets JSON.
    const std::string h = Http::RequestHeaders("POST", "/rest/v1/rpc/example", "api.nuvio.tv",
        "aCcEpT: application/json\r\nContent-Type: application/json", 2);
    assert(h.find("Accept: */*") == std::string::npos);
    assert(h.find("Content-Length: 2\r\n\r\n") != std::string::npos);
    assert(Http::RequestHeaders("GET", "/image.png", "host", "", 0).find("Accept: */*") != std::string::npos);
    Boot(); assert(s_accountState == 0);
    s_refresh = "test-logout-token"; s_access = "test-logout-access"; s_accountState = 2;
    assert(SaveSession());
    NuvioHttpData logout;
    assert(NuvioNativeGet("/account/logout", 4096, &logout));
    delete[] logout.bytes;
    assert(s_accountState == 0 && ReadCache().empty());
    Boot(); assert(s_accountState == 0);
    Reply("start_device_login_session", 200, "[{\"device_code\":\"test-code\",\"user_code\":\"ABCD\",\"verification_uri_complete\":\"https://nuvio.tv/link?code=test\"}]");
    StartLogin(); assert(s_accountState == 1);
    Reply("poll_tv_login_session", 200, "[{\"status\":\"approved\"}]");
    Reply("tv-logins-exchange", 200, "{\"access_token\":\"test-access\",\"refresh_token\":\"test-refresh-1\",\"expires_in\":3600,\"user\":{\"email\":\"test@example.invalid\"}}");
    PollLogin(); assert(s_accountState == 2);
    assert(ReadCache().find("test-refresh-1") != std::string::npos);
    assert(ReadCache().find("test-access") == std::string::npos);
    assert(ReadCache().find("test-code") == std::string::npos);
    // Real reboot: memory disappears; only the on-disk session survives.
    Boot(); assert(s_access.empty() && s_refresh == "test-refresh-1");
    Reply("grant_type=refresh_token", 200, "{\"access_token\":\"test-access-2\",\"refresh_token\":\"test-refresh-2\",\"expires_in\":3600}");
    StartLogin(); assert(s_accountState == 2 && s_access == "test-access-2");
    assert(ReadCache().find("test-refresh-2") != std::string::npos);
    // Transient 406/429/5xx/network errors must never erase the saved login.
    const int transient[] = {406, 429, 503, 0};
    for (int i = 0; i < 4; ++i) {
        Boot(); Reply("grant_type=refresh_token", transient[i], "{}");
        StartLogin(); assert(s_accountState == 2 && s_refresh == "test-refresh-2");
        assert(ReadCache().find("test-refresh-2") != std::string::npos);
        assert(AccountStatus().find("signed_in") == std::string::npos);
    }
    Reply("grant_type=refresh_token", 200, "{\"access_token\":\"test-access-3\",\"refresh_token\":\"test-refresh-3\",\"expires_in\":30}");
    assert(EnsureAuth());
    assert((LONG)(s_tokenExpires - GetTickCount()) > 0);
    // A protected 401 gets one refresh and exactly one retry.
    Reply("sync_pull_profiles", 401, "{}");
    Reply("grant_type=refresh_token", 200, "{\"access_token\":\"test-access-4\",\"refresh_token\":\"test-refresh-4\",\"expires_in\":3600}");
    Reply("sync_pull_profiles", 200, "[]");
    picojson::value response;
    assert(Api("POST", "/rest/v1/rpc/sync_pull_profiles", s_access, "{}", response));
    assert(s_refresh == "test-refresh-4");
    // Invalid remote session: remove it and allow a fresh QR pairing.
    Reply("grant_type=refresh_token", 400, "{\"code\":\"refresh_token_not_found\"}");
    assert(!RefreshToken() && s_accountState == 0 && ReadCache().empty());
    // GoTrue can put the marker in error_code or a legacy message instead.
    const char *invalidBodies[] = {"{\"error_code\":\"session_not_found\"}",
                                  "{\"msg\":\"Invalid Refresh Token: Already Used\"}"};
    for (int i = 0; i < 2; ++i) {
        s_refresh = "test-invalid"; s_accountState = 2; assert(SaveSession());
        Reply("grant_type=refresh_token", 400, invalidBodies[i]);
        assert(!RefreshToken() && s_accountState == 0 && ReadCache().empty());
    }
    // A damaged cache is not interpreted as a login.
    { std::ofstream f(kSessionPaths[0]); f << "broken"; }
    Boot(); assert(s_accountState == 0);
    assert(requests.empty());
    ClearSession();
    std::puts("PASS: pairing, restart without QR, token rotation, transient failures, 401 retry, expiry, corrupt cache, logout, HTTP Accept");
    return 0;
}
