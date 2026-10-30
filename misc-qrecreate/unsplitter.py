import os
import base64
import zipfile
import sys
from pathlib import Path
from PIL import Image
import tempfile
import math

try:
    from pyzbar.pyzbar import decode as qr_decode
except ImportError:
    qr_decode = None


def reconstruct_qr(zip_path: str) -> None:
    with tempfile.TemporaryDirectory() as tmpdir:
        # --- Unzip ---
        print(f"Extracting {zip_path}...")
        with zipfile.ZipFile(zip_path, "r") as zf:
            zf.extractall(tmpdir)

        # --- Collect tiles ---
        # Folder names are Base64(zero-padded 3-digit decimal), 1-based row-major.
        # Decode to integer, subtract 1 to get a 0-based position for grid math.
        tiles: dict[int, Image.Image] = {}

        root = Path(tmpdir)

        # Descend into a single top-level wrapper folder if present (e.g. "output/")
        top_level = [p for p in root.iterdir() if p.is_dir()]
        if len(top_level) == 1:
            root = top_level[0]

        for entry in root.iterdir():
            if not entry.is_dir():
                continue
            try:
                # Sequence numbers are 1-based; convert to 0-based for grid math
                position = int(base64.b64decode(entry.name).decode()) - 1
            except Exception:
                continue

            if position < 0:
                print(f"  Warning: decoded position < 0 for folder '{entry.name}', skipping.")
                continue

            img_path = entry / "data" / "img.png"
            if not img_path.exists():
                print(f"  Warning: missing image for tile at position {position}, skipping.")
                continue

            tiles[position] = Image.open(img_path).copy()

        if not tiles:
            print("No valid tiles found in the archive.")
            sys.exit(1)

        print(f"Found {len(tiles)} tile(s). Reconstructing image...")

        # --- Determine grid dimensions ---
        # The splitter always produces a 10×10 grid (100 tiles).
        # Use isqrt for the square case; fall back to smallest-factor split
        # (widest reasonable grid) only if tile count isn't a perfect square.
        total = len(tiles)
        sq    = math.isqrt(total)

        if sq * sq == total:
            cols = sq
        else:
            # Pick the factor pair closest to square (minimise |cols - rows|)
            best_cols = 1
            for c in range(1, int(total ** 0.5) + 1):
                if total % c == 0:
                    best_cols = c          # c is the smaller factor → rows=total//c
            cols = total // best_cols      # use the larger factor as cols

        rows     = total // cols
        tile_w, tile_h = next(iter(tiles.values())).size
        print(f"  Grid: {cols} columns × {rows} rows  |  Tile size: {tile_w}×{tile_h}px")

        # --- Stitch ---
        first_tile = next(iter(tiles.values()))
        canvas = Image.new(first_tile.mode, (cols * tile_w, rows * tile_h))

        for pos, tile in tiles.items():
            row = pos // cols
            col = pos % cols
            canvas.paste(tile, (col * tile_w, row * tile_h))

        output_path = Path(zip_path).with_suffix(".png")
        canvas.save(output_path)
        print(f"  Reconstructed image saved to: {output_path}")

        # --- Decode QR ---
        if qr_decode is None:
            print("\npyzbar is not installed — cannot decode QR code automatically.")
            print("Install it with:  pip install pyzbar")
            print(f"Open {output_path} in any QR reader to get the result.")
            return

        # pyzbar works best on an RGB image
        results = qr_decode(canvas.convert("RGB"))
        if not results:
            print("\nNo QR code detected in the reconstructed image.")
            print("The grid dimensions or tile order may be off, or the image quality is too low.")
            print(f"Inspect the saved image at: {output_path}")
            return

        print("\n=== QR Code Contents ===")
        for r in results:
            data = r.data.decode("utf-8", errors="replace")
            print(f"  Type : {r.type}")
            print(f"  Data : {data}")
        print("========================")


def main():
    zip_path = "output.zip"

    if len(sys.argv) > 1:
        zip_path = sys.argv[1]

    if not os.path.exists(zip_path):
        print(f"Error: '{zip_path}' not found.")
        print("Usage: python main.py [path/to/output.zip]")
        sys.exit(1)

    reconstruct_qr(zip_path)


if __name__ == "__main__":
    main()
