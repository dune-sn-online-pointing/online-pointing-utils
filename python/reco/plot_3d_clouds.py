#!/usr/bin/env python3
"""
plot_3d_clouds.py

Open an .npz produced get_3d_clouds.py and plot the 3D cloud
in the same style as the 3D subplot in plot_matched_clusters.py.

Controls:
- Close the window to advance to the next (event_id, match_id)
- Ctrl+C to quit

NPZ expected fields:
  event_id            (M,)
  match_id            (M,)
  collection_adc      (M,)
  truth_xyz           (M,3)  NaNs if missing
  truth_mom_xyz       (M,3)  NaNs if missing
  cloud_data          (N,4)  (x,y,z,amp) concatenated
  cloud_offsets       (M+1,)
Optional:
  hyp_data            (K,3)  concatenated
  hyp_offsets         (M+1,)
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Optional, Tuple

import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt


def _is_finite3(v: np.ndarray) -> bool:
    return v.shape == (3,) and np.isfinite(v).all()


def _slice_ragged(data: np.ndarray, offsets: np.ndarray, i: int) -> np.ndarray:
    a = int(offsets[i])
    b = int(offsets[i + 1])
    if a < 0 or b < 0 or b < a:
        return data[:0]
    return data[a:b]


def plot_cloud_like_subplot(
    *,
    ev_id: int,
    match_id: int,
    collection_adc: float,
    cloud_xyz_amp: np.ndarray,          # (n,4) float
    truth_xyz: Optional[np.ndarray],    # (3,) or None
    truth_mom_xyz: Optional[np.ndarray],# (3,) or None
    hyp_xyz: Optional[np.ndarray] = None,# (k,3) or None
    true_label: str = "Truth",
    true_mom_label: str = "Truth mom",
    top_charge_frac: float = 0.80,
) -> Tuple[plt.Figure, mpl.axes.Axes]:
    """
    Replicates the style/behavior of the original 3D subplot:
    - bar3d voxels colored by charge with plasma colormap + colorbar
    - optionally overlay all hypotheses faintly
    - top-80%-charge filter
    - truth point and truth momentum arrow
    - axis limits padded by 1 cm
    """

    fig = plt.figure(figsize=(9, 7), constrained_layout=False)
    ax = fig.add_subplot(111, projection="3d")

    ax.set_title(f"3D reco (voxels) | Event {ev_id} match_id={match_id} | ΣADC(X)={collection_adc:.1f}")
    ax.set_xlabel("x [cm]")
    ax.set_ylabel("y [cm]")
    ax.set_zlabel("z [cm]")

    # Optional faint hypotheses scatter
    if hyp_xyz is not None and hyp_xyz.size:
        ax.scatter(hyp_xyz[:, 0], hyp_xyz[:, 1], hyp_xyz[:, 2], s=4, alpha=0.08)

    has_truth = truth_xyz is not None and _is_finite3(truth_xyz)
    has_true_mom = truth_mom_xyz is not None and _is_finite3(truth_mom_xyz)

    if has_truth:
        ax.scatter(
            [float(truth_xyz[0])], [float(truth_xyz[1])], [float(truth_xyz[2])],
            s=80, c="red", marker="o", depthshade=False, label=true_label
        )

    # These match the original code’s voxel sizing choices
    # dx is "1 tick -> cm" in the code, but here we just replicate the look.
    dx = 0.0805  # cm
    dz = 0.30    # cm (rough collection pitch look; change if you want exact)
    dy = 0.78    # cm

    cmap3d = plt.cm.plasma

    if cloud_xyz_amp is None or cloud_xyz_amp.size == 0:
        ax.text2D(0.05, 0.95, "No 3D points", transform=ax.transAxes, va="top")

        if has_truth:
            pad_cm = 5.0
            ax.set_xlim(float(truth_xyz[0] - pad_cm), float(truth_xyz[0] + pad_cm))
            ax.set_ylim(float(truth_xyz[1] - pad_cm), float(truth_xyz[1] + pad_cm))
            ax.set_zlim(float(truth_xyz[2] - pad_cm), float(truth_xyz[2] + pad_cm))

        # Truth momentum arrow (still can draw even without points)
        if has_true_mom:
            _draw_truth_momentum(ax, truth_xyz, truth_mom_xyz, label=true_mom_label)

        if has_truth or has_true_mom:
            ax.legend(loc="upper left")
        ax.view_init(elev=20, azim=-65)
        plt.tight_layout(pad=0.7)
        return fig, ax

    # Split columns
    xs = cloud_xyz_amp[:, 0].astype(float, copy=False)
    ys = cloud_xyz_amp[:, 1].astype(float, copy=False)
    zs = cloud_xyz_amp[:, 2].astype(float, copy=False)
    amps = cloud_xyz_amp[:, 3].astype(float, copy=False)

    # -----------------------------
    # TOP-FRACTION-CHARGE FILTER (same logic as the original)
    # -----------------------------
    amps_rank = np.nan_to_num(amps, nan=0.0, posinf=0.0, neginf=0.0)
    amps_rank = np.maximum(amps_rank, 0.0)
    total_q = float(np.sum(amps_rank))

    if total_q > 0.0 and 0.0 < top_charge_frac < 1.0:
        order = np.argsort(amps_rank)[::-1]
        csum = np.cumsum(amps_rank[order])
        cutoff = top_charge_frac * total_q

        keep_sorted = np.nonzero(csum <= cutoff)[0]
        if keep_sorted.size == 0:
            keep_sorted = np.array([0], dtype=int)
        else:
            last = keep_sorted[-1]
            if last + 1 < order.size:
                keep_sorted = np.append(keep_sorted, last + 1)

        keep_idx = order[keep_sorted]
        xs, ys, zs, amps = xs[keep_idx], ys[keep_idx], zs[keep_idx], amps[keep_idx]
    # -----------------------------

    dy_eff = dy * np.ones_like(xs)

    vmin = float(np.min(amps)) if amps.size else 0.0
    vmax = float(np.max(amps)) if amps.size else 1.0
    if vmax <= vmin:
        vmax = vmin + 1.0

    norm = mpl.colors.Normalize(vmin=vmin, vmax=vmax)
    colors = cmap3d(norm(amps))

    ax.bar3d(
        xs - dx / 2.0,
        ys - dy_eff / 2.0,
        zs - dz / 2.0,
        dx * np.ones_like(xs),
        dy_eff,
        dz * np.ones_like(xs),
        color=colors,
        shade=True,
        edgecolors=(0, 0, 0, 0.05),
        linewidths=0.4,
        alpha=0.15,
    )

    sm = mpl.cm.ScalarMappable(norm=norm, cmap=cmap3d)
    sm.set_array([])
    fig.colorbar(sm, ax=ax, fraction=0.046, pad=0.08, label="Charge (norm)")

    # Axis limits padded by 1 cm (including truth if present)
    pad_cm = 1.0
    if has_truth:
        xs_lim = np.concatenate([xs, [float(truth_xyz[0])]])
        ys_lim = np.concatenate([ys, [float(truth_xyz[1])]])
        zs_lim = np.concatenate([zs, [float(truth_xyz[2])]])
    else:
        xs_lim, ys_lim, zs_lim = xs, ys, zs

    ax.set_xlim(float(np.min(xs_lim) - pad_cm), float(np.max(xs_lim) + pad_cm))
    ax.set_ylim(float(np.min(ys_lim) - pad_cm), float(np.max(ys_lim) + pad_cm))
    ax.set_zlim(float(np.min(zs_lim) - pad_cm), float(np.max(zs_lim) + pad_cm))

    # Truth momentum arrow (same scaling rule as the original)
    if has_true_mom:
        _draw_truth_momentum(ax, truth_xyz if has_truth else None, truth_mom_xyz, label=true_mom_label)

    if has_truth or has_true_mom:
        ax.legend(loc="upper left")

    ax.view_init(elev=20, azim=-65)
    plt.tight_layout(pad=0.7)
    return fig, ax


def _draw_truth_momentum(ax, truth_xyz: Optional[np.ndarray], mom_xyz: np.ndarray, *, label: str) -> None:
    mx, my, mz = float(mom_xyz[0]), float(mom_xyz[1]), float(mom_xyz[2])
    v = np.array([mx, my, mz], dtype=float)
    vnorm = float(np.linalg.norm(v))
    if vnorm <= 0.0:
        return

    x0l, x1l = ax.get_xlim()
    y0l, y1l = ax.get_ylim()
    z0l, z1l = ax.get_zlim()

    span = max(abs(x1l - x0l), abs(y1l - y0l), abs(z1l - z0l))
    desired_len = span / 3.0
    u = (v / vnorm) * desired_len

    if truth_xyz is not None and _is_finite3(truth_xyz):
        ox, oy, oz = float(truth_xyz[0]), float(truth_xyz[1]), float(truth_xyz[2])
    else:
        ox, oy, oz = (x0l + x1l) / 2.0, (y0l + y1l) / 2.0, (z0l + z1l) / 2.0

    ax.quiver(
        ox, oy, oz,
        float(u[0]), float(u[1]), float(u[2]),
        color="red",
        arrow_length_ratio=0.15,
        linewidth=2.0,
        label=label,
    )


def main() -> None:
    ap = argparse.ArgumentParser(description="View 3D clouds from a dumper .npz")
    ap.add_argument("npz", help="Input .npz from get_3d_clouds.py")
    ap.add_argument("--start", type=int, default=0, help="Start index into groups (default 0)")
    ap.add_argument("--only-event", type=int, default=None, help="Only show entries with this event_id")
    ap.add_argument("--only-match", type=int, default=None, help="Only show entries with this match_id (works with --only-event too)")
    ap.add_argument("--top-frac", type=float, default=0.80, help="Keep voxels up to this cumulative charge fraction (default 0.80)")
    ap.add_argument("--no-hypotheses", action="store_true", help="Do not overlay hyp_data even if present")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    p = Path(args.npz)
    if not p.is_file():
        raise SystemExit(f"ERROR: not found: {p}")

    data = np.load(str(p), allow_pickle=False)

    event_id = data["event_id"].astype(np.int64, copy=False)
    match_id = data["match_id"].astype(np.int64, copy=False)
    collection_adc = data["collection_adc"].astype(np.float64, copy=False)
    truth_xyz = data["truth_xyz"].astype(np.float64, copy=False)
    truth_mom_xyz = data["truth_mom_xyz"].astype(np.float64, copy=False)

    cloud_data = data["cloud_data"].astype(np.float32, copy=False)
    cloud_offsets = data["cloud_offsets"].astype(np.int64, copy=False)

    has_hyp = ("hyp_data" in data.files) and ("hyp_offsets" in data.files) and (not args.no_hypotheses)
    hyp_data = data["hyp_data"].astype(np.float32, copy=False) if has_hyp else None
    hyp_offsets = data["hyp_offsets"].astype(np.int64, copy=False) if has_hyp else None

    M = event_id.shape[0]
    if cloud_offsets.shape[0] != M + 1:
        raise SystemExit("ERROR: cloud_offsets length mismatch vs event_id")

    # Build index list with filters
    idxs = np.arange(M, dtype=int)

    if args.only_event is not None:
        idxs = idxs[event_id[idxs] == int(args.only_event)]
    if args.only_match is not None:
        idxs = idxs[match_id[idxs] == int(args.only_match)]

    if idxs.size == 0:
        raise SystemExit("ERROR: no entries matched your filters")

    start_pos = max(0, int(args.start))
    if start_pos >= idxs.size:
        raise SystemExit(f"ERROR: --start {start_pos} out of range (0..{idxs.size-1})")

    plt.ion()

    for j in range(start_pos, idxs.size):
        i = int(idxs[j])
        ev = int(event_id[i])
        mid = int(match_id[i])
        cadc = float(collection_adc[i])

        cloud_i = _slice_ragged(cloud_data, cloud_offsets, i)

        hyp_i = None
        if has_hyp and hyp_data is not None and hyp_offsets is not None:
            hyp_i = _slice_ragged(hyp_data, hyp_offsets, i)

        txyz = truth_xyz[i]
        tmom = truth_mom_xyz[i]

        if args.verbose:
            npts = int(cloud_i.shape[0])
            nhyp = int(hyp_i.shape[0]) if hyp_i is not None else 0
            print(f"[{j+1}/{idxs.size}] idx={i} event={ev} match={mid} ΣADC(X)={cadc:.1f} cloud_pts={npts} hyp={nhyp}")

        fig, _ = plot_cloud_like_subplot(
            ev_id=ev,
            match_id=mid,
            collection_adc=cadc,
            cloud_xyz_amp=cloud_i,
            truth_xyz=txyz,
            truth_mom_xyz=tmom,
            hyp_xyz=hyp_i,
            top_charge_frac=float(args.top_frac),
        )

        plt.show(block=True)
        plt.close(fig)


if __name__ == "__main__":
    main()
