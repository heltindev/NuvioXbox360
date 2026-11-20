#ifndef NUVIO_INPUT_H
#define NUVIO_INPUT_H
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
