#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration
"""Compare a gmclash report with the list of known clashes.

Fails on a volume that clashes but is not in the list, on a known volume that
no longer clashes (fixed: remove it from the list) and on a known clash
without an issue number. A known clash that comes out deeper than its recorded
bound is only a warning. Each volume may appear only once in the list.

Geant4 reports at most one clash per checked volume, against whichever partner
the deepest of its random points fell in, and that depth is a lower bound on
the real overlap. gmclash uses a fixed seed, but the random sequence runs on
from one volume to the next, so adding or removing volumes anywhere moves the
points sampled in every volume checked later. The partner, the type of clash
and the depth of an unchanged clash can then all change. The check therefore
matches clashes by volume alone and does not fail on depth; the partner and
type in the list are only for information.

  check_clashes.py REPORT KNOWN           check REPORT against KNOWN
  check_clashes.py REPORT KNOWN --update  rewrite KNOWN from REPORT, keeping
                                          the issue numbers, partners and
                                          deepest bounds of known clashes
"""

import argparse
import json
import sys

CLASH_TYPES = {0: "mother", 1: "sister", 2: "encapsulated", 3: "invalid solid"}

# Slack on the recorded depth before a deeper clash is reported:
# max(ABS_TOLERANCE_MM, REL_TOLERANCE * depth).
ABS_TOLERANCE_MM = 1.0
REL_TOLERANCE = 0.05


def read_report(path):
    with open(path) as f:
        report = json.load(f)["ClashesReport"]
    clashes = {}
    for c in report:
        clash = {
            "type": CLASH_TYPES.get(c["typeOfClash"], str(c["typeOfClash"])),
            "volume": c["volume1Name"],
            "other": c["volume2Name"],
            "distance_mm": round(c["distance[mm]"], 1),
        }
        previous = clashes.get(clash["volume"])
        if previous is None or clash["distance_mm"] > previous["distance_mm"]:
            clashes[clash["volume"]] = clash
    return clashes


def read_known(path):
    with open(path) as f:
        entries = json.load(f)
    known = {}
    duplicates = set()
    for c in entries:
        if c["volume"] in known:
            duplicates.add(c["volume"])
        known[c["volume"]] = c
    if duplicates:
        # Clashes are matched by volume, so a second entry would be dropped.
        sys.exit(f"duplicate volumes in {path}: {', '.join(sorted(duplicates))}")
    return known


def allowed(depth):
    return depth + max(ABS_TOLERANCE_MM, REL_TOLERANCE * depth)


def describe(c):
    return f"{c['type']:>12}  {c['volume']}  vs  {c['other']}"


def check(found, known):
    failures = []
    warnings = []
    for volume, c in sorted(found.items()):
        if volume not in known:
            failures.append(f"new        {describe(c)}  ({c['distance_mm']} mm)")
            continue
        bound = known[volume]["max_distance_mm"]
        # Encapsulated clashes report a distance of -1, which is no depth.
        if bound >= 0 and c["distance_mm"] > allowed(bound):
            warnings.append(
                f"deeper     {describe(c)}  ({c['distance_mm']} mm, "
                f"known {bound} mm, #{known[volume]['issue']})"
            )
    for volume, c in sorted(known.items()):
        if volume not in found:
            failures.append(
                f"fixed      {describe(c)}  (#{c['issue']}): "
                "remove it from the known clashes"
            )
        issue = c.get("issue")
        if isinstance(issue, bool) or not isinstance(issue, int) or issue <= 0:
            failures.append(f"no issue   {describe(c)}")
    return failures, warnings


def update(found, known, path):
    entries = []
    for volume, c in sorted(found.items()):
        old = known.get(volume)
        if old is None:
            entry = {
                "type": c["type"],
                "volume": volume,
                "other": c["other"],
                "max_distance_mm": c["distance_mm"],
                "issue": None,
            }
        else:
            # Keep the recorded clash and only raise its bound, so that the
            # list does not change each time the sampling moves.
            entry = dict(old)
            entry["max_distance_mm"] = max(old["max_distance_mm"], c["distance_mm"])
        entries.append(entry)
    entries.sort(key=lambda e: (e["type"], e["volume"], e["other"]))
    with open(path, "w") as f:
        json.dump(entries, f, indent=2)
        f.write("\n")
    missing = sum(1 for e in entries if e["issue"] is None)
    print(f"wrote {len(entries)} clashes to {path}, {missing} without an issue")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("report", help="gmclash JSON report")
    parser.add_argument("known", help="JSON list of known clashes")
    parser.add_argument(
        "--update", action="store_true", help="rewrite KNOWN from REPORT"
    )
    args = parser.parse_args()

    found = read_report(args.report)
    known = read_known(args.known)
    if args.update:
        update(found, known, args.known)
        return 0

    failures, warnings = check(found, known)
    print(f"{len(found)} clashes found, {len(known)} known")
    for w in warnings:
        print(f"warning: {w}")
    for f in failures:
        print(f)
    if failures:
        print(
            f"\n{len(failures)} problems. If the change is intended, run\n"
            f"  {sys.argv[0]} {args.report} {args.known} --update\n"
            "and give every new clash an issue number."
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
