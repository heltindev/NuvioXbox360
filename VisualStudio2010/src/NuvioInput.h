#ifndef NUVIO_INPUT_H
#define NUVIO_INPUT_H
#include "NuvioApp.h"

// XInput button bits, shared with the portable regression tests. Search only
// edits local text and queues a request; artwork must never block its keyboard.
inline unsigned short NuvioInputDuringLoadMask(const NuvioState &state,bool catalogLoading) {
    if(state.screen==NUVIO_SCREEN_SEARCH)return 0xFFFF;
    unsigned short mask=0x000F|0x0020|0x2000; // directions, Back, B
    const bool browse=state.screen==NUVIO_SCREEN_HOME||state.screen==NUVIO_SCREEN_DISCOVER||
        state.screen==NUVIO_SCREEN_LIBRARY||state.screen==NUVIO_SCREEN_SEARCH_RESULTS;
    if(browse)mask|=0x8000; // Y opens search locally, even while catalog loads.
    if(!catalogLoading&&browse&&!state.sidebarFocused)mask|=0x1000;
    if(state.screen==NUVIO_SCREEN_ACCOUNT)mask|=0x1000; // queues sign-out until idle
    if(state.screen==NUVIO_SCREEN_SETTINGS)mask|=0x8000; // open account
    return mask;
}
// Portable, per-controller input policy. Diagonals choose the dominant axis.
struct NuvioInputRepeat {
    unsigned short held;
    unsigned long deadline;
    NuvioInputRepeat() : held(0), deadline(0) {}
};
inline unsigned short NuvioStickDirections(int x, int y) {
    const int deadzone=9000;
    const int ax=x<0?-x:x, ay=y<0?-y:y;
    if (ax<deadzone && ay<deadzone) return 0;
    if (ax>ay) return x<0 ? 4 : 8;
    return y>0 ? 1 : 2;
}
inline unsigned short NuvioRepeatDirections(NuvioInputRepeat &state,
    unsigned short directions, unsigned long now) {
    if (!directions) { state.held=0; return 0; }
    if (directions!=state.held) {
        state.held=directions; state.deadline=now+330; return directions;
    }
    if ((long)(now-state.deadline)>=0) {state.deadline=now+110;return directions;}
    return 0;
}
#endif
