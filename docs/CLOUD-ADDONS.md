# Cloud add-ons and TV interface

## Protocol references

Protocol review used NuvioMedia/NuvioTV revision
`ff7fbd747955a1a6108ac23c296b79ad851a14b1`:

- AddonSyncService.kt: get_sync_owner, profile filtering, inheritance, sync_push_addons.
- SupabaseModels.kt: profile and add-on fields.
- AddonRepositoryImpl.kt: URL normalization preserving configuration and query.
- [Public manifest](https://catalog.nuvio.tv/manifest.json): catalogs, extras, resources and types.

[Upstream tree](https://github.com/NuvioMedia/NuvioTV/tree/ff7fbd747955a1a6108ac23c296b79ad851a14b1).
This implementation runs native C++, not Kotlin/Android.

## Data flow

After profile authorization/selection, /profile/select sets the real profile.
The client requests add-ons for its synchronization owner. Home loads enabled
manifests and catalog endpoints. Search uses catalogs advertising the search extra;
catalogs requiring other missing filters are not requested. The default catalog
is a fallback only when applicable and not explicitly disabled.

Internal TSV uses #catalog and #row headers. A valid empty response clears the
list; a failed response preserves prior data. IDs such as tmdb:42 are accepted
without assuming an IMDb prefix or mandatory year. Entire service databases are
not downloaded. Requests and caches have explicit limits.

Metadata, episodes and sources use the configured add-on URL. Nuvio API
credentials are not sent to add-on hosts, source URLs are not shown in the UI,
and library writes preserve the source add-on.

Before add-on mutation, the backend re-reads remote state and preserves other
records, including legacy URLs and disabled entries. Success requires a successful
write response. The official API replaces the full list, so simultaneous edits
from another device can still be last-writer-wins. Lists over 64 entries are
protected from truncating writes; inherited profile add-ons are read-only here.

Catalog and artwork/add-on jobs run outside the drawing thread and are serialized
because the backend has shared session state. Remaining account/profile calls
can still block temporarily. Up to eight catalogs of twenty titles are requested
per group; LB/RB changes the group. UI8 keyboard editing stays local during loading.

## Validation and open work

Mock tests check protocol/cache/search behavior without modifying a real account.
They do not prove live service availability or provider playback. Validate saved
session restoration, profile ownership, Home, search, library, add-on edits and
sign-out on the target device. PIN verification is still missing.

Media playback remains unresolved; see [known issues](KNOWN_ISSUES.md).
