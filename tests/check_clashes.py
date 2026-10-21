#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration
"""Compare a gmclash report with the list of known clashes.

Fails on a clash that is not in the list, on a known clash that got deeper,
on a known clash that is gone (fixed: remove it from the list) and on a known
clash without an issue number.

Geant4 reports at most one clash per checked volume, and the distance is the
deepest of the random points it sampled, so it is a lower bound on the real
overlap. gmclash uses a fixed seed, so the report is reproducible for a given
geometry and number of points.

  check_clashes.py REPORT KNOWN           check REPORT against KNOWN
  check_clashes.py REPORT KNOWN --update  rewrite KNOWN from REPORT, keeping
                                          the issue numbers of known clashes
"""

import argparse
import json
import sys

CLASH_TYPES = {0: "mother", 1: "sister", 2: "encapsulated", 3: "invalid solid"}

# Slack on the recorded depth, so that a rounding difference between platforms
# does not fail the check: max(ABS_TOLERANCE_MM, REL_TOLERANCE * depth).
ABS_TOLERANCE_MM = 1.0
REL_TOLERANCE = 0.05


def key(clash):
    return (clash["type"], clash["volume"], clash["other"])


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
        k = key(clash)
        if k in clashes:
            clashes[k]["distance_mm"] = max(
                clashes[k]["distance_mm"], clash["distance_mm"]
            )
        else:
            clashes[k] = clash
    return clashes


def read_known(path):
    with open(path) as f:
        return {key(c): c for c in json.load(f)}


def allowed(depth):
    return depth + max(ABS_TOLERANCE_MM, REL_TOLERANCE * depth)


def describe(c):
    return f"{c['type']:>12}  {c['volume']}  vs  {c['other']}"


def check(found, known):
    failures = []
    for k, c in sorted(found.items()):
        if k not in known:
            failures.append(f"new        {describe(c)}  ({c['distance_mm']} mm)")
            continue
        bound = known[k]["max_distance_mm"]
        # Encapsulated clashes report a distance of -1, which is no depth.
        if bound >= 0 and c["distance_mm"] > allowed(bound):
            failures.append(
                f"deeper     {describe(c)}  ({c['distance_mm']} mm, "
                f"known {bound} mm, #{known[k]['issue']})"
            )
    for k, c in sorted(known.items()):
        if k not in found:
            failures.append(
                f"fixed      {describe(c)}  (#{c['issue']}): "
                "remove it from the known clashes"
            )
        if not c.get("issue"):
            failures.append(f"no issue   {describe(c)}")
    return failures


def update(found, known, path):
    entries = []
    for k, c in sorted(found.items()):
        entries.append(
            {
                "type": c["type"],
                "volume": c["volume"],
                "other": c["other"],
                "max_distance_mm": c["distance_mm"],
                "issue": known[k]["issue"] if k in known else None,
            }
        )
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

    failures = check(found, known)
    print(f"{len(found)} clashes found, {len(known)} known")
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
