# Search, account and login — UI8

UI8 source changes are included in the latest linked executable. A newer local
XEX was generated on November 23, 2026. Current video playback still has a
reported failure; see [known issues](KNOWN_ISSUES.md).

- Search accepts A, X, Y, Shift and Start while catalog/artwork loads. The field
  displays edited text and scrolls with the cursor. Start/OK submits the query;
  B from results returns to editing the submitted term.
- B from the keyboard returns Home. Deferred buttons from another screen are discarded.
- Settings → You → Account displays status, email and Sign out. Y in Settings
  also opens Account. In-app labels may appear as Voce and Conta.
- Sign out drains the active request, clears the saved session and account
  profile/cache state, and opens a fresh QR automatically. It does not delete
  the cloud account or its library.
- Login uses a purple split layout and QR pairing. The QR has a white background,
  a four-module quiet zone and whole-pixel modules.
- Category icons: person, palette, play, chain links and gear.

Reference: NuvioMedia/NuvioDesktop, Dev branch, AccountSettingsPage.kt and
DeviceLinkAuthSection.kt. Icon drawings are independently authored.

Windows navigation/keyboard tests, mock-transport authentication and XDK syntax
checks passed. Physical Xbox layout, input and camera scanning remain unverified.

Build with VisualStudio2010/BUILD-RELEASE.cmd. Check Xbox UI8 in Settings.
