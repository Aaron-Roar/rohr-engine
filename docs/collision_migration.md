# Explicit collision-mask migration

Project format **11** and runtime state/snapshot format **9** used mutual collision
filters: both bodies had to accept a pair. Current formats **12** and **10** use
directional `responds_to` masks. Loading an old file still fails until it is
explicitly migrated. Older structural formats are unsupported.

Close applications/editors using the files, then preview **all files that can
interact together**, using their actual JSON paths rather than a workspace manifest:

```sh
rohr-cli migrate-collisions preview game/objects/project.rohr.json saves/world.json
rohr-cli migrate-collisions apply game/objects/project.rohr.json saves/world.json
```

Preview prints a JSON mapping report and writes nothing. Apply prepares and
verifies the mapping again, prints the report, then replaces the supplied files.
It leaves byte-for-byte originals at `<input>.collision_original` and writes the
report to `<first-input>.collision_migration.json`. Existing sidecars or inputs
changed after preparation cause refusal. C files, assets and project manifests
are untouched; relative resource paths continue to resolve in the same location.

Apply stages every output and backup before replacement. If a replacement fails,
it restores earlier inputs. This is failure rollback, not a cross-filesystem
crash-atomic transaction. After interruption or an explicitly reported rollback
failure, restore **every input in the report** from its `.collision_original`
copy before retrying. Keep those originals until the upgraded project is verified.
Do not load a mixture of old and upgraded files.

The tool audits the collision records, not unrelated asset availability or every
other engine schema field. Normal project/runtime loading still validates those
fields. Run project validation, regenerate editor-owned C, build, and exercise
the resulting application after applying:

```sh
rohr-cli --project game validate
rohr-cli --project game generate-c
rohr-cli --project game build
```

## What is preserved

Every supplied `(category, legacy filter)` profile is included, even if its
collision is disabled. Rigid bodies, standalone particles, soft nodes and beam
topology filters are covered. Implicit runtime filters are materialized as the
old default category `1` and filter `UINT64_MAX`. Omitted editor body/node filters
use their old category `1`, filter `1` defaults. Beam collision remains opt-in.
Runtime templates and snapshots share the same conversion, including component
mask updates when a previously implicit filter becomes explicit.

For every ordered pair, including two instances of the same profile, migration
checks that **each** new response direction equals the old mutual acceptance.
Previously excluded pairs therefore stay excluded, and accepted pairs keep
two-sided response. Geometry, motion, materials, ownership and collision-enabled
flags do not change. Geometric overlap queries remain independent of response.

This guarantee covers the supplied profiles, including cross-file pairs. It
cannot infer masks in developer-authored C, files omitted from the migration,
or future edits. Migrate interacting files together with a shared legacy bit
convention. Multiple editor documents must have identical named-bit registries.

## Mapping and limits

The deterministic first pass retains category values and names and removes
unsafe response bits. If this cannot express every old relationship, the tool
groups profiles with identical response neighborhoods and assigns one bit to
each group, preferring available original single-bit categories. Reassigned
names include their bit number and an original category label. The report lists
every profile's old/new masks, the new named-bit registry, and the union of new
membership bits for each old named category.

A named-category union describes membership only; it is not a blanket replacement
for a response mask. Splitting a category requires the original filter context.
Update numeric C constants and removed C API names explicitly using the profile
table. No developer-owned C is rewritten automatically.

If grouping needs more than 64 bits, a third pass encodes pairs of legacy bits:
membership in channel `(category bit, filter bit)` responds to the reversed
channel. Channels with identical membership share a bit. For example, many
profiles using seven legacy bits fit in at most 49 channels.

There are 64 category bits. If none of these exact encodings fits, migration
refuses without changing files. This is the supported conversion's capacity
limit, not a proof that no alternative Boolean encoding exists. The bounded
audit accepts up to 128 files, 64 MiB per
file, 65,536 collision records, and 2,048 distinct profiles. It never approximates
relationships or introduces a legacy runtime mode.
