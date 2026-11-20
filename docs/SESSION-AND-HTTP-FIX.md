# Session restoration and HTTP content negotiation

This Xbox implementation follows the public NuvioTV AuthManager,
SupabaseAuthSession, AuthRefreshResponsePolicy and SupabaseModule behavior:

- Pair through the official device-link endpoints and import both tokens.
- Save the renewable session immediately after pairing and every rotation.
- Restore it before starting another QR code; fetch a new access token on boot.
- Keep the saved credential on network errors, HTTP 406/429 and server errors.
- Retry an authenticated HTTP 401 once after refresh.
- Only discard a session for a definitive invalid-refresh response or logout.

The cache is `nuvio_session.dat` beside the XEX (`game:\\`, `D:\\`, then
relative path fallback). This folder must be writable. Writes use a temporary
file and replace the old file only after a successful write/flush/close. The
cache contains the refresh token and email, **not** a password, access token or
QR nonce. It is local plaintext account data: exclude it from uploads, shared
packages and source control. Y in Settings signs out and deletes this cache.
An Android private app directory is not available for an unsigned homebrew XEX;
this port does not claim hardware-backed or encrypted credential storage.

The native HTTP transport used to prepend `Accept: */*` even when API callers
provided `Accept: application/json`. It now preserves a caller's Accept field
and supplies the wildcard only when absent. This removes a content-negotiation
incompatibility that can result in PostgREST HTTP 406. Errors now show the RPC
or endpoint name without printing credentials, bodies or query strings.

## Validation

- `VisualStudio2010/run_auth_tests.bat`: actual backend with mock HTTP and an
  isolated on-disk cache. Pairing, reboot without QR, token rotation, offline,
  HTTP 406/429/503, authenticated 401 retry, short token lifetime, invalid
  session, damaged cache, and request header negotiation.
- `VisualStudio2010/run_core_tests.bat`: navigation, profiles and catalogs.
- Release Xbox 360 XEX compiled with the installed XDK/Visual Studio 2010.

These checks do not approve a real account or prove success against the live
Nuvio service on an RGH console. Scan once, quit completely, and reopen from
the same writable folder to validate that final hardware/network path.

Sources:
https://github.com/NuvioMedia/NuvioTV/blob/main/app/src/main/java/com/nuvio/tv/core/auth/AuthManager.kt
https://github.com/NuvioMedia/NuvioTV/blob/main/app/src/main/java/com/nuvio/tv/core/auth/AuthRefreshResponsePolicy.kt
https://github.com/NuvioMedia/NuvioTV/blob/main/app/src/main/java/com/nuvio/tv/core/di/SupabaseModule.kt
