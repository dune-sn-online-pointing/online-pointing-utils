#!/usr/bin/env python3
"""Re-run match_clusters at various time tolerances and compute truth residuals."""
from __future__ import annotations

import json
import math
import subprocess
from pathlib import Path
from shutil import rmtree
from typing import Dict, List, Tuple

import uproot

REPO_ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = REPO_ROOT / "build" / "src" / "app"
MATCH_CLUSTERS = BUILD_DIR / "match_clusters"
BASE_CONFIG = REPO_ROOT / "json" / "cc_valid.json"
TEMP_CONFIG = REPO_ROOT / "tmp" / "cc_valid_temp.json"
RUN_LOG_TEMPLATE = "run_t{tol}.log"

# Time tolerances to scan (ticks)
TIME_TOLERANCES = [500, 300, 100, 50, 40, 30, 20, 10, 5, 0]


def sanitize(value: str) -> str:
    """Mirror the repo's folder naming logic for clustering conditions."""
    if "." in value:
        dot = value.index(".")
        value = value[: min(len(value), dot + 2)]
    return value.replace(".", "p")


def resolved_matched_folder(config: Dict[str, object]) -> Path:
    base_folder = Path(config["tpstream_folder"])
    prefix = config.get("clusters_folder_prefix", "")
    tick_limit = sanitize(str(config.get("tick_limit", 0)))
    channel_limit = sanitize(str(config.get("channel_limit", 0)))
    min_tps = sanitize(str(config.get("min_tps_to_cluster", 0)))
    tot_cut = sanitize(str(config.get("tot_cut", 0)))
    energy_cut = sanitize(str(config.get("energy_cut", 0.0)))
    conditions = f"tick{tick_limit}_ch{channel_limit}_min{min_tps}_tot{tot_cut}_e{energy_cut}"
    return base_folder / f"matched_clusters_{prefix}_{conditions}"


def run_match_clusters(config_path: Path, out_folder: Path, tolerance: int) -> None:
    if out_folder.exists():
        rmtree(out_folder)
    out_folder.mkdir(parents=True, exist_ok=True)

    log_path = REPO_ROOT / RUN_LOG_TEMPLATE.format(tol=tolerance)
    with log_path.open("w", encoding="ascii", errors="replace") as log_file:
        subprocess.run(
            [
                str(MATCH_CLUSTERS),
                "-j",
                str(config_path),
                "-m",
                "5",
                "-f",
                "-v",
                "--outFolder",
                str(out_folder),
            ],
            check=True,
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )


def collect_distances(matched_dir: Path) -> Tuple[List[float], List[float]]:
    u_distances: List[float] = []
    v_distances: List[float] = []

    for root_file in sorted(matched_dir.glob("*_matched.root")):
        with uproot.open(root_file) as f:
            tree_x = f["clusters"]["clusters_tree_X"]
            tree_u = f["clusters"]["clusters_tree_U"]
            tree_v = f["clusters"]["clusters_tree_V"]

            arr_x = tree_x.arrays(
                [
                    "cluster_id",
                    "matching_clusterId_U",
                    "matching_clusterId_V",
                    "true_pos_x",
                    "true_pos_y",
                    "true_pos_z",
                    "is_main_cluster",
                ],
                library="np",
            )
            arr_u = tree_u.arrays(["cluster_id", "true_pos_x", "true_pos_y", "true_pos_z"], library="np")
            arr_v = tree_v.arrays(["cluster_id", "true_pos_x", "true_pos_y", "true_pos_z"], library="np")

            u_positions: Dict[int, Tuple[float, float, float]] = {
                int(cid): (float(x), float(y), float(z))
                for cid, x, y, z in zip(
                    arr_u["cluster_id"],
                    arr_u["true_pos_x"],
                    arr_u["true_pos_y"],
                    arr_u["true_pos_z"],
                )
            }
            v_positions: Dict[int, Tuple[float, float, float]] = {
                int(cid): (float(x), float(y), float(z))
                for cid, x, y, z in zip(
                    arr_v["cluster_id"],
                    arr_v["true_pos_x"],
                    arr_v["true_pos_y"],
                    arr_v["true_pos_z"],
                )
            }

            for idx in range(len(arr_x["cluster_id"])):
                if not bool(arr_x["is_main_cluster"][idx]):
                    continue
                tx = float(arr_x["true_pos_x"][idx])
                ty = float(arr_x["true_pos_y"][idx])
                tz = float(arr_x["true_pos_z"][idx])
                match_u = int(arr_x["matching_clusterId_U"][idx])
                match_v = int(arr_x["matching_clusterId_V"][idx])

                if match_u >= 0 and match_u in u_positions:
                    ux, uy, uz = u_positions[match_u]
                    u_distances.append(math.sqrt((ux - tx) ** 2 + (uy - ty) ** 2 + (uz - tz) ** 2))
                if match_v >= 0 and match_v in v_positions:
                    vx, vy, vz = v_positions[match_v]
                    v_distances.append(math.sqrt((vx - tx) ** 2 + (vy - ty) ** 2 + (vz - tz) ** 2))

    return u_distances, v_distances


def main() -> None:
    base_config = json.loads(BASE_CONFIG.read_text(encoding="utf-8"))

    results = []
    for tolerance in TIME_TOLERANCES:
        config = dict(base_config)
        config["time_tolerance_ticks"] = tolerance
        TEMP_CONFIG.write_text(json.dumps(config, indent=2), encoding="utf-8")

        out_folder = REPO_ROOT / "tmp" / f"matched_truth_t{tolerance}"
        run_match_clusters(TEMP_CONFIG, out_folder, tolerance)
        u_distances, v_distances = collect_distances(out_folder)

        avg_u = sum(u_distances) / len(u_distances) if u_distances else float("nan")
        avg_v = sum(v_distances) / len(v_distances) if v_distances else float("nan")
        results.append(
            {
                "tolerance": tolerance,
                "avg_u_cm": avg_u,
                "avg_v_cm": avg_v,
                "count_u": len(u_distances),
                "count_v": len(v_distances),
            }
        )

        rmtree(out_folder)

    TEMP_CONFIG.unlink(missing_ok=True)

    for item in results:
        tol = item["tolerance"]
        avg_u = item["avg_u_cm"]
        avg_v = item["avg_v_cm"]
        print(f"{tol:4d} ticks -> avg U: {avg_u:.3f} cm ({item['count_u']} matches), avg V: {avg_v:.3f} cm ({item['count_v']} matches)")


if __name__ == "__main__":
    main()
