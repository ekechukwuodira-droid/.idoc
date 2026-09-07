# idoc-engine — Stage 1: Container + Model + Serde

A Qt-free C++17 static library implementing the `.idoc` container format
(header, manifest, block directory, TLV records, zstd compression, CRC64
checksum) plus the `Metadata` document-model block end to end, a CLI test
harness, and a doctest suite. This is the foundation the UI (word
processor) layers on top of later — nothing here depends on Qt.

## What's implemented in this stage

- **Container** (`idoc_core`): `FileHeader`, `Manifest` (JSON), binary
  `BlockDirectory`, recursive TLV record encode/decode, per-block zstd
  compression, whole-file trailing CRC64 checksum.
- **Model**: `Metadata`, `Theme`, `Styles`, `Sections`, `Paragraph`/`Run`,
  `NumberingDefinitions`, `Fields`, `Tables` (as before), plus
  `FootnotesEndnotes`/`Comments`/`BookmarksHyperlinks` (§10, three
  physical blocks) and `ResourceIndex` (§11 — metadata only; see
  "Deferred" below), `PreservedUnknown` (§12 — the DOCX-safety
  mechanism), and `LayoutCache` (§13 — derived, never authoritative).
  **All fourteen of the spec's container blocks now exist.**
- **Serde**: all fourteen blocks map to/from their TLV block with the
  same skip-unknown forward-compatibility pattern. `Size2D` encode/decode
  moved into `common_serde` alongside everything else once
  `ResourceEntry.natural_size` needed it.
- **CLI** (`idoc_cli`): `create --with-defaults` now writes a full
  fourteen-block document; `dump` decodes and prints all fourteen.
- **Tests** (`idoc_tests`, doctest via CTest, 117 cases / 350 assertions):
  every block's round-trip and forward-compatibility case, plus container
  tests up to a realistic fourteen-block document.
- **CI**: green on Ubuntu, macOS, and Windows. Fixed a real cross-platform
  bug along the way — `doctest`'s own `CMakeLists.txt` requires a CMake
  compatibility floor below 3.5, which modern CMake (bundled on GitHub's
  macOS/Windows runner images) refuses outright. Fixed via
  `CMAKE_POLICY_VERSION_MINIMUM`, set globally in our own CMakeLists so
  local builds on a recent CMake get the same fix.

## Two things flagged, not silently resolved

1. **FileHeader size.** The spec's struct listing sums to 68 bytes
   (`magic[4] + 2×u16 + u32 + 5×u64 + reserved[16]`) but its prose says
   "fixed 64 bytes." This implementation uses `reserved[12]` (68 − 4) to
   hit 64 bytes exactly. One-line change (`kReservedSize` in
   `include/idoc/container/file_header.hpp`) if the intent was actually 68.
2. **`Color` / `FontRef` wire shape.** The spec uses both throughout
   (Theme, `RunProperties`, etc.) but never defines their fields — unlike
   `BorderSet`/`Shading`/`TabStop[]`, which are at least named in the
   spec's own "Open Questions" as deferred. This implementation uses
   RGBA bytes for `Color` and a family+optional-fallback pair for
   `FontRef`. Reasonable, but worth confirming against whatever the next
   spec draft settles on — see the comment in
   `include/idoc/model/theme.hpp`.
3. **`NumberingRestart`'s shape.** `Section.page_number_restart` is typed
   `NumberingRestart` in the spec, but that type is never defined — and
   the spec's actual `RestartRule` enum (for numbering *levels*) doesn't
   fit page-number semantics (`RestartAfterHigherLevel(level_index)` has
   no equivalent for a page counter). Implemented as the simpler thing
   real word processors expose here: `restart: bool` + optional
   `start_at`. See the comment in `include/idoc/model/sections.hpp`.
4. **`Underline` is explicitly open-ended** (`"None | Single | Double |
   Word-only | Dotted..."`) — unlike a closed enum, an unrecognized future
   value must survive a round-trip rather than fail to parse. Modeled as
   a raw `optional<uint8_t>` with named constants for the values given,
   not a strict `enum class`. See `model/paragraph.hpp`.
5. **`Section.content` / physical "Document Content" block mismatch.**
   §2's document tree types `Section.content` as `Block[]` embedded
   directly in the Section. But §1.1's physical layout separately numbers
   "Block 5: Document Content" as its own top-level block. The spec
   doesn't reconcile these two views. We built Document Content as its
   own block of ID-addressable `Paragraph` records (matching what the
   physical layout table names it, and consistent with §1.6's
   incremental-save story needing paragraph-level granularity).
   `Section.content_raw` stays a reserved opaque blob until the reference
   format connecting the two is actually decided — see the note in
   `include/idoc/serde/paragraph_serde.hpp`.
6. **`Glyph`, `ResourceRef`, `LevelOverride` (§7) are all used but never
   defined.** `Glyph` → `uint32_t` Unicode codepoint (unambiguous from
   context). `ResourceRef` → a single `resource_id` string, same
   reference-by-ID treatment as `ListRef`/`style_id` elsewhere; may need
   revisiting once §11 (Resources) defines what a resource actually
   carries. `LevelOverride` → modeled as exactly the spec's own example
   ("this list starts at 5, not 1": `level` + optional `start_at`), not a
   guessed fuller shape. See `include/idoc/model/numbering.hpp`.
7. **§8's `NumberFormat` collides with §7's.** §8.1's page-number format
   (`Decimal | UpperRoman | ... | LeadingZeros(width)`) is a different
   closed set from §7's `NumberFormat` (which has `Bullet`/`Ordinal`/
   `ChineseCounting`/`CustomGlyph`/`Image` instead) — named
   `PageNumberFormat` here to avoid collision. See
   `include/idoc/model/fields.hpp`.
8. **§8.1's `CustomAnchor` doesn't state units.** Modeled as twips
   (`int32_t x, y`), consistent with every other measurement in the spec.
9. **§8.2 explicitly defers `TableOfContents`/`CrossReference`/`StyleRef`
   payload shapes** to a follow-up doc, pending the layout engine's
   multi-pass resolution algorithm. `Field` only fully models
   `PageNumberFieldPayload`; every other `FieldType` (including ones
   §8.2 doesn't even mention — `TotalPages`, `ChapterNumber`,
   `SectionNumber`, `Date`, `Custom`) gets an opaque `other_payload_raw`
   slot rather than an invented shape.
10. **§9's `TableProperties.borders` has no `?`** (unlike
    `default_shading?`), implying it's required — but since `BorderSet`
    has no defined wire shape anywhere in the spec, it can't actually be
    required yet. Modeled as optional here too, with a note to tighten
    once `BorderSet` exists.
11. **§9's `Column` width** ("twips, can be % or fixed") is modeled as a
    tagged `WidthType::kFixed`/`kPercent` value rather than two separate
    always-present fields; percent is a `float` (e.g. `33.33` = 33.33%)
    since the spec doesn't specify fixed-point precision either way.
12. **§9 reuses "VerticalAlign" for a third, unrelated closed set**
    (`Top | Center | Bottom`, for table cells) — named `CellVerticalAlign`
    here to avoid colliding with §6's `TextVerticalAlign`
    (`Baseline | Superscript | Subscript`).
13. **§10 reuses "RestartRule" for a fourth, incompatible closed set**
    (`Continuous | PerPage | PerSection`, for footnote/endnote numbering)
    — neither of the other two `RestartRule`-shaped things in this spec
    (§7's level-based one with a `level_index` parameter, or our own
    `NumberingRestart` invented for §5's page numbers) has an equivalent
    to `PerPage`/`PerSection`. Named `NoteRestartRule` here. See
    `include/idoc/model/annotations.hpp`.
14. **`ResourceRef`'s `MimeType` and `sha256` fields (§11) have no stated
    wire shape.** `MimeType` → plain string (MIME types are an open,
    growing set; no enumeration would make sense). `sha256` → lowercase
    hex string rather than raw 32 bytes, for Manifest-adjacent
    human-inspectability. See `include/idoc/model/resources.hpp`.
15. **§12's `Checksum` type is used but never defined.** Reused CRC64
    (`idoc::crc64`) since it's the only checksum algorithm the spec
    defines anywhere (§1.1's whole-file trailing checksum) — introducing
    a second hash algorithm for one field would be an unforced
    complication. See `include/idoc/model/preserved_unknown.hpp`.
16. **§13's `PageGeometry` is described only in prose**, not a formal
    field list. "Content block references" hits the same undecided
    `Section.content` reference-format question (point 5); "field cache
    pointers" is read as a plain list of field IDs. See
    `include/idoc/model/layout_cache.hpp`.

## Deliberately deferred (per current scope)

- `StyleDefinition.paragraph_props`/`run_props` (§4) — still reserved raw
  byte slots even though `ParagraphProperties`/`RunProperties` now exist
  (§6). Populating them is now possible but wasn't done this stage; it's
  a serde-only change (encode/decode the real struct into the slot) with
  no changes to `Styles`' shape.
- `Section.content`, `HeaderFooterContent.content`, `Cell.content`, and
  `PageGeometry.content_block_refs` (all `Block[]` or references into
  it) — reserved raw byte slots until the `{content_type, content_id}`
  reference format is decided. Now that Paragraph, Table, *and* the
  layout cache all need this same decision, it's the most natural next
  thing to resolve.
- `ParagraphProperties.borders`/`shading`/`tab_stops`, and `TableProperties`/
  `Cell`'s equivalents — reserved raw byte slots; `BorderSet`/`Shading`/
  `TabStop[]` have no defined wire shape anywhere in the spec (the spec's
  own "Open Questions" already flags these as deferred).
- `Field.other_payload_raw` for every `FieldType` except `kPageNumber` —
  see point 9 above.
- **Resource blob storage** (§11) — `ResourceIndex`/`ResourceEntry`
  metadata is fully modeled and round-trips, but `ContainerWriter` still
  always emits an empty resource area (unchanged since stage 1). Actually
  writing/reading raw resource bytes at `blob_offset`/`blob_length` is
  separate, additional engineering, not part of modeling the index.
- **The link between `PreservedUnknown`'s DOCX-round-trip mechanism and
  this codebase's own "skip unknown TLV field" forward-compatibility
  pattern** — these are deliberately NOT wired together; see the note in
  `include/idoc/model/preserved_unknown.hpp` for why they're different
  concerns that happen to share a "don't lose unrecognized data" spirit.
- `RevisionLog` (tracked changes) block
- Encryption (`header_flags` bit 0)
- Incremental save (append + dead-space tracking) — `build()` always
  produces a dense file
- Salvage/degraded-open mode (`§15`) — `ContainerReader::open()` throws on
  any structural problem rather than attempting partial recovery
- Style resolver, numbering *resolution*, fields *computation* (walking
  the models to actually compute a page number or TOC entry), the actual
  layout/pagination engine that would populate `LayoutCache` — real
  engineering work built on top of what exists now

## One spec discrepancy, flagged not silently resolved

The spec's `FileHeader` struct listing sums to 68 bytes
(`magic[4] + 2×u16 + u32 + 5×u64 + reserved[16]`) but its prose says
"fixed 64 bytes." This implementation uses `reserved[12]` (68 − 4) to hit
64 bytes exactly. If the spec's intent was actually 68, this is a one-line
change (`kReservedSize` in `include/idoc/container/file_header.hpp`) —
nothing else in the codebase depends on the reserved region's size.

## Building

Requires CMake 3.20+ and a C++17 compiler. Dependencies (zstd,
nlohmann_json, doctest) are fetched automatically via `FetchContent` —
no manual installation needed, just network access on first configure.

### Linux / macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Windows (PowerShell)

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Using the CLI (the "no UI" test harness)

```bash
# Create a minimal .idoc file with a Metadata block
./build/idoc_cli create --output test.idoc --title "The War of Ash and Iron" --author "Mystic" --language en-US

# Add --with-defaults for a sample Theme + Styles + Sections + Numbering + Content + Fields + Tables + Annotations + Resources + Layout Cache set too (all 14 blocks)
./build/idoc_cli create --output test.idoc --title "The War of Ash and Iron" --author "Mystic" --with-defaults

# Inspect it — prints the manifest JSON and decoded metadata/theme/styles
./build/idoc_cli dump test.idoc

# Verify structural integrity (checksum + parse) without printing content
./build/idoc_cli verify test.idoc
```

PowerShell equivalents:

```powershell
.\build\idoc_cli.exe create --output test.idoc --title "The War of Ash and Iron" --author "Mystic" --with-defaults
.\build\idoc_cli.exe dump test.idoc
.\build\idoc_cli.exe verify test.idoc
```

## CI

`.github/workflows/ci.yml` builds and runs the full test suite on Ubuntu,
Windows, and macOS on every push/PR. Push this repo to GitHub and it runs
automatically — no configuration needed.

## Project layout

```
include/idoc/           public headers
  container/             FileHeader, Manifest, BlockDirectory, TLV, compression, block type IDs
  model/                 all 14 blocks' pure data types (Metadata through LayoutCache)
  serde/                 per-block TLV mapping, plus common_serde for shared primitives
    common_serde.hpp       Color, FontRef, Indent, Spacing, Size2D, RunProperties, RestartRule
src/                     implementation, mirrors include/idoc/ layout
cli/main.cpp             idoc_cli: create / dump / verify
tests/                   doctest suite, one file per concern
.github/workflows/ci.yml
```

## Next stage

**All 14 of the spec's container blocks now exist and round-trip.** What
remains is qualitatively different work:

1. **The `Section.content`/`Cell.content`/`PageGeometry.content_block_refs`
   reference format** — the single most-referenced open question across
   this codebase (5+ deferred slots all point back to it). Deciding this
   unlocks populating several reserved fields at once.
2. **Resource blob storage** — an actual read/write API on
   `ContainerWriter`/`ContainerReader` for the resource area, which has
   been reserved-but-empty since stage 1.
3. **Migration framework** (§14) and **salvage mode** (§15) — the two
   infrastructure pieces the spec describes but that aren't new blocks.
4. Separately, and much larger: the **style resolver**, **numbering
   resolution**, **fields computation**, and the **layout/pagination
   engine itself** — the real word-processor rendering work this entire
   container format exists to support.

Say which and I'll build it the same way: full files, tested, zipped,
and pushed with notes.
