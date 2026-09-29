#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) CERN for the benefit of the SHiP Collaboration
#
# Cut a release: bump CMakeLists.txt VERSION, regenerate CHANGELOG.md via
# git-cliff, create a release commit and an annotated tag. Does NOT push.

set -euo pipefail

usage() {
    cat <<'EOF'
Usage: scripts/release.sh <version>

  <version>   semver without leading 'v', e.g. 0.2.0

The script must be run from a clean working tree. It will:
  1. bump the VERSION line in CMakeLists.txt
  2. bump version and date-released in CITATION.cff (if present)
  3. bump the [package] version in pixi.toml (if present)
  4. regenerate CHANGELOG.md with `git cliff --tag v<version>`
  5. create commit `chore(release): v<version>`
  6. create annotated tag `v<version>`

Pushing is left to the operator:
  git push origin <branch> && git push origin v<version>
EOF
}

if [[ $# -ne 1 ]]; then
    usage >&2
    exit 64
fi

case "$1" in
    -h|--help) usage; exit 0 ;;
esac

VERSION="$1"
TAG="v${VERSION}"

if ! [[ "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "error: version must match MAJOR.MINOR.PATCH (got: ${VERSION})" >&2
    exit 64
fi

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "${REPO_ROOT}"

if ! git cliff --version >/dev/null 2>&1; then
    echo "error: 'git cliff' not available; install git-cliff (https://git-cliff.org/)" >&2
    exit 69
fi

if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "error: working tree is dirty; commit or stash changes first" >&2
    exit 65
fi

BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [[ "${BRANCH}" != "main" ]]; then
    echo "warning: on branch '${BRANCH}', not 'main'" >&2
fi

if git rev-parse --verify --quiet "refs/tags/${TAG}" >/dev/null; then
    echo "error: tag ${TAG} already exists" >&2
    exit 65
fi

# Everything below mutates the working tree. Any failure -- a read-only file, a
# `git cliff` error, an interrupted commit -- must leave the repository as we
# found it, so record each file as it is first touched and restore the lot on a
# non-zero exit. `git checkout HEAD --` (rather than `git checkout --`) also
# resets the index, covering failures after the files have been staged.
RELEASE_FILES=()
restore_release_files() {
    local status=$?
    if [[ ${status} -eq 0 ]]; then
        return 0
    fi
    local file
    for file in ${RELEASE_FILES[@]+"${RELEASE_FILES[@]}"}; do
        # Best effort, one file at a time: a path git cannot restore (an
        # as-yet-untracked CHANGELOG.md, say) must not block the others.
        git checkout HEAD -- "${file}" 2>/dev/null || true
    done
    return 0
}
trap restore_release_files EXIT

CMAKE_FILE="CMakeLists.txt"
if ! grep -qE '^[[:space:]]*VERSION[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+' "${CMAKE_FILE}"; then
    echo "error: could not find VERSION line in ${CMAKE_FILE}" >&2
    exit 70
fi

RELEASE_FILES+=("${CMAKE_FILE}")
sed -i -E "s/^([[:space:]]*VERSION[[:space:]]+)[0-9]+\.[0-9]+\.[0-9]+/\1${VERSION}/" "${CMAKE_FILE}"

if ! grep -qE "^[[:space:]]*VERSION[[:space:]]+${VERSION//./\\.}([[:space:]]|$)" "${CMAKE_FILE}"; then
    echo "error: failed to update VERSION in ${CMAKE_FILE}" >&2
    exit 70
fi

# A present-but-unbumpable CITATION.cff is an error, not a silent no-op: the
# release commit would otherwise stage a stale citation version.
CITATION_FILE="CITATION.cff"
if [[ -f "${CITATION_FILE}" ]]; then
    if ! grep -qE '^version: ' "${CITATION_FILE}"; then
        echo "error: could not find version line in ${CITATION_FILE}" >&2
        exit 70
    fi
    RELEASE_FILES+=("${CITATION_FILE}")
    sed -i -E "s/^version: .*/version: ${VERSION}/" "${CITATION_FILE}"
    sed -i -E "s/^date-released: .*/date-released: \"$(date -u +%Y-%m-%d)\"/" "${CITATION_FILE}"
    if ! grep -qE "^version: ${VERSION//./\\.}$" "${CITATION_FILE}"; then
        echo "error: failed to update version in ${CITATION_FILE}" >&2
        exit 70
    fi
fi

# Bump the [package] version in pixi.toml so the source dependency and the
# conda recipe stay in lockstep with the tag. A pixi.toml without a [package]
# section has no version to bump, so the top-level key being absent is a clean
# no-op; but exactly one must match when it is present, because anything else
# means the anchor no longer picks out the key we think it does -- and a silent
# skip here would tag a release carrying a stale package version. Anchored at
# column 0, it never matches the inline version = fields of
# [package.build]/host-dependency tables.
PIXI_FILE="pixi.toml"
if [[ -f "${PIXI_FILE}" ]]; then
    PIXI_MATCHES="$(grep -cE '^version = "[0-9]+\.[0-9]+\.[0-9]+"$' "${PIXI_FILE}" || true)"
    if [[ "${PIXI_MATCHES}" -gt 1 ]]; then
        echo "error: expected at most one top-level version key in ${PIXI_FILE} (found ${PIXI_MATCHES})" >&2
        exit 70
    fi
    if [[ "${PIXI_MATCHES}" -eq 1 ]]; then
        RELEASE_FILES+=("${PIXI_FILE}")
        sed -i -E "s/^version = \"[0-9]+\.[0-9]+\.[0-9]+\"$/version = \"${VERSION}\"/" "${PIXI_FILE}"
        if ! grep -qE "^version = \"${VERSION//./\\.}\"$" "${PIXI_FILE}"; then
            echo "error: failed to update version in ${PIXI_FILE}" >&2
            exit 70
        fi
    elif grep -qE '^\[package\]' "${PIXI_FILE}"; then
        echo "error: ${PIXI_FILE} has a [package] section but no bumpable top-level version key" >&2
        exit 70
    fi
fi

RELEASE_FILES+=("CHANGELOG.md")
git cliff --tag "${TAG}" -o CHANGELOG.md

git add "${RELEASE_FILES[@]}"
git commit -m "chore(release): ${TAG}"
# Committed: those files are no longer ours to restore, so a failing `git tag`
# must not roll the release commit's contents back.
trap - EXIT

git tag -a "${TAG}" -m "Release ${TAG}"

cat <<EOF

Release ${TAG} prepared on branch '${BRANCH}'.

Next steps:
  git push origin ${BRANCH}
  git push origin ${TAG}

(or:  git push --follow-tags origin ${BRANCH})
EOF
