# Roadmap

## Highest priority: unresolved playback

- [ ] Reproduce and fix [video-loading failure #4](https://github.com/heltindev/NuvioXbox360/issues/4).
- [ ] Identify the failing stage: peers, metadata, pieces, container, decoder or output.
- [ ] Validate the current decoder with a local H.264/AAC <=720p sample.
- [ ] Validate HTTP and torrent playback through the full app on physical Xbox 360.
- [ ] Test public trackers/DHT, pause, seek, cancellation, A/V sync and long playback.
- [ ] Measure total memory and sustained performance on hardware.

## Implemented, still requiring real-device regression checks

- [x] Official QR pairing, saved-session restoration and account/profile synchronization.
- [x] Cloud add-ons, catalog discovery, search and library routes.
- [x] Four-item sidebar and five Settings categories with implemented actions enabled.
- [x] UI8 search input during loading, live query display and fresh QR after sign-out.
- [x] Native FFmpeg/XAudio2 integration and bounded torrent v1 client.
- [x] Isolated Windows tests and an earlier local-file Xenia decoder smoke test.
- [ ] Confirm UI8 keyboard, category icons, account switching and QR scanning on Xbox.
- [ ] Confirm catalog/library invalidation after changing profiles/add-ons.

## Missing features and limitations

- [ ] PIN authentication for protected profiles.
- [ ] Subtitle handling and provider compatibility testing.
- [ ] Persistent catalog/image cache across restarts.
- [ ] Replace remaining synchronous account/profile network calls with cancellable jobs.
- [ ] Complete currently dimmed appearance, layout, integration, group-session and system pages.
- [ ] Validate optional external conversion with real AV1, actual TorrServer and long movies.

Native HEVC/AV1/DRM, uTP, BitTorrent v2 and IPv6 are not implemented. No delivery
date or performance gain is promised. A checked implementation item is not proof
that the complete media application works on hardware.
