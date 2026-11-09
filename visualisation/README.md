# Standard geometry figures

One PDF per subsystem, plus one for the full detector (`ship_geometry.pdf`),
rendered from the geometry with fullSimLight. Each PDF has one page per view:
isometric, side, top and front, an exploded view, and an interior view where the
outside hides what matters.

```bash
pixi run visualisation                       # everything
pixi run python visualisation/make_visualisations.py Target Magnet   # after a first full run
```

This uses only the default environment. Rendering needs no display and no working
OpenGL. The full set takes several
minutes, most of it in the Calorimeter and the full detector.

## Layout

| Path | Contents |
| --- | --- |
| `make_visualisations.py` | Builds the databases, writes macros, renders, makes the PDFs |
| `figure.typ` | Lays out the PDF pages with [Typst](https://typst.app) |
| `views.toml` | Colours, views, volumes to hide and how each subsystem explodes |
| `pdf/` | The figures (generated, not tracked) |
| `macros/` | The Geant4 macros, one per scene (generated, not tracked) |

Databases, raw images and the logs of each run go to `build/figures/`. The
PDFs are not committed because the repository's pre-commit hooks reject files
over 500 KB.

## How a figure is made

1. `build_geometry` builds the scene's database, so figures match the source.
2. The logical volumes and their materials are read from the database.
3. A macro hides containers and fill gases and colours everything else, using
   `/vis/geometry/set/visibility` and `/vis/geometry/set/colour`. Do not add
   `/vis/viewer/set/culling global false` to these macros: it draws the hidden
   volumes as well.
4. fullSimLight renders the macro with the `TOOLSSG_OFFSCREEN` driver.
5. The script finds what each image draws and writes a JSON description of each
   PDF. `figure.typ` then crops each image to that and places it on an A4
   landscape page with a title, a legend and axes, and a scale bar on the
   axis-aligned views.

Typst uses its built-in font, so the figures look the same on any machine, and
repeating a run with the same stamp gives identical PDFs. Images are embedded at
full size because fine structure such as the trackers' straws shows moire at half
of it. The drawn area comes from how fullSimLight's PNGs are encoded, so that
reading them needs no image library. If that encoding changes, `drawn_area` fails
with an error.

fullSimLight only creates its vis manager with `-i`, which normally opens a Qt
session. Setting `G4UI_USE_TCSH=1` swaps that for a terminal session reading
stdin, so `exit` on stdin ends the run. After `pixi run visualisation`, from the
repository root, this writes the images to the current directory:

```bash
echo exit | G4UI_USE_TCSH=1 fullSimLight -g build/figures/Target.db -m visualisation/macros/Target.mac -i
```

## Exploded views

Geant4's `/vis/viewer/set/explodeFactor` has no effect with this driver. Instead,
the script copies the database and moves placements in the copy, following the
`explode` rules in `views.toml`, and fullSimLight draws that copy:

- `scale` multiplies the translation of every child of a matching parent, which
  suits stacks: spreading only along z pulls layers apart without moving them
  sideways.
- `offset` adds a fixed shift to matching children, which suits parts nested
  inside others, such as the neutrino detector inside the muon shield.

The copy overlaps itself and is only for drawing. Do not simulate with it.

The Cavern is a single solid, so it has nothing to pull apart and shows a cutaway
view instead.

## Volume budget

The figures are meant to run in CI, so the number of volumes drawn is limited.
Fibres make up nearly all of the placements in the neutrino detector and the
calorimeter, so they are not rendered: the `envelopes` in `views.toml` are drawn
as solids and marked `/vis/geometry/set/daughtersInvisible`, which stops Geant4
descending below them. An air mother gets its own legend entry, so that it does
not read as a material.

Each scene prints how many volumes it draws, and rendering fails above
`max_drawn_volumes`. If that fails, add an envelope rather than raising the limit.

## Colours

Within a subsystem, colour means material, from the
[Okabe-Ito](https://jfly.uni-koeln.de/color/) palette, and a material has the
same colour in every subsystem. The full detector is coloured by subsystem with
[Paul Tol's muted](https://sronpersonalpages.nl/~pault/) palette, because nine
categories are more than Okabe-Ito offers. Its side view also labels subsystems
directly, so the figure does not depend on colour alone.

`--check-palette` prints the closest pair of colours under normal vision and
simulated protanopia, deuteranopia and tritanopia. Rendering warns if a figure's
legend has a pair closer than `min_palette_separation`.

## Adding a subsystem

Add a `[scene.<Name>]` section to `views.toml`, spelling `<Name>` as
`build_geometry --list` does, with its `explode` rules. Add any new material to
`material_colours` (or to `hidden_materials`), and for the full detector add the
subsystem to `subsystem_colours` and `direct_labels`.

## The interactive viewer

These figures do not use the interactive viewer. If
`pixi run fullSimLight -g <db> -i` aborts with `Could not initialize GLX`, Qt
could not get a usable OpenGL configuration from the display. Compare
`glxinfo -B` with that command: if `glxinfo` works, suspect the environment's
libraries rather than the display.
