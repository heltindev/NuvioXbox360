// Native Xbox test: decode/render a supplied game:\sample.mp4 without account login.
#include <xtl.h>
#include <cstdio>
#include <string>
#include <vector>
#include "FFPlayer.h"
#include "Log.h"
#include "Http.h"
int main() {
    Log::Open("game:\\player-smoke.log");
    IDirect3D9 *d3d=Direct3DCreate9(D3D_SDK_VERSION);
    if(!d3d)return 1;
    D3DPRESENT_PARAMETERS pp;ZeroMemory(&pp,sizeof(pp));
    pp.BackBufferWidth=1280;pp.BackBufferHeight=720;pp.BackBufferCount=1;
    pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    pp.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
    IDirect3DDevice9 *device=NULL;
    if(FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,NULL,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&device)))return 2;
    FFPlayer *player=new FFPlayer;
    if(!player->Startup(device))return 3;
    std::string source="game:\\sample.mp4";
    std::FILE *input=std::fopen("game:\\smoke-source.txt","rb");
    if(input) {
        char url[1024]={0};std::fgets(url,sizeof(url),input);std::fclose(input);
        source=url;while(!source.empty()&&(source[source.size()-1]=='\r'||source[source.size()-1]=='\n'))source.erase(source.size()-1);
    }
    if(source.find("http")==0||source.find("magnet:")==0) {std::string error,ip;if(!Http::Startup(error,ip))return 5;}
    std::vector<std::string> urls;urls.push_back(source);
    std::vector<std::wstring> labels;labels.push_back(L"Decoder smoke");
    player->Open(urls,labels,"");
    const DWORD started=GetTickCount();
    FFPlayer::Stats stats;
    for(int frame=0;frame<3000;++frame) {
        player->Update();
        device->Clear(0,NULL,D3DCLEAR_TARGET,0xFF000000,1,0);
        device->BeginScene();device->SetRenderState(D3DRS_VIEWPORTENABLE,TRUE);
        D3DRECT screen={0,0,1280,720};player->RenderFrame(screen);
        device->EndScene();device->Present(NULL,NULL,NULL,NULL);
        player->GetStats(stats);
        if(frame%30==0||player->Failed()) {
            std::FILE *out=std::fopen("game:\\decoder-smoke.txt","wb");
            if(out) {
                MEMORYSTATUS mem;GlobalMemoryStatus(&mem);
                std::fprintf(out,"decoded=%ld\nshown=%ld\nposition=%.3f\naudio_ms=%d\nwidth=%d\nheight=%d\nfree_bytes=%lu\nerror=%s\n",
                    stats.decoded,stats.shown,player->Position(),stats.audioBufferedMs,stats.width,stats.height,mem.dwAvailPhys,player->Error().c_str());
                std::fclose(out);
            }
        }
        if((stats.shown>=40&&player->Position()>=2)||player->Failed()||GetTickCount()-started>45000)break;
        Sleep(1);
    }
    player->GetStats(stats);
    std::FILE *out=std::fopen("game:\\decoder-smoke.txt","wb");
    if(out) {
        MEMORYSTATUS mem;GlobalMemoryStatus(&mem);
        std::fprintf(out,"decoded=%ld\nshown=%ld\ndropped=%ld\nposition=%.3f\naudio_ms=%d\nwidth=%d\nheight=%d\nfree_bytes=%lu\nerror=%s\n",
            stats.decoded,stats.shown,stats.dropped,player->Position(),stats.audioBufferedMs,stats.width,stats.height,mem.dwAvailPhys,player->Error().c_str());
        std::fclose(out);
    }
    delete player;device->Release();d3d->Release();
    return stats.shown>=40?0:4;
}
