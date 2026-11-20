// Actual production backend, isolated HTTP transport; no real account credentials.
#define NUVIO_BACKEND_TEST
#include "../src/NuvioNativeBackend.cpp"
#include <cassert>
#include <deque>

struct ExpectedCloud {
    std::string endpoint, body;
    int status;
    ExpectedCloud(const char *e, int s, const char *b) : endpoint(e), body(b), status(s) {}
};
static std::deque<ExpectedCloud> pending;
static std::string lastPayload;
static void Reply(const char *e, int s, const char *b) {
    pending.push_back(ExpectedCloud(e,s,b));
}
bool Http::Startup(std::string &, std::string &) { return true; }
Http::Response Http::Request(const char *, const std::string &url,
    const std::string &headers, const std::string &body, unsigned long) {
    assert(!pending.empty());
    const ExpectedCloud e = pending.front(); pending.pop_front();
    if (url.find(e.endpoint) == std::string::npos) {
        std::fprintf(stderr, "Unexpected endpoint; expected %s\n", e.endpoint.c_str());
        assert(false);
    }
    assert(headers.find("Accept: application/json") != std::string::npos);
    // Credentials must never be forwarded to third-party addon hosts.
    if (url.find("https://api.nuvio.tv") != 0) {
        assert(headers.find("Authorization") == std::string::npos);
        assert(headers.find("apikey") == std::string::npos);
    }
    lastPayload = body;
    Http::Response r; r.status=e.status; r.body=e.body; return r;
}
static const char *manifest =
    "{\"id\":\"test.catalog\",\"name\":\"Test Catalog\",\"resources\":[{\"name\":\"stream\",\"types\":[\"movie\"]}],\"catalogs\":["
    "{\"type\":\"movie\",\"id\":\"trending\",\"name\":\"Real Rail\"},"
    "{\"type\":\"movie\",\"id\":\"search\",\"name\":\"Search\",\"extra\":[{\"name\":\"search\",\"isRequired\":true}]}]}";
static const char *rows =
    "[{\"url\":\"https://addon.test/Key%7CCase/manifest.json?config=a%2Fb\",\"name\":\"Mine\",\"enabled\":true},"
    "{\"url\":\"http://legacy.test\",\"enabled\":false}]";
static void Rows() { Reply("/rest/v1/addons?",200,rows); }
static void SignedIn() {
    ClearProfileData();
    s_started=true; s_accountState=2; s_access="mock-access"; s_refresh="mock-refresh";
    TokenDeadline(3600);
    s_profileAllowed[1]=s_profileAllowed[2]=true;
    s_primaryAddons[2]=true;
    assert(SelectProfile(1));
    Reply("get_sync_owner",200,"\"00000000-0000-0000-0000-000000000001\"");
}
int main() {
    std::FILE *existingConfig=std::fopen("nuvio.cfg","rb");
    if(existingConfig){std::fclose(existingConfig);std::fputs("Run isolated cloud tests without a local nuvio.cfg in VisualStudio2010. Existing configuration was preserved.\n",stderr);return 1;}

    assert(Canonical("https://addon.test/AbC%7C/manifest.json?token=A%2FB") ==
        "https://addon.test/AbC%7C?token=A%2FB");
    assert(AddonUrl("https://addon.test/AbC?token=A%2FB", "/meta/movie/tmdb%3A42.json") ==
        "https://addon.test/AbC/meta/movie/tmdb%3A42.json?token=A%2FB");
    assert(Canonical("https://addon.test/\r\nInjected: x").empty());
    assert(Canonical("https://user:password@addon.test").empty());
    assert(Decode(Encode("tmdb:42&x=A%2FB")) == "tmdb:42&x=A%2FB");
    SignedIn();
    Rows(); Reply("/manifest.json?config=a%2Fb",200,manifest);
    const std::string addons=Addons();
    assert(addons.find("#cloud\n")==0 && addons.find("Mine")!=std::string::npos);
    assert(s_addons.size()==2 && !s_addons[1].enabled);
    Reply("/catalog/movie/trending.json?config=a%2Fb",200,
        "{\"metas\":[{\"id\":\"tmdb:42\",\"name\":\"Upcoming\",\"poster\":\"https://images.test/p.jpg\"}]}");
    std::string catalog=Catalog(0,"");
    assert(catalog.find("#row\tReal Rail")!=std::string::npos);
    assert(catalog.find("tmdb:42\tmovie\t0\tUpcoming")!=std::string::npos);
    assert(s_art.size()==1 && s_art[0].base=="https://addon.test/Key%7CCase?config=a%2Fb");
    Reply("/stream/movie/tmdb%3A42.json?config=a%2Fb",200,
        "{\"streams\":[{\"name\":\"Provider\",\"title\":\"720p\",\"url\":\"https://video.test/private-key.mp4\"}]}");
    const std::string sources=Sources("tmdb:42",-1);
    assert(sources.find("Provider")!=std::string::npos && sources.find("private-key")==std::string::npos);
    NuvioPlaybackSource playable;
    assert(NuvioNativePlaybackSource(0,&playable));
    assert(playable.url=="https://video.test/private-key.mp4" && playable.headers.empty());
    // Honor provider headers without allowing a second HTTP request to be injected.
    picojson::value hdr;
    assert(Parse("{\"url\":\"https://video.test/a.mp4\",\"behaviorHints\":{\"proxyHeaders\":{\"request\":"
        "{\"Referer\":\"https://provider.test/\",\"Host\":\"wrong.test\",\"Range\":\"bytes=9-\","
        "\"X-Unsafe\":\"value\\r\\nInjected: true\"}}}}",hdr));
    s_sources.clear();s_sources.push_back(hdr);
    assert(NuvioNativePlaybackSource(0,&playable));
    assert(playable.headers=="Referer: https://provider.test/\r\n");
    // Empty, out-of-range and external-player-only entries never reach the decoder.
    assert(!NuvioNativePlaybackSource(99,&playable));
    assert(Parse("{\"clientResolve\":{\"infoHash\":\"0123456789012345678901234567890123456789\",\"fileIdx\":2,\"sources\":[\"tracker:udp://tracker.test:80/announce\"]}}",hdr));
    s_sources.clear();s_sources.push_back(hdr);
    assert(NuvioNativePlaybackSource(0,&playable));
    assert(playable.url.find("magnet:?xt=urn:btih:")==0&&playable.url.find("&so=2")!=std::string::npos&&
           playable.url.find("&tr=udp%3A%2F%2Ftracker.test%3A80%2Fannounce")!=std::string::npos);
    assert(Parse("{\"url\":\"https://video.test/huge.mp4\",\"name\":\"4K HEVC HDR\"}",hdr));
    s_sources.clear();s_sources.push_back(hdr);assert(!NuvioNativePlaybackSource(0,&playable));

    // The conversion job is prepared on the player worker, without HTTP on the UI thread.
    std::FILE *cfg=std::fopen("nuvio.cfg","wb");assert(cfg);
    std::fputs("transcode_server=http://converter.test:8787\ntranscode_token=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\ntranscode_mode=auto\n",cfg);std::fclose(cfg);
    assert(NuvioNativePlaybackSource(0,&playable));
    assert(playable.url=="http://converter.test:8787/v1/jobs");
    picojson::value prepared;assert(Parse(playable.prepareBody,prepared));
    assert(AsString(Field(prepared,"url"))=="https://video.test/huge.mp4");
    assert(playable.headers.find("Authorization: Bearer AAAAA") == 0);
    std::remove("nuvio.cfg");
    assert(Parse("{\"externalUrl\":\"https://provider.test/web-player\"}",hdr));
    s_sources.clear();s_sources.push_back(hdr);
    assert(!NuvioNativePlaybackSource(0,&playable));
    Reply("/catalog/movie/search/search=hello%20world.json?config=a%2Fb",200,
        "{\"metas\":[{\"id\":\"tt42\",\"name\":\"Found\",\"releaseInfo\":\"2025\"}]}");
    assert(Catalog(0,"hello world").find("Found")!=std::string::npos);
    // Failed cloud writes never turn the displayed addon off.
    Rows(); Reply("sync_push_addons",503,"{}");
    assert(ChangeAddon("toggle",1,"").empty() && s_addons[0].enabled);
    Rows(); Reply("sync_push_addons",200,"{}");
    assert(ChangeAddon("toggle",1,"")=="OK\n" && !s_addons[0].enabled);
    picojson::value payload;
    assert(Parse(lastPayload,payload));
    const picojson::array &pushed=Field(payload,"p_addons").get<picojson::array>();
    assert(pushed.size()==2);
    assert(AsString(Field(pushed[0],"url"))=="https://addon.test/Key%7CCase/manifest.json?config=a%2Fb");
    assert(AsString(Field(pushed[1],"url"))=="http://legacy.test");
    // Inheritance cannot accidentally mutate primary addons from a child profile.
    assert(SelectProfile(2) && s_addonProfile==1);
    Rows(); Reply("/manifest.json?config=a%2Fb",200,manifest);
    assert(!Addons().empty());
    Rows();
    assert(ChangeAddon("remove",1,"").empty() && s_addons.size()==2);
    assert(!SelectProfile(3)); // locked/unknown
    assert(SelectProfile(1));
    Reply("/rest/v1/addons?",200,"[]");
    assert(Addons()=="#cloud\n" && s_addons.empty());
    // Empty accounts use the official catalog, without a cloud write.
    Reply("https://catalog.nuvio.tv/manifest.json",200,manifest);
    Reply("/catalog/movie/trending.json",200,"{\"metas\":[]}");
    assert(Catalog(0,"")=="#catalog\n");
    // Library uses its original addon and IDs, with a valid empty response.
    Reply("sync_pull_library",200,
        "[{\"content_id\":\"tmdb:42\",\"content_type\":\"movie\",\"name\":\"Saved\","
        "\"addon_base_url\":\"https://addon.test\",\"poster\":\"https://images.test/p.jpg\"}]");
    assert(LibraryCatalog().find("tmdb:42\tmovie\t0\tSaved")!=std::string::npos);
    Reply("sync_pull_library",200,"[]");
    assert(LibraryCatalog()=="#catalog\n" && s_art.empty());
    // Native routes decode configured URLs and preserve authenticated session.
    Reply("/rest/v1/addons?",200,"[]");
    Reply("https://new.test/manifest.json",200,manifest);
    Reply("sync_push_addons",200,"{}");
    NuvioHttpData result;
    const std::string route="/addons/add?url="+Encode("https://new.test/manifest.json");
    assert(NuvioNativeGet(route.c_str(),4096,&result));
    delete[] result.bytes;
    assert(s_addons.size()==1);
    Reply("/rest/v1/addons?",200,"[{\"url\":\"https://new.test\",\"enabled\":true}]");
    Reply("sync_push_addons",200,"{}");
    assert(NuvioNativeGet("/addons/remove?id=1",4096,&result));
    delete[] result.bytes;
    assert(s_addons.empty());
    assert(pending.empty());
    ClearProfileData();
    assert(s_addons.empty() && s_art.empty() && !s_profileAllowed[1]);
    std::puts("PASS: cloud owner, profiles, inheritance, configured URLs, TMDB, rails, search, write rollback, library, default catalog, credentials isolation");
    return 0;
}
