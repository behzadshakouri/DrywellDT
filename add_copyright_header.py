#!/usr/bin/env python3
"""
add_copyright_header.py

Prepends a GPL v3 copyright header to all .cpp and .h files in the
OpenHydroTwin root folder (top-level only, no recursion).

Usage:
    python3 add_copyright_header.py /path/to/OpenHydroTwin
    python3 add_copyright_header.py            # uses current directory
    python3 add_copyright_header.py --dry-run  # show what would change
"""

import argparse
import sys
from datetime import datetime
from pathlib import Path

AUTHOR = "Arash Massoudieh"
PROJECT = "OpenHydroTwin"
YEAR = datetime.now().year

HEADER = f"""/*
 * {PROJECT}
 * Copyright (C) {YEAR}  {AUTHOR}
 *
 * This file is part of {PROJECT}.
 *
 * {PROJECT} is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * {PROJECT} is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

"""

EXTENSIONS = {".cpp", ".h"}


def prepend_header(file_path: Path, dry_run: bool = False) -> bool:
    """Prepend HEADER to file_path. Returns True if modified."""
    try:
        original = file_path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        # Fall back for files with non-UTF-8 encoding (rare in C++ source)
        original = file_path.read_text(encoding="latin-1")

    new_content = HEADER + original

    if dry_run:
        print(f"[dry-run] would update: {file_path.name}")
        return True

    file_path.write_text(new_content, encoding="utf-8")
    print(f"updated: {file_path.name}")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepend GPL v3 copyright header to .cpp/.h files."
    )
    parser.add_argument(
        "folder",
        nargs="?",
        default=".",
        help="OpenHydroTwin root folder (default: current directory)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show which files would be modified without writing",
    )
    args = parser.parse_args()

    root = Path(args.folder).resolve()
    if not root.is_dir():
        print(f"Error: {root} is not a directory", file=sys.stderr)
        return 1

    print(f"Scanning {root} (top-level only)...")

    # Top-level only — no recursion
    targets = sorted(
        p for p in root.iterdir()
        if p.is_file() and p.suffix in EXTENSIONS
    )

    if not targets:
        print("No .cpp or .h files found.")
        return 0

    for f in targets:
        prepend_header(f, dry_run=args.dry_run)

    action = "would be modified" if args.dry_run else "modified"
    print(f"\nDone. {len(targets)} file(s) {action}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
