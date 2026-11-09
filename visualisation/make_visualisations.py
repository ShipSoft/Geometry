#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration
"""Render the standard geometry figures with fullSimLight.

For each subsystem, and for the full detector, this builds the geometry, writes
a Geant4 macro per scene, renders it offscreen, and has Typst lay the images out
as a PDF with one page per view. See visualisation/README.md.
"""

from __future__ import annotations

import argparse
import datetime
import json
import math
import os
import re
import shutil
import sqlite3
import struct
import subprocess
import sys
import tomllib
import zlib
from dataclasses import dataclass, field
from pathlib import Path


HERE = Path(__file__).resolve().parent
REPO = HERE.parent
FULL = "ship_geometry"

# The renderer fits the scene's bounding sphere to the shorter image side, so a
# square image always holds the longest axis. Embedded as it is: at half this size,
# the trackers' straws show moire.
DEFAULT_RENDER_PX = 6000
CROP_MARGIN_PX = 24

# Machado, Oliveira & Fernandes (2009), severity 1.0, applied in linear RGB.
CVD_MATRICES = {
    "protanopia": [[0.152286, 1.052583, -0.204868], [0.114503, 0.786281, 0.099216], [-0.003882, -0.048116, 1.051998]],
    "deuteranopia": [[0.367322, 0.860646, -0.227968], [0.280085, 0.672501, 0.047413], [-0.011820, 0.042940, 0.968881]],
    "tritanopia": [[1.255528, -0.076749, -0.178779], [-0.078411, 0.930809, 0.147602], [0.004733, 0.691367, 0.303900]],
}


def hex_to_rgb(colour: str) -> tuple[float, float, float]:
    """Convert ``#rrggbb`` to floats in [0, 1]."""
    return tuple(int(colour[i : i + 2], 16) / 255 for i in (1, 3, 5))


def to_linear(rgb: tuple[float, ...]) -> list[float]:
    """Undo the sRGB transfer function."""
    return [c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4 for c in rgb]


def transform(matrix: list[list[float]], vector: list[float]) -> list[float]:
    """Multiply a vector by a matrix."""
    return [sum(m * v for m, v in zip(row, vector)) for row in matrix]


def to_lab(linear: list[float]) -> list[float]:
    """Convert linear sRGB to CIE L*a*b* (D65)."""
    clipped = [min(max(c, 0.0), 1.0) for c in linear]
    xyz = transform([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]], clipped)
    t = [c / white for c, white in zip(xyz, (0.9505, 1.0, 1.089))]
    f = [c ** (1 / 3) if c > 0.008856 else 7.787 * c + 16 / 116 for c in t]
    return [116 * f[1] - 16, 500 * (f[0] - f[1]), 200 * (f[1] - f[2])]


def palette_separation(colours: dict[str, str]) -> dict[str, tuple[float, str, str]]:
    """Return the closest pair (CIE76 distance) with normal and simulated CVD vision."""
    result = {}
    names = list({colour: name for name, colour in colours.items()}.values())
    for vision in ("normal", *CVD_MATRICES):
        labs = {}
        for name in names:
            linear = to_linear(hex_to_rgb(colours[name]))
            if vision != "normal":
                linear = transform(CVD_MATRICES[vision], linear)
            labs[name] = to_lab(linear)
        pairs = [(math.dist(labs[a], labs[b]), a, b) for i, a in enumerate(names) for b in names[i + 1 :]]
        if pairs:
            result[vision] = min(pairs)
    return result


@dataclass
class Scene:
    """One subsystem's figure settings, resolved from views.toml."""

    name: str
    title: str
    hide: list[str] = field(default_factory=list)
    views: dict[str, dict] = field(default_factory=dict)
    explode: dict | None = None
    segments: int = 24


def load_config(path: Path) -> tuple[dict, dict[str, Scene]]:
    """Read views.toml, filling each scene's views from the defaults."""
    config = tomllib.loads(path.read_text())
    defaults = config["default_views"]
    scenes = {}
    for name, section in config["scene"].items():
        views = {k: dict(v) for k, v in defaults.items() if k not in section.get("skip_views", [])}
        for key, overrides in section.get("views", {}).items():
            views[key] = {**views.get(key, {}), **overrides}
        scenes[name] = Scene(name, section["title"], section.get("hide", []), views, section.get("explode"), section.get("segments", 24))
    return config, scenes


def logical_volumes(db_path: Path) -> list[tuple[str, str]]:
    """Return every distinct (logical volume, material) name pair in a database."""
    with sqlite3.connect(db_path) as db:
        rows = db.execute("SELECT DISTINCT l.name, m.name FROM LogVols l JOIN Materials m ON l.material = m.id")
        return sorted(rows)


def build_databases(build_dir: Path, work: Path, names: list[str]) -> dict[str, Path]:
    """Build a fresh database per scene, so figures always match the source."""
    app = build_dir / "apps" / "build_geometry"
    databases = {}
    for name in names:
        db = work / f"{name}.db"
        command = [app, "-o", db] if name == FULL else [app, "--standalone", name, "-o", db]
        # fullSimLight logs to <scene>.log, so this needs a name of its own.
        with (work / f"{name}_build.log").open("w") as output:
            subprocess.run(command, check=True, stdout=output, stderr=subprocess.STDOUT)
        databases[name] = db
    return databases


def explode_database(source: Path, target: Path, rules: dict) -> int:
    """Copy a database, moving placements apart. Returns the placements moved.

    Geant4's explodeFactor only exists in its OpenGL viewers, so the layout is
    spread here instead. ``scale`` rules multiply the translation of each child
    of a matching parent; ``offset`` rules add to it for matching children. The
    copy overlaps itself and is for drawing only: never simulate with it.
    """
    shutil.copyfile(source, target)
    scale_rules = [(re.compile(r["parent"]), r["scale"]) for r in rules.get("scale", [])]
    offset_rules = [(re.compile(r["child"]), r["offset"]) for r in rules.get("offset", [])]
    moved = 0
    with sqlite3.connect(target) as db:
        types = dict(db.execute("SELECT id, nodeType FROM GeoNodesTypes"))
        tables = {"GeoTransform": "Transforms", "GeoAlignableTransform": "AlignableTransforms"}
        logvol = {}
        for node, table in (("GeoPhysVol", "PhysVols"), ("GeoFullPhysVol", "FullPhysVols")):
            for pv, name in db.execute(f"SELECT p.id, l.name FROM {table} p JOIN LogVols l ON p.logvol = l.id"):
                logvol[(node, pv)] = name
        rows = db.execute(
            "SELECT id, parentId, parentTable, childTable, childId FROM ChildrenPositions"
            " ORDER BY parentTable, parentId, parentCopyNumber, position"
        ).fetchall()
        updates = []
        pending, pending_key = None, None
        for row_id, parent, parent_table, child_table, child in rows:
            key = (types[parent_table], parent)
            node = types[child_table]
            if node in tables:
                pending, pending_key = (row_id, node, child), key
            elif (node, child) in logvol and pending and pending_key == key:
                parent_name = logvol.get(key, "")
                factor = [1.0, 1.0, 1.0]
                for pattern, scale in scale_rules:
                    if pattern.fullmatch(parent_name):
                        factor = [f * s for f, s in zip(factor, scale)]
                shift = [0.0, 0.0, 0.0]
                for pattern, offset in offset_rules:
                    if pattern.fullmatch(logvol[(node, child)]):
                        shift = [a + b for a, b in zip(shift, offset)]
                if factor != [1.0, 1.0, 1.0] or shift != [0.0, 0.0, 0.0]:
                    updates.append((pending, factor, shift))
                pending = None
        cursor = db.cursor()
        for (row_id, node, transform), factor, shift in updates:
            # Transforms can be shared between placements, so each gets its own.
            table = tables[node]
            values = cursor.execute(
                f"SELECT xx, xy, xz, yx, yy, yz, zx, zy, zz, dx, dy, dz FROM {table} WHERE id = ?", (transform,)
            ).fetchone()
            moved_d = [v * f + s for v, f, s in zip(values[9:], factor, shift)]
            cursor.execute(
                f"INSERT INTO {table} (xx, xy, xz, yx, yy, yz, zx, zy, zz, dx, dy, dz) VALUES (?,?,?,?,?,?,?,?,?,?,?,?)",
                (*values[:9], *moved_d),
            )
            cursor.execute("UPDATE ChildrenPositions SET childId = ? WHERE id = ?", (cursor.lastrowid, row_id))
            moved += 1
    return moved


def colour_map(config: dict, scene: Scene, databases: dict[str, Path], subsystem_lvs: dict[str, set[str]]) -> tuple[dict, dict]:
    """Assign each logical volume a colour. Returns (volume -> colour, legend)."""
    hidden_materials = set(config["hidden_materials"])
    hidden_names = [re.compile(p) for p in scene.hide]
    envelopes = [(re.compile(e["pattern"]), e) for e in config["envelopes"]]
    colours, legend = {}, {}
    for volume, material in logical_volumes(databases[scene.name]):
        colours.setdefault(volume, None)
        envelope = next((e for pattern, e in envelopes if pattern.fullmatch(volume)), None)
        if any(p.fullmatch(volume) for p in hidden_names) or (material in hidden_materials and envelope is None):
            continue
        if scene.name == FULL:
            label = next((s for s, names in subsystem_lvs.items() if volume in names), None)
            if label is None:
                continue
            colours[volume] = config["subsystem_colours"][label]["colour"]
        elif envelope and "label" in envelope:
            colours[volume] = legend.setdefault(envelope["label"], envelope["colour"])
        else:
            colours[volume] = legend.setdefault(material, config["material_colours"][material])
    if scene.name == FULL:
        # Listed along the beam, as views.toml orders them.
        legend = {s["label"]: s["colour"] for s in config["subsystem_colours"].values()}
    return colours, legend


def drawn_volumes(db_path: Path, colours: dict[str, str | None], envelopes: list[re.Pattern]) -> int:
    """Count the placements a macro draws: visible ones, and none below an envelope."""
    with sqlite3.connect(db_path) as db:
        types = dict(db.execute("SELECT id, nodeType FROM GeoNodesTypes"))
        names = {}
        for node, table in (("GeoPhysVol", "PhysVols"), ("GeoFullPhysVol", "FullPhysVols")):
            for pv, name in db.execute(f"SELECT p.id, l.name FROM {table} p JOIN LogVols l ON p.logvol = l.id"):
                names[(node, pv)] = name
        serial = {i: ((types[table], volume), copies) for i, volume, table, copies in db.execute("SELECT id, volId, volTable, copies FROM SerialTransformers")}
        children = {}
        for parent, parent_table, child_table, child in db.execute("SELECT parentId, parentTable, childTable, childId FROM ChildrenPositions"):
            node = types[child_table]
            if (node, child) in names:
                children.setdefault((types[parent_table], parent), []).append(((node, child), 1))
            elif node == "GeoSerialTransformer":
                children.setdefault((types[parent_table], parent), []).append(serial[child])
        volume, table = db.execute("SELECT volId, volTable FROM RootVolume").fetchone()
    is_envelope = {name: any(p.fullmatch(name) for p in envelopes) for name in set(names.values())}
    counted: dict[tuple, int] = {}

    def count(key: tuple) -> int:
        """Count the placements drawn at and below one volume."""
        # Shared volumes are visited once, so the many placements cost nothing.
        if key not in counted:
            name = names[key]
            own = 1 if colours.get(name) else 0
            counted[key] = own if is_envelope[name] else own + sum(n * count(c) for c, n in children.get(key, []))
        return counted[key]

    return count((types[table], volume))


def visibility_commands(colours: dict[str, str | None], force_hidden: list[re.Pattern] | None = None) -> list[str]:
    """Macro commands that hide containers and colour everything else."""
    lines = []
    for volume, colour in colours.items():
        if colour is None or any(p.fullmatch(volume) for p in force_hidden or []):
            lines.append(f"/vis/geometry/set/visibility {volume} 0 false")
        else:
            r, g, b = hex_to_rgb(colour)
            lines.append(f"/vis/geometry/set/visibility {volume} 0 true")
            lines.append(f"/vis/geometry/set/colour {volume} 0 {r:.4f} {g:.4f} {b:.4f} 1")
    return lines


def write_macro(path: Path, scene: Scene, stem: str, views: dict[str, dict], colours: dict, envelopes: list[re.Pattern], render_px: int) -> None:
    """Write a macro drawing the given views. Images land in the working directory."""
    lines = [
        "# Generated by visualisation/make_visualisations.py. Run from a working directory with:",
        f"#   G4UI_USE_TCSH=1 fullSimLight -g {stem}.db -m {path.name} -i",
        "/control/verbose 0",
        "/run/initialize",
        f"/vis/open TOOLSSG_OFFSCREEN {render_px}x{render_px}",
        "/vis/viewer/set/autoRefresh false",
        "/vis/verbose errors",
        "/vis/drawVolume",
        "/vis/viewer/set/background white",
        "/vis/viewer/set/style surface",
        "/vis/viewer/set/projection orthogonal",
        f"/vis/viewer/set/lineSegmentsPerCircle {scene.segments}",
        "/vis/viewer/set/lightsMove camera",
        "/vis/viewer/set/lightsVector 0 0 1",
        "/vis/tsg/offscreen/set/format zb_png",
    ]
    # Geant4 does not even descend below these, which is what keeps fibres cheap.
    lines += [f"/vis/geometry/set/daughtersInvisible {v} 0 true" for v in colours if any(p.fullmatch(v) for p in envelopes)]
    previous = None
    for key, view in views.items():
        lines += [f"# ---- {key}: {view['label']}"]
        # Thousands of commands in a large scene, so only sent when they change.
        if view.get("hide", []) != previous:
            previous = view.get("hide", [])
            lines += visibility_commands(colours, [re.compile(p) for p in previous])
        vp, up = view["viewpoint"], view["up"]
        lines += ["/vis/viewer/clearCutawayPlanes"]
        if "cutaway_normal" in view:
            nx, ny, nz = view["cutaway_normal"]
            lines += [f"/vis/viewer/addCutawayPlane 0 0 0 m {nx} {ny} {nz}"]
        lines += [
            f"/vis/viewer/set/viewpointVector {vp[0]} {vp[1]} {vp[2]}",
            f"/vis/viewer/set/upVector {up[0]} {up[1]} {up[2]}",
            f"/vis/tsg/offscreen/set/file {stem}_{key}.png",
            "/vis/viewer/rebuild",
            "/vis/viewer/flush",
            "/vis/scene/list ! all",
        ]
    path.write_text("\n".join(lines) + "\n")


def run_fullsimlight(db: Path, macro: Path, work: Path, log: Path) -> list[tuple[float, list[float]]]:
    """Run fullSimLight headless. Returns each view's scene radius and centre in mm.

    fullSimLight only creates its vis manager with -i. G4UI_USE_TCSH replaces
    the Qt session with one that reads stdin, so neither a display nor GLX is
    involved, and the 'exit' on stdin ends the run.
    """
    env = {k: v for k, v in os.environ.items() if k not in ("DISPLAY", "WAYLAND_DISPLAY")}
    env["G4UI_USE_TCSH"] = "1"
    with log.open("w") as output:
        subprocess.run(
            ["fullSimLight", "-g", str(db), "-m", str(macro), "-i"],
            input="exit\n", text=True, cwd=work, env=env, stdout=output, stderr=subprocess.STDOUT, check=True,
        )
    text = log.read_text()
    if "COMMAND NOT FOUND" in text or "***** Illegal" in text:
        raise RuntimeError(f"Geant4 rejected a macro command, see {log}")
    limits = re.findall(
        r"Overall extent or bounding box.*?X limits: (\S+) (\S+)\s+Y limits: (\S+) (\S+)\s+Z limits: (\S+) (\S+)", text, re.S
    )
    scenes = []
    for x0, x1, y0, y1, z0, z1 in limits:
        low, high = [float(x0), float(y0), float(z0)], [float(x1), float(y1), float(z1)]
        scenes.append((math.dist(low, high) / 2, [(a + b) / 2 for a, b in zip(low, high)]))
    return scenes


def dot(a: list[float], b: list[float]) -> float:
    """Return the dot product of two vectors."""
    return sum(x * y for x, y in zip(a, b))


def cross(a: list[float], b: list[float]) -> list[float]:
    """Return the cross product of two 3-vectors."""
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def normalised(a: list[float]) -> list[float]:
    """Return a vector scaled to unit length."""
    length = math.hypot(*a)
    return [x / length for x in a]


def camera_axes(view: dict) -> tuple[list[float], list[float], list[float]]:
    """Return the image's right, up and out-of-screen directions in world axes."""
    out = normalised([float(x) for x in view["viewpoint"]])
    right = normalised(cross([float(x) for x in view["up"]], out))
    return right, cross(out, right), out


def drawn_area(image: Path) -> tuple[int, int, int, int]:
    """Return the left, top, right and bottom pixels of what a render draws.

    The renderer writes 8-bit RGBA with the Up filter on every row but the first,
    so a row is its difference from the row above and these need no decoding:
    wherever something is drawn, a difference appears as it starts and ends. The
    top row has to be background, which fitting the scene's bounding sphere to the
    image ensures, along with something being left below it.
    """
    data = image.read_bytes()
    position = 8
    compressed = []
    while position < len(data):
        length, kind = struct.unpack(">I4s", data[position : position + 8])
        body = data[position + 8 : position + 8 + length]
        if kind == b"IHDR":
            width, height, depth, colour_type = struct.unpack(">IIBB", body[:10])
            if (depth, colour_type) != (8, 6):
                raise RuntimeError(f"{image} is not 8-bit RGBA")
        elif kind == b"IDAT":
            compressed.append(body)
        position += 12 + length
    pixels = zlib.decompress(b"".join(compressed))

    stride = width * 4
    top_row = pixels[1 : stride + 1]
    if top_row != top_row[:4] * width:
        raise RuntimeError(f"{image} has no background along its top row")
    left, right = stride, -1
    top = bottom = None
    for y in range(1, height):
        start = y * (stride + 1)
        if pixels[start] != 2:
            raise RuntimeError(f"{image} has a row without the PNG Up filter, which drawn_area relies on")
        row = pixels[start + 1 : start + 1 + stride]
        rest = row.lstrip(b"\0")
        if rest:
            if top is None:
                top = y
            bottom = y - 1
            left = min(left, stride - len(rest))
            right = max(right, len(row.rstrip(b"\0")) - 1)
    if top is None:
        raise RuntimeError(f"{image} is blank")
    return left // 4, top, right // 4, bottom


def page_spec(scene: Scene, view: dict, image: Path, radius: float, centre: list[float], legend: dict, stamp: str, config: dict, render_px: int) -> dict:
    """Describe one PDF page for figure.typ."""
    right, up, out = camera_axes(view)
    # The renderer fits the scene's bounding sphere to the image, about its centre.
    px_per_mm = (render_px / 2) / radius
    labels = []
    if view.get("direct_labels") and scene.name == FULL:
        for item in config["direct_labels"]:
            point = [a - b for a, b in zip(item["at"], centre)]
            labels.append(
                {
                    "text": item["label"],
                    "dx": dot(point, right) * px_per_mm,
                    "dy": -dot(point, up) * px_per_mm,
                    "row": item.get("row", 1),
                    "align": item.get("ha", "center"),
                }
            )
    axes = []
    for name, axis in zip("xyz", ([1, 0, 0], [0, 1, 0], [0, 0, 1])):
        axes.append({"name": name, "dx": dot(axis, right), "dy": dot(axis, up), "toward_viewer": dot(axis, out) > 0})
    drawn_left, drawn_top, drawn_right, drawn_bottom = drawn_area(image)
    crop_left, crop_top = max(drawn_left - CROP_MARGIN_PX, 0), max(drawn_top - CROP_MARGIN_PX, 0)
    crop_right, crop_bottom = min(drawn_right + CROP_MARGIN_PX + 1, render_px), min(drawn_bottom + CROP_MARGIN_PX + 1, render_px)
    return {
        "title": scene.title,
        "subtitle": view["label"],
        "stamp": stamp,
        "image": str(image),
        "image_size": render_px,
        "crop": {"left": crop_left, "top": crop_top, "width": crop_right - crop_left, "height": crop_bottom - crop_top},
        "scale_bar": bool(view.get("orthographic_scale")),
        "pixels_per_mm": px_per_mm,
        "labels": labels,
        "axes": axes,
        "legend": [{"label": label, "colour": colour} for label, colour in legend.items()],
    }


def render_scene(scene: Scene, config: dict, databases: dict, subsystem_lvs: dict, work: Path, out: Path, stamp: str, render_px: int) -> None:
    """Render all of one scene's views and write its PDF."""
    colours, legend = colour_map(config, scene, databases, subsystem_lvs)
    for vision, (distance, a, b) in palette_separation(legend).items():
        if distance < config["min_palette_separation"]:
            print(f"warning: {scene.name}: {a} and {b} are close under {vision} ({distance:.0f})", file=sys.stderr)

    envelopes = [re.compile(e["pattern"]) for e in config["envelopes"]]
    drawn = drawn_volumes(databases[scene.name], colours, envelopes)
    print(f"{scene.name}: {drawn} volumes drawn")
    if drawn > config["max_drawn_volumes"]:
        raise RuntimeError(f"{scene.name} draws {drawn} volumes, over max_drawn_volumes = {config['max_drawn_volumes']}: add an envelope in views.toml")

    views = dict(scene.views)
    plain = {k: v for k, v in views.items() if not v.get("exploded")}
    exploded = {k: v for k, v in views.items() if v.get("exploded")}
    (out / "macros").mkdir(parents=True, exist_ok=True)
    pages = {}

    def render(stem: str, db: Path, subset: dict[str, dict]) -> None:
        """Render some of the scene's views from one database, and describe their pages."""
        macro = out / "macros" / f"{stem}.mac"
        write_macro(macro, scene, stem, subset, colours, envelopes, render_px)
        log = work / f"{stem}.log"
        scene_views = run_fullsimlight(db, macro, work, log)
        if len(scene_views) != len(subset):
            raise RuntimeError(f"expected {len(subset)} scene listings in {log}, found {len(scene_views)}")
        for (key, view), (radius, centre) in zip(subset.items(), scene_views):
            pages[key] = page_spec(scene, view, work / f"{stem}_{key}.png", radius, centre, legend, stamp, config, render_px)

    if plain:
        render(scene.name, databases[scene.name], plain)
    if exploded:
        db = work / f"{scene.name}_exploded.db"
        moved = explode_database(databases[scene.name], db, scene.explode)
        print(f"{scene.name}: moved {moved} placements for the exploded view")
        render(f"{scene.name}_exploded", db, exploded)

    spec = work / f"{scene.name}.json"
    spec.write_text(json.dumps({"title": f"SHiP geometry: {scene.title}", "pages": [pages[key] for key in views]}))
    subprocess.run(["typst", "compile", "--ignore-system-fonts", "--root", "/", "--input", f"spec={spec}", HERE / "figure.typ", out / "pdf" / f"{scene.name}.pdf"], check=True)
    print(f"{scene.name}: wrote {out / 'pdf' / (scene.name + '.pdf')}")


def main() -> int:
    """Parse the command line and render the selected scenes."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("names", nargs="*", help=f"scenes to render (subsystem names or {FULL}); default all")
    parser.add_argument("--build-dir", type=Path, default=REPO / "build", help="CMake build directory holding apps/build_geometry")
    parser.add_argument("--work-dir", type=Path, default=None, help="where databases, raw renders and logs go (default <build-dir>/figures)")
    parser.add_argument("--output", type=Path, default=HERE, help="where pdf/ and macros/ go")
    parser.add_argument("--render-px", type=int, default=DEFAULT_RENDER_PX, help="side of the square render")
    parser.add_argument("--check-palette", action="store_true", help="print each palette's colour-vision separation and exit")
    args = parser.parse_args()

    config, scenes = load_config(HERE / "views.toml")
    if args.check_palette:
        groups = {"materials": config["material_colours"], "subsystems": {k: v["colour"] for k, v in config["subsystem_colours"].items()}}
        for group, colours in groups.items():
            for vision, (distance, a, b) in palette_separation(colours).items():
                print(f"{group:11s} {vision:13s} closest pair {a} / {b}: {distance:.1f}")
        return 0

    selected = args.names or list(scenes)
    unknown = [n for n in selected if n not in scenes]
    if unknown:
        parser.error(f"unknown scene(s): {', '.join(unknown)}; choose from {', '.join(scenes)}")

    work = args.work_dir or args.build_dir / "figures"
    # Typst is needed only at the end of a long run, so check before starting it.
    for tool in ("fullSimLight", "typst"):
        if shutil.which(tool) is None:
            parser.error(f"{tool} is not on PATH: run this inside the pixi environment")
    work.mkdir(parents=True, exist_ok=True)
    (args.output / "pdf").mkdir(parents=True, exist_ok=True)

    # The full detector is coloured by subsystem, which needs every subsystem's volumes.
    subsystems = [n for n in scenes if n != FULL]
    needed = sorted(set(selected) | (set(subsystems) if FULL in selected else set()))
    databases = build_databases(args.build_dir.resolve(), work.resolve(), needed)
    subsystem_lvs = {n: {v for v, _ in logical_volumes(databases[n])} for n in subsystems if n in databases}

    revision = subprocess.run(["git", "-C", REPO, "describe", "--always", "--dirty"], capture_output=True, text=True).stdout.strip()
    stamp = f"SHiP geometry {revision}, rendered with fullSimLight on {datetime.date.today().isoformat()}"
    for name in selected:
        render_scene(scenes[name], config, databases, subsystem_lvs, work.resolve(), args.output.resolve(), stamp, args.render_px)
    return 0


if __name__ == "__main__":
    sys.exit(main())
