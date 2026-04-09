"""
ArcGIS World Imagery Tile Downloader for Offline CesiumJS

Downloads satellite imagery tiles from ArcGIS REST services
and saves them in {z}/{x}/{y}.jpg format for UrlTemplateImageryProvider.

Usage:
  python download_tiles.py --preset global_low
  python download_tiles.py --preset turkey_medium
  python download_tiles.py --bbox 25.5 35.5 45.0 42.5 --zoom 7 12
  python download_tiles.py --preset global_low --preset turkey_medium  (chain multiple)
"""

import argparse
import json
import math
import os
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import requests
from tqdm import tqdm

# ArcGIS World Imagery tile server
# URL format: tile/{z}/{y}/{x}  (note: ArcGIS uses z/y/x, we save as z/x/y)
TILE_URL = "https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}"

# Default output directory (relative to this script's parent = project root)
DEFAULT_OUTPUT = os.path.join(os.path.dirname(__file__), "..", "cesium_web", "public", "tiles")

# Rate limiting
MAX_WORKERS = 4
REQUEST_DELAY = 0.05  # 50ms between requests per thread (~80 req/s total)
REQUEST_TIMEOUT = 30
MAX_RETRIES = 3

# Load presets from config file
def load_presets():
    config_path = os.path.join(os.path.dirname(__file__), "download_config.json")
    if os.path.exists(config_path):
        with open(config_path, "r") as f:
            return json.load(f)
    return {}


def lon_to_tile_x(lon, zoom):
    """Convert longitude to tile X coordinate."""
    return int((lon + 180.0) / 360.0 * (1 << zoom))


def lat_to_tile_y(lat, zoom):
    """Convert latitude to tile Y coordinate (Web Mercator)."""
    lat_rad = math.radians(lat)
    n = 1 << zoom
    return int((1.0 - math.log(math.tan(lat_rad) + 1.0 / math.cos(lat_rad)) / math.pi) / 2.0 * n)


def get_tile_ranges(bbox, zoom):
    """Get tile X/Y ranges for a bounding box at a given zoom level."""
    min_lon, min_lat, max_lon, max_lat = bbox
    x_min = lon_to_tile_x(min_lon, zoom)
    x_max = lon_to_tile_x(max_lon, zoom)
    y_min = lat_to_tile_y(max_lat, zoom)  # note: y is inverted
    y_max = lat_to_tile_y(min_lat, zoom)
    return x_min, x_max, y_min, y_max


def count_tiles(bbox, zoom_min, zoom_max):
    """Count total tiles to download."""
    total = 0
    for z in range(zoom_min, zoom_max + 1):
        x_min, x_max, y_min, y_max = get_tile_ranges(bbox, z)
        total += (x_max - x_min + 1) * (y_max - y_min + 1)
    return total


def download_tile(z, x, y, output_dir, session):
    """Download a single tile. Returns (success, path)."""
    # Save as z/x/y.jpg (CesiumJS UrlTemplateImageryProvider convention)
    tile_path = os.path.join(output_dir, str(z), str(x), f"{y}.jpg")

    # Skip if already exists (resume support)
    if os.path.exists(tile_path):
        return True, tile_path, True  # skipped

    url = TILE_URL.format(z=z, y=y, x=x)

    for attempt in range(MAX_RETRIES):
        try:
            resp = session.get(url, timeout=REQUEST_TIMEOUT)
            if resp.status_code == 200:
                os.makedirs(os.path.dirname(tile_path), exist_ok=True)
                with open(tile_path, "wb") as f:
                    f.write(resp.content)
                time.sleep(REQUEST_DELAY)
                return True, tile_path, False
            elif resp.status_code == 404:
                return False, tile_path, False  # tile doesn't exist
            else:
                time.sleep(1)  # back off on errors
        except requests.exceptions.RequestException:
            time.sleep(2 ** attempt)

    return False, tile_path, False


def download_region(bbox, zoom_min, zoom_max, output_dir, label="", workers=MAX_WORKERS):
    """Download all tiles for a region."""
    total = count_tiles(bbox, zoom_min, zoom_max)
    print(f"\n{'=' * 60}")
    print(f"Region: {label or 'custom'}")
    print(f"Bbox: {bbox}")
    print(f"Zoom: {zoom_min}-{zoom_max}")
    print(f"Total tiles: {total:,}")
    print(f"Output: {os.path.abspath(output_dir)}")
    print(f"{'=' * 60}")

    session = requests.Session()
    session.headers.update({
        "User-Agent": "CesiumTacview-TileDownloader/1.0",
        "Referer": "https://cesium-tacview.local",
    })

    downloaded = 0
    skipped = 0
    failed = 0

    with tqdm(total=total, desc=f"[{label}]", unit="tile") as pbar:
        with ThreadPoolExecutor(max_workers=workers) as executor:
            futures = []
            for z in range(zoom_min, zoom_max + 1):
                x_min, x_max, y_min, y_max = get_tile_ranges(bbox, z)
                for x in range(x_min, x_max + 1):
                    for y in range(y_min, y_max + 1):
                        futures.append(
                            executor.submit(download_tile, z, x, y, output_dir, session)
                        )

            for future in as_completed(futures):
                success, path, was_skipped = future.result()
                if was_skipped:
                    skipped += 1
                elif success:
                    downloaded += 1
                else:
                    failed += 1
                pbar.update(1)

    print(f"\nResults: {downloaded} downloaded, {skipped} skipped (existing), {failed} failed")
    return downloaded, skipped, failed


def main():
    parser = argparse.ArgumentParser(description="Download ArcGIS World Imagery tiles for offline CesiumJS")
    parser.add_argument("--preset", action="append", help="Preset name(s) from download_config.json")
    parser.add_argument("--bbox", type=float, nargs=4, metavar=("MIN_LON", "MIN_LAT", "MAX_LON", "MAX_LAT"),
                        help="Bounding box: min_lon min_lat max_lon max_lat")
    parser.add_argument("--zoom", type=int, nargs=2, metavar=("MIN", "MAX"),
                        help="Zoom range: min max")
    parser.add_argument("--output", type=str, default=DEFAULT_OUTPUT,
                        help="Output directory (default: cesium_web/public/tiles)")
    parser.add_argument("--workers", type=int, default=MAX_WORKERS,
                        help=f"Number of download threads (default: {MAX_WORKERS})")
    parser.add_argument("--list-presets", action="store_true",
                        help="List available presets and exit")

    args = parser.parse_args()

    presets = load_presets()

    if args.list_presets:
        if not presets:
            print("No presets found. Create scripts/download_config.json")
            return
        print("Available presets:")
        for name, cfg in presets.items():
            tile_count = count_tiles(cfg["bbox"], cfg["zoom_min"], cfg["zoom_max"])
            est_mb = tile_count * 25 / 1024  # ~25KB average per tile
            print(f"  {name}: zoom {cfg['zoom_min']}-{cfg['zoom_max']}, ~{tile_count:,} tiles (~{est_mb:.0f} MB)")
            if "description" in cfg:
                print(f"    {cfg['description']}")
        return

    workers = args.workers

    output_dir = os.path.abspath(args.output)
    os.makedirs(output_dir, exist_ok=True)

    total_downloaded = 0
    total_skipped = 0
    total_failed = 0

    # Download presets
    if args.preset:
        for preset_name in args.preset:
            if preset_name not in presets:
                print(f"Error: Unknown preset '{preset_name}'")
                print(f"Available: {', '.join(presets.keys())}")
                sys.exit(1)
            cfg = presets[preset_name]
            d, s, f = download_region(
                cfg["bbox"], cfg["zoom_min"], cfg["zoom_max"],
                output_dir, label=preset_name, workers=workers
            )
            total_downloaded += d
            total_skipped += s
            total_failed += f

    # Download custom bbox
    if args.bbox and args.zoom:
        d, s, f = download_region(
            args.bbox, args.zoom[0], args.zoom[1],
            output_dir, label="custom", workers=workers
        )
        total_downloaded += d
        total_skipped += s
        total_failed += f

    if not args.preset and not (args.bbox and args.zoom):
        parser.print_help()
        sys.exit(1)

    print(f"\n{'=' * 60}")
    print(f"TOTAL: {total_downloaded} downloaded, {total_skipped} skipped, {total_failed} failed")
    print(f"Tiles saved to: {output_dir}")
    print(f"{'=' * 60}")


if __name__ == "__main__":
    main()
