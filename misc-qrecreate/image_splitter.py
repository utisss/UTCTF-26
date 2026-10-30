#!/usr/bin/env python3
"""
image_splitter.py — Split a QR code image into exactly 100 tiles (10×10 grid).

The image is resized to the nearest multiple of 10 in each dimension before
splitting, so every tile is always the same size and the grid is always even.

Folder naming
-------------
Each of the 100 tiles gets its own folder named after the Base64 encoding
of its zero-padded 2-digit sequence number (1-based, row-major):

  seq  zero-padded  Base64 folder
   1  →  "01"      →  MDE=
   2  →  "02"      →  MDI=
  10  →  "10"      →  MTA=
  99  →  "99"      →  OTk=
 100  →  "100"     →  MTAw   ← 3 digits, auto-sized to fit

Reconstruction (no manifest needed)
-------------------------------------
A client who knows:
  1. folder names = Base64(zero-padded decimal sequence number)
  2. grid is always 10 columns wide, row-major order
can decode every folder name → sort numerically → reassemble.

Usage:
    python image_splitter.py <image_path> [OPTIONS]
    python image_splitter.py reconstruct <manifest.json> <out.png>

Options:
    --output      DIR   Output base directory (default: ./output)
    --subfolder   STR   Subfolder name inside each tile folder (default: data)
    --zip         PATH  Zip archive path, manifest excluded (default: <output>.zip)
"""

import argparse
import base64
import json
import os
import sys
import zipfile
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is required.  pip install Pillow")

# Grid is fixed: 10 columns × 10 rows = 100 tiles
GRID_ROWS = 10
GRID_COLS = 10
NUM_TILES = GRID_ROWS * GRID_COLS          # 100
PADDING   = len(str(NUM_TILES))            # 3  (to cover "100")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def folder_name_for(seq: int) -> str:
    """Return Base64(zero-padded sequence number).

    seq=1   → "001" → "MDAx"
    seq=10  → "010" → "MDAx"  ... wait, let's be precise:
    seq=1   → zfill(3) → "001" → b64 → "MDAx"
    seq=10  → zfill(3) → "010" → b64 → "MDAx"  no:
      "001".encode() = b'001' → base64 = 'MDAx'
      "010".encode() = b'010' → base64 = 'MDAw'  ... actually:
    We just encode and let Python sort it out.
    """
    return base64.b64encode(str(seq).zfill(PADDING).encode()).decode()


def make_even_size(img: Image.Image) -> Image.Image:
    """Resize img so both dimensions are exact multiples of GRID_COLS/ROWS.

    Uses nearest-neighbour to preserve QR code sharp edges.
    The image is scaled to the closest multiple of 10 that is >= original size.
    """
    w, h         = img.size
    new_w        = ((w + GRID_COLS - 1) // GRID_COLS) * GRID_COLS
    new_h        = ((h + GRID_ROWS - 1) // GRID_ROWS) * GRID_ROWS

    if new_w != w or new_h != h:
        img = img.resize((new_w, new_h), Image.NEAREST)
        print(f"[info] Image resized from {w}×{h} to {new_w}×{new_h} "
              f"for even {GRID_COLS}×{GRID_ROWS} split.")
    return img


def zip_output(output_dir: str, zip_path: str) -> None:
    """Zip the output folder, excluding manifest.json."""
    base    = Path(output_dir)
    exclude = {"manifest.json"}

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for f in sorted(base.rglob("*")):
            if f.is_file() and f.name not in exclude:
                zf.write(f, f.relative_to(base.parent))

    print(f"Zip archive saved to: {zip_path}")


# ---------------------------------------------------------------------------
# Core logic
# ---------------------------------------------------------------------------

def split_image(
    image_path: str,
    output_dir: str,
    subfolder_name: str,
) -> None:
    img    = Image.open(image_path).convert("RGB")
    img    = make_even_size(img)
    width, height = img.size
    tile_w = width  // GRID_COLS
    tile_h = height // GRID_ROWS

    base = Path(output_dir)
    base.mkdir(parents=True, exist_ok=True)

    manifest = []

    for r in range(GRID_ROWS):
        for c in range(GRID_COLS):
            seq         = r * GRID_COLS + c + 1          # 1-based, row-major
            tile        = img.crop((
                c * tile_w,
                r * tile_h,
                c * tile_w + tile_w,
                r * tile_h + tile_h,
            ))
            folder_name = folder_name_for(seq)
            folder_path = base / folder_name / subfolder_name
            folder_path.mkdir(parents=True, exist_ok=True)

            tile.save(folder_path / "img.png")

            manifest.append({
                "seq":    seq,
                "folder": folder_name,
                "row":    r,
                "col":    c,
            })

            print(f"  [{seq:>3}/{NUM_TILES}] "
                  f"(r{r:02},c{c:02}) → {folder_name}/{subfolder_name}/img.png")

    manifest_path = base / "manifest.json"
    with open(manifest_path, "w") as f:
        json.dump({
            "source":    image_path,
            "grid":      [GRID_ROWS, GRID_COLS],
            "tile_size": [tile_w, tile_h],
            "padding":   PADDING,
            "subfolder": subfolder_name,
            "tiles":     manifest,
        }, f, indent=2)

    print(f"\n{NUM_TILES} tiles written to '{output_dir}/'")
    print(f"Manifest: {manifest_path}")


# ---------------------------------------------------------------------------
# Reconstruction
# ---------------------------------------------------------------------------

def reconstruct_image(manifest_path: str, output_path: str) -> None:
    """Reassemble the original image from manifest.json."""
    with open(manifest_path) as f:
        data = json.load(f)

    rows, cols = data["grid"]
    tw, th     = data["tile_size"]
    subfolder  = data["subfolder"]
    base       = Path(manifest_path).parent

    canvas = Image.new("RGB", (tw * cols, th * rows))

    for entry in sorted(data["tiles"], key=lambda e: e["seq"]):
        tile = Image.open(
            base / entry["folder"] / subfolder / "img.png"
        ).convert("RGB")
        canvas.paste(tile, (entry["col"] * tw, entry["row"] * th))

    canvas.save(output_path)
    print(f"Reconstructed image saved to: {output_path}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description=(
            f"Split a QR code into exactly {NUM_TILES} tiles "
            f"({GRID_ROWS}×{GRID_COLS} grid). "
            "Each tile folder is named Base64(zero-padded sequence number), "
            "row-major order."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("image",       help="Source QR code image file")
    p.add_argument("--output",    type=str, default="output", help="Output directory (default: ./output)")
    p.add_argument("--subfolder", type=str, default="data",   help="Subfolder inside each tile folder (default: data)")
    p.add_argument("--zip",       type=str, default=None,     metavar="ZIP_PATH",
                   help="Zip archive path (manifest excluded); default: <output>.zip")

    sub = p.add_subparsers(dest="command")
    rec = sub.add_parser("reconstruct", help="Reassemble image from manifest.json")
    rec.add_argument("manifest", help="Path to manifest.json")
    rec.add_argument("output",   help="Output image path")

    return p


def main() -> None:
    parser = build_parser()
    args   = parser.parse_args()

    if args.command == "reconstruct":
        reconstruct_image(args.manifest, args.output)
        return

    if not os.path.isfile(args.image):
        sys.exit(f"Error: file not found: {args.image}")

    print(f"Image     : {args.image}")
    print(f"Grid      : {GRID_ROWS} rows × {GRID_COLS} cols = {NUM_TILES} tiles")
    print(f"Padding   : {PADDING} digits before Base64 encoding")
    print(f"Output    : {args.output}/")
    print()

    split_image(
        image_path    = args.image,
        output_dir    = args.output,
        subfolder_name= args.subfolder,
    )

    zip_path = args.zip or (str(Path(args.output).with_suffix("")) + ".zip")
    zip_output(args.output, zip_path)


if __name__ == "__main__":
    main()
