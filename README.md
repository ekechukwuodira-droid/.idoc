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
  `NumberingDefinitions` (as before), plus `Fields` (§8 — full
  `PageNumberFieldPayload`, opaque-by-design for every other field type)
  and `Tables` (§9 — `Table`/`Row`/`Cell` with merge/span support).
  `Run.field_ref` upgraded from a reserved raw slot to a real `FieldRef`
  now that §8 exists.
- **Serde**: all nine blocks map to/from their TLV block with the same
  skip-unknown forward-compatibility pattern. `RestartRule` encode/decode
  moved into `common_serde` alongside `RunProperties`, since §8's
  `PageNumberFieldPayload.restart_rule` explicitly shares §7's enum.
- **CLI** (`idoc_cli`): `create --with-defaults` now writes a full
  eight-block document; `dump` decodes and prints all eight.
- **Tests** (`idoc_tests`, doctest via CTest, 82 cases / 257 assertions):
  every block's round-trip and forward-compatibility case, plus container
  tests up to a realistic eight-block document.

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

## Deliberately deferred (per current scope)

- `StyleDefinition.paragraph_props`/`run_props` (§4) — still reserved raw
  byte slots even though `ParagraphProperties`/`RunProperties` now exist
  (§6). Populating them is now possible but wasn't done this stage; it's
  a serde-only change (encode/decode the real struct into the slot) with
  no changes to `Styles`' shape.
- `Section.content`, `HeaderFooterContent.content`, and `Cell.content`
  (all `Block[]` — Paragraph | Table) — reserved raw byte slots until the
  reference format described in point 5 above is decided. Now that both
  Paragraph *and* Table exist, this is the natural next thing to resolve.
- `ParagraphProperties.borders`/`shading`/`tab_stops`, and `TableProperties`/
  `Cell`'s equivalents — reserved raw byte slots; `BorderSet`/`Shading`/
  `TabStop[]` have no defined wire shape anywhere in the spec (the spec's
  own "Open Questions" already flags these as deferred).
- `Field.other_payload_raw` for every `FieldType` except `kPageNumber` —
  see points 9 above.
- `RevisionLog` (tracked changes) block
- Encryption (`header_flags` bit 0)
- Incremental save (append + dead-space tracking) — `build()` always
  produces a dense file
- Resource blob area (`§11`) — the writer always emits an empty resource
  area; `kResourceIndex`/blob storage lands with the Resources block,
  which will also finally give `ResourceRef` (§7) a real home
- Salvage/degraded-open mode (`§15`) — `ContainerReader::open()` throws on
  any structural problem rather than attempting partial recovery
- Style resolver, numbering *resolution*, fields *computation* (walking
  the models to actually compute a page number or TOC entry), layout
  cache — later stages built on top of what exists now

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

# Add --with-defaults for a sample Theme + Styles + Sections + Numbering + Content + Fields + Tables set too
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
  model/                 Metadata, Theme, Styles, Sections, Paragraph, Numbering, Fields, Tables
  serde/                 per-block TLV mapping, plus common_serde for shared primitives
    common_serde.hpp       Color, FontRef, Indent, Spacing, RunProperties, RestartRule
src/                     implementation, mirrors include/idoc/ layout
cli/main.cpp             idoc_cli: create / dump / verify
tests/                   doctest suite, one file per concern
.github/workflows/ci.yml
```

## Next stage

All nine of the spec's fully-specifiable blocks now exist: Metadata,
Theme, Styles, Sections, Numbering, Document Content, Fields, Tables —
plus the container itself. What's left, in roughly the order it makes
sense to tackle:

1. **Footnotes, Endnotes, Comments, Bookmarks, Hyperlinks** (§10) — the
   last block with a genuinely new schema to build from spec text.
2. **Resources** (§11) — gives `ResourceRef` (§7) and every deferred
   image/media reference a real home.
3. **The `Section.content`/`Cell.content` reference format** — now that
   Paragraph *and* Table both exist, this is finally decidable: an
   ordered list of `{content_type, content_id}` pairs pointing into
   Document Content and Tables.
4. **Preserved-Unknown Store** (§12) and **Layout Cache** (§13) — round
   out the container format's block list.

After that, the migration framework (§14) and salvage mode (§15) are
infrastructure rather than new blocks. Separately — real engineering
work, not more container/serde slices — sit the style resolver,
numbering resolution, fields computation, and the actual layout/
pagination engine.

Say which and I'll build it the same way: full files, tested, zipped.
