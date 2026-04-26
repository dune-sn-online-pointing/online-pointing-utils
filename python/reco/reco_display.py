#!/usr/bin/env python3
"""
plot_matched_clusters.py

Matched cluster waveform viewer (U/V/X) using histogram filling + imshow.

Key behavior:
- X plane is at the bottom (order U, V, X)
- DOES NOT modify TP time values
- Shifts only the DISPLAY y-window per plane (WINDOW_SHIFT_TICKS) to align drift
- Anchors ONLY the y-range (base) to the X-plane content
- DOES NOT match x-axes between planes (each plane keeps its own x-range)
- Sets automatic x ticks at the very end (after limits are final)
- Uses REAL channel numbers on the x-axis

Close window to advance. Ctrl+C to quit.
"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple, Any

import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.ticker import AutoLocator
from matplotlib.gridspec import GridSpec
from mpl_toolkits.mplot3d import Axes3D
from scipy.optimize import minimize_scalar

from space_transformations import (
    xyz_to_UVX_wires_lut,
    DRIFT_VELOCITY,
    X_WIRE_PITCH,
    U_WIRE_PITCH,
    V_WIRE_PITCH,
)

# Try custom style module; fall back gracefully
try:
    import style  # type: ignore
    _PARULA = getattr(style, "parula_map", None)
except Exception:
    _PARULA = None

from load_matched_clusters import MatchedClustersLoader, ClusterItem, MatchedGroup
import reco_3d

from reco_3d import (
    U_y_sorted,
    U_u_sorted,
    V_y_sorted,
    V_v_sorted
)

# -----------------------------
# Drift alignment (WINDOW SHIFT ONLY)
# -----------------------------
DRIFT_VELOCITY_CM_PER_TICK = 0.0805
PLANE_GAP_CM = 0.47  # cm
GAP_TICKS = int(round(PLANE_GAP_CM / DRIFT_VELOCITY_CM_PER_TICK))  # ~6 ticks

# relative to X: show an earlier time window for earlier planes
WINDOW_SHIFT_TICKS = {"U": -2 * GAP_TICKS, "V": -1 * GAP_TICKS, "X": 0}

# -----------------------------
# Drawing algorithms
# -----------------------------

class DrawingAlgorithms:
    @staticmethod
    def polygon_area(vertices: List[Tuple[float, float]]) -> float:
        n = len(vertices)
        if n < 3:
            return 0.0
        sum1, sum2 = 0.0, 0.0
        for i in range(n):
            j = (i + 1) % n
            sum1 += vertices[i][0] * vertices[j][1]
            sum2 += vertices[j][0] * vertices[i][1]
        return 0.5 * abs(sum1 - sum2)

    @staticmethod
    def calculate_pentagon_params(
        time_start: float,
        time_peak: float,
        time_end: float,
        adc_peak: float,
        adc_integral: float,
        threshold: float,
    ) -> Optional[Dict[str, float]]:
        time_over_threshold = time_end - time_start
        if time_over_threshold <= 0:
            return None

        t1 = round(time_start + 0.5 * (time_peak - time_start))
        t2 = round(time_peak + 0.5 * (time_end - time_peak))

        offset_area = threshold * time_over_threshold
        max_h = adc_peak - threshold
        if max_h <= 0:
            return None

        def objective(h: float) -> float:
            vertices = [
                (time_start, threshold),
                (t1, threshold + h),
                (time_peak, adc_peak),
                (t2, threshold + h),
                (time_end, threshold),
            ]
            pent_area = DrawingAlgorithms.polygon_area(vertices)
            return abs((pent_area + offset_area) - adc_integral)

        result = minimize_scalar(objective, bounds=(0, max_h), method="bounded")
        h_opt = float(result.x)
        y_intermediate = threshold + h_opt

        return {
            "time_int_rise": float(t1),
            "time_int_fall": float(t2),
            "h_int_rise": float(y_intermediate),
            "h_int_fall": float(y_intermediate),
            "threshold": float(threshold),
        }

    @staticmethod
    def fill_histogram_pentagon(
        hist: np.ndarray,
        ch_idx: int,
        time_start: int,
        time_peak: int,
        samples_over_threshold: int,
        adc_peak: float,
        adc_integral: float,
        threshold_adc: float,
        tmin: int,
        ch_map: Dict[int, int],
    ):
        if samples_over_threshold <= 0:
            return

        time_end = time_start + samples_over_threshold
        time_peak = max(time_start, min(time_end, time_peak))

        params = DrawingAlgorithms.calculate_pentagon_params(
            time_start, time_peak, time_end, adc_peak, adc_integral, threshold_adc
        )
        if not params:
            DrawingAlgorithms.fill_histogram_triangle(
                hist, ch_idx, time_start, samples_over_threshold,
                time_peak - time_start, adc_peak, threshold_adc, tmin, ch_map
            )
            return

        t_int_rise = int(round(params["time_int_rise"]))
        t_int_fall = int(round(params["time_int_fall"]))
        threshold = float(params["threshold"])

        extended_start = time_start - 1
        extended_end = time_end + 1

        for t in range(extended_start, extended_end):
            intensity = 0.0

            if t < time_start:
                span = t_int_rise - time_start
                if span > 0:
                    frac = (t - time_start) / span
                    intensity = threshold + frac * (params["h_int_rise"] - threshold)
                    if intensity < threshold_adc * 0.5:
                        intensity = 0.0
            elif t < t_int_rise:
                span = t_int_rise - time_start
                if span > 0:
                    frac = (t - time_start) / span
                    intensity = threshold + frac * (params["h_int_rise"] - threshold)
            elif t < time_peak:
                span = time_peak - t_int_rise
                if span > 0:
                    frac = (t - t_int_rise) / span
                    intensity = params["h_int_rise"] + frac * (adc_peak - params["h_int_rise"])
                else:
                    intensity = adc_peak
            elif t == time_peak:
                intensity = adc_peak
            elif t <= t_int_fall:
                span = t_int_fall - time_peak
                if span > 0:
                    frac = (t - time_peak) / span
                    intensity = adc_peak - frac * (adc_peak - params["h_int_fall"])
                else:
                    intensity = params["h_int_fall"]
            elif t < time_end:
                span = time_end - t_int_fall
                if span > 0:
                    frac = (t - t_int_fall) / span
                    intensity = params["h_int_fall"] - frac * (params["h_int_fall"] - threshold)
            else:
                span = time_end - t_int_fall
                if span > 0:
                    frac = (t - t_int_fall) / span
                    intensity = params["h_int_fall"] - frac * (params["h_int_fall"] - threshold)
                    if intensity < threshold_adc * 0.5:
                        intensity = 0.0

            if intensity > 0:
                t_idx = t - tmin
                if 0 <= t_idx < hist.shape[0] and 0 <= ch_idx < hist.shape[1]:
                    hist[t_idx, ch_idx] = max(hist[t_idx, ch_idx], intensity)

    @staticmethod
    def fill_histogram_triangle(
        hist: np.ndarray,
        ch_idx: int,
        time_start: int,
        samples_over_threshold: int,
        samples_to_peak: int,
        adc_peak: float,
        threshold_adc: float,
        tmin: int,
        ch_map: Dict[int, int],
    ):
        if samples_over_threshold <= 0:
            return

        time_end = time_start + max(1, samples_over_threshold)
        peak_time = time_start + samples_to_peak

        for t in range(time_start, time_end):
            if t <= peak_time:
                if peak_time != time_start:
                    frac = (t - time_start) / (peak_time - time_start)
                    intensity = threshold_adc + frac * (adc_peak - threshold_adc)
                else:
                    intensity = adc_peak
            else:
                fall_span = (time_end - 1) - peak_time
                if fall_span > 0:
                    frac = (t - peak_time) / fall_span
                    intensity = adc_peak - frac * (adc_peak - threshold_adc)
                else:
                    intensity = adc_peak

            t_idx = t - tmin
            if 0 <= t_idx < hist.shape[0] and 0 <= ch_idx < hist.shape[1]:
                hist[t_idx, ch_idx] = max(hist[t_idx, ch_idx], intensity)

    @staticmethod
    def fill_histogram_rectangle(
        hist: np.ndarray,
        ch_idx: int,
        time_start: int,
        samples_over_threshold: int,
        adc_integral: float,
        tmin: int,
        ch_map: Dict[int, int],
    ):
        if samples_over_threshold <= 0:
            return
        uniform_intensity = float(adc_integral) / float(samples_over_threshold)
        time_end = time_start + samples_over_threshold

        for t in range(time_start, time_end):
            t_idx = t - tmin
            if 0 <= t_idx < hist.shape[0] and 0 <= ch_idx < hist.shape[1]:
                hist[t_idx, ch_idx] = max(hist[t_idx, ch_idx], uniform_intensity)


# -----------------------------
# Views container
# -----------------------------

@dataclass
class MatchedEventViews:
    ev_id: int
    match_id: int
    views: List[Dict[str, Any]]


def extract_matched_event_views_object(
    group: MatchedGroup,
    *,
    order: Tuple[str, str, str] = ("U", "V", "X"),
) -> MatchedEventViews:
    return MatchedEventViews(
        ev_id=group.event_id,
        match_id=group.match_id,
        views=[{"name": p, "cluster": group.get(p)} for p in order],
    )


# -----------------------------
# Plotting helpers
# -----------------------------

def _safe_tp_count(c: Optional[ClusterItem]) -> int:
    if c is None:
        return 0
    return min(len(c.ch), len(c.tstart), len(c.sot))


def _waveform_hist_for_cluster(
    cluster: ClusterItem,
    *,
    draw_mode: str,
    threshold_adc: float,
    pad_bins: int,
) -> Tuple[np.ndarray, Tuple[float, float, float, float], int, int, int, int]:
    """
    Build waveform histogram on an x-axis of REAL channel numbers.

    Returns:
        hist_masked
        extent = (xmin, xmax, ymin, ymax) for imshow
        tmin, tmax (ticks)
        ch_min, ch_max (channels)
    """
    n = _safe_tp_count(cluster)
    if n <= 0:
        hist = np.zeros((1, 1), dtype=float)
        return np.ma.array(hist), (0, 1, 0, 1), 0, 0, 0, 0

    tstarts = [int(cluster.tstart[i]) for i in range(n)]
    tmin = int(min(tstarts))
    tmax = int(max(int(cluster.tstart[i]) + int(cluster.sot[i]) for i in range(n)))

    channels = [int(cluster.ch[i]) for i in range(n)]
    ch_min = int(min(channels))
    ch_max = int(max(channels))

    n_ch = (ch_max - ch_min + 1)
    n_t = int(tmax - tmin + 1)
    hist = np.zeros((n_t + 2 * pad_bins, n_ch + 2 * pad_bins), dtype=float)

    def ch_to_col(ch: int) -> int:
        return (ch - ch_min) + pad_bins

    dm = draw_mode.lower()
    for i in range(n):
        ts = int(cluster.tstart[i])
        tot = int(cluster.sot[i])
        if tot <= 0:
            continue

        col = ch_to_col(int(cluster.ch[i]))
        samples_to_peak = int(cluster.stopeak[i]) if i < len(cluster.stopeak) else (tot // 2)
        peak_time = ts + samples_to_peak

        peak_adc = float(cluster.adc_peak[i]) if i < len(cluster.adc_peak) else 200.0
        adc_integral = (
            float(cluster.adc_integral[i])
            if i < len(cluster.adc_integral)
            else (peak_adc * tot / 2.0)
        )

        if dm == "pentagon":
            DrawingAlgorithms.fill_histogram_pentagon(
                hist, col, ts, peak_time, tot, peak_adc, adc_integral,
                threshold_adc, int(tmin), {}
            )
        elif dm == "triangle":
            DrawingAlgorithms.fill_histogram_triangle(
                hist, col, ts, tot, samples_to_peak, peak_adc,
                threshold_adc, int(tmin), {}
            )
        elif dm == "rectangle":
            DrawingAlgorithms.fill_histogram_rectangle(
                hist, col, ts, tot, adc_integral, int(tmin), {}
            )
        else:
            raise ValueError(f"Unknown draw_mode={draw_mode!r}")

    hist_masked = np.ma.masked_where(hist < threshold_adc, hist)

    xmin = (ch_min - pad_bins) - 0.5
    xmax = (ch_max + pad_bins) + 0.5
    ymin = (tmin - pad_bins) - 0.5
    ymax = (tmax + pad_bins) + 0.5
    extent = (xmin, xmax, ymin, ymax)

    return hist_masked, extent, tmin, tmax, ch_min, ch_max


# -----------------------------
# Main plotting
# -----------------------------

def plot_matched_event_views(
    views_obj: Optional[MatchedEventViews],
    *,
    draw_mode: str = "pentagon",
    thr_u: float = 70.0,
    thr_v: float = 70.0,
    thr_x: float = 60.0,
    expand: float = 1.5,
    figsize: Tuple[float, float] = (15, 10),
    pad_bins: int = 2,
    xyz_selected: Optional[List[Tuple[float, float, float, float]]] = None,  # (x,y,z,amp)
    xyz_all: Optional[List[Tuple[float, float, float]]] = None,              # (x,y,z)
    true_x: Optional[float] = None,
    true_y: Optional[float] = None,
    true_z: Optional[float] = None,
    true_U_id: Optional[float] = None,
    true_V_id: Optional[float] = None,
    true_X_id: Optional[float] = None,
    true_mom_x: Optional[float] = None,
    true_mom_y: Optional[float] = None,
    true_mom_z: Optional[float] = None,
    true_label: str = "Truth",
    true_mom_label: str = "Truth mom",
):
    """
    Plot U/V/X waveform views plus a 3D reconstruction panel.

    Behavior (2D panels)
    --------------------
    - Time window anchored to X view only (then shifted by WINDOW_SHIFT_TICKS per plane).
    - Each plane keeps its own wire/channel x-range, expanded about its own extent.
    - If true_U_id / true_V_id / true_X_id is provided, draws a red vertical line at that x value
      on the corresponding 2D plane panel.

    Behavior (3D panel)
    -------------------
    - Renders selected 3D points as voxels via bar3d.
    - Colors voxels by charge (amplitude) using a colormap + colorbar.
    - Optionally overlays all hypothesis points faintly.
    - Sets 3D axis limits to the selected points' bounding box padded by 1 cm in x/y/z.
    - If (true_x, true_y, true_z) are provided, plots truth as a red dot.
    - Keeps only the set of voxels that cumulatively account for the top 80% of deposited charge.

    """
    if views_obj is None or getattr(views_obj, "views", None) is None:
        return None, None

    ev_id = int(getattr(views_obj, "ev_id", -1))
    match_id = int(getattr(views_obj, "match_id", -1))
    views = views_obj.views

    # -----------------------------
    # Figure layout: 3 rows x 2 cols, bigger 3D column
    # -----------------------------
    fig = plt.figure(figsize=figsize, constrained_layout=False)
    gs = GridSpec(
        nrows=3,
        ncols=2,
        figure=fig,
        width_ratios=[2.6, 2.2],
        wspace=0.25,
        hspace=0.35,
    )

    axes = np.array([fig.add_subplot(gs[i, 0]) for i in range(3)])
    ax3d = fig.add_subplot(gs[:, 1], projection="3d")

    fig.subplots_adjust(left=0.10, right=0.95)
    fig.suptitle(f"Event {ev_id} | match_id={match_id} | mode={draw_mode}", fontsize=18)

    thr_by_plane = {"U": float(thr_u), "V": float(thr_v), "X": float(thr_x)}
    true_id_by_plane = {"U": true_U_id, "V": true_V_id, "X": true_X_id}

    # -----------------------------
    # Precompute hists
    # -----------------------------
    pre: List[Dict[str, Any]] = []
    for view in views:
        plane = str(view.get("name", "")).strip()
        cluster: Optional[ClusterItem] = view.get("cluster", None)
        thr = thr_by_plane.get(plane, float(thr_x))

        if cluster is None or _safe_tp_count(cluster) <= 0:
            pre.append({"plane": plane, "ok": False})
            continue

        hist_masked, extent, tmin, tmax, ch_min, ch_max = _waveform_hist_for_cluster(
            cluster,
            draw_mode=draw_mode,
            threshold_adc=thr,
            pad_bins=pad_bins,
        )
        pre.append(
            {
                "plane": plane,
                "ok": True,
                "hist": hist_masked,
                "extent": extent,
                "tmin": int(tmin),
                "tmax": int(tmax),
            }
        )

    # -----------------------------
    # Anchor ONLY the y-window (base) to X
    # -----------------------------
    x_idx = next((k for k, pc in enumerate(pre) if pc.get("plane") == "X" and pc.get("ok")), None)
    if x_idx is None:
        x_idx = next((k for k, pc in enumerate(pre) if pc.get("ok")), None)
    if x_idx is None:
        for ax in axes:
            ax.set_title("(no TPs)")
        ax3d.set_title("3D reco (no TPs)")
        return fig, axes

    tmin_x = int(pre[x_idx]["tmin"])
    tmax_x = int(pre[x_idx]["tmax"])

    y0t = (tmin_x - pad_bins) - 0.5
    y1t = (tmax_x + pad_bins) + 0.5
    ymid = (y0t + y1t) / 2.0
    yhalf = (y1t - y0t) / 2.0
    ylim_target_x = (ymid - expand * yhalf, ymid + expand * yhalf)

    # -----------------------------
    # Draw per plane (2D)
    # -----------------------------
    for ax, view, pc in zip(axes, views, pre):
        plane = str(view.get("name", "")).strip()
        ax.set_ylabel("Time tick")

        dy_ticks = int(WINDOW_SHIFT_TICKS.get(plane, 0))
        ax.set_ylim(ylim_target_x[0] + dy_ticks, ylim_target_x[1] + dy_ticks)

        ax.set_box_aspect(2 / 3)

        if not pc.get("ok", False):
            ax.set_title(f"View {plane} | (no TPs)")
            ax.set_axisbelow(False)
            continue

        xmin, xmax, _, _ = pc["extent"]
        xmid = (xmin + xmax) / 2.0
        xhalf = (xmax - xmin) / 2.0
        ax.set_xlim(xmid - expand * xhalf, xmid + expand * xhalf)

        cmap2d = _PARULA if _PARULA is not None else plt.cm.viridis
        if plane == "X":
            cmap2d = plt.cm.plasma

        im = ax.imshow(
            pc["hist"],
            aspect="auto",
            origin="lower",
            extent=pc["extent"],
            cmap=cmap2d,
            interpolation="nearest",
        )

        # red vertical truth line (x-axis = wire/channel id)
        true_id = true_id_by_plane.get(plane, None)
        if true_id is not None:
            ax.axvline(
                float(true_id),
                color="red",
                linewidth=2.0,
                alpha=0.95,
                zorder=50,
            )

        adc_sum = float(np.ma.filled(pc["hist"], 0.0).sum())
        ax.set_title(f"View {plane} | $\\sum ADC = {adc_sum:.1f}$")
        fig.colorbar(im, ax=ax, label="ADC")
        ax.set_axisbelow(False)

    for ax in axes:
        ax.xaxis.set_major_locator(AutoLocator())
        ax.tick_params(axis="x", labelrotation=0)

    for ax in axes:
        xmin, xmax = ax.get_xlim()
        ymin, ymax = ax.get_ylim()

        ax.yaxis.set_major_locator(AutoLocator())

        x0g = np.ceil(xmin - 0.5) + 0.5
        y0g = np.ceil(ymin - 0.5) + 0.5
        x1g = np.floor(xmax - 0.5) + 0.5
        y1g = np.floor(ymax - 0.5) + 0.5

        xi = np.arange(x0g, x1g + 1e-9, 1.0)
        yi = np.arange(y0g, y1g + 1e-9, 1.0)

        ax.set_xticks(xi, minor=True)
        ax.set_yticks(yi, minor=True)

        ax.grid(True, which="minor", linestyle="-", alpha=0.30, zorder=10)
        ax.tick_params(which="minor", length=0)

    axes[-1].set_xlabel("Wire/Channel (local APA)")

    # -----------------------------
    # 3D voxel panel
    # -----------------------------
    ax3d.set_title("3D reco (voxels)")
    ax3d.set_xlabel("x [cm]")
    ax3d.set_ylabel("y [cm]")
    ax3d.set_zlabel("z [cm]")

    if xyz_all:
        xs_all = [p[0] for p in xyz_all]
        ys_all = [p[1] for p in xyz_all]
        zs_all = [p[2] for p in xyz_all]
        ax3d.scatter(xs_all, ys_all, zs_all, s=4, alpha=0.08)

    has_truth = (true_x is not None) and (true_y is not None) and (true_z is not None)
    has_true_mom = (true_mom_x is not None) and (true_mom_y is not None) and (true_mom_z is not None)

    if has_truth:
        ax3d.scatter(
            [float(true_x)], [float(true_y)], [float(true_z)],
            s=80, c="red", marker="o", depthshade=False, label=true_label
        )

    dx = float(DRIFT_VELOCITY)  # 1 tick -> cm
    dz = float(X_WIRE_PITCH)    # collection pitch
    dy = 0.78

    cmap3d = plt.cm.plasma

    xlim_final = ylim_final = zlim_final = None

    if xyz_selected:
        xs = np.array([p[0] for p in xyz_selected], dtype=float)
        ys = np.array([p[1] for p in xyz_selected], dtype=float)
        zs = np.array([p[2] for p in xyz_selected], dtype=float)
        amps = np.array([p[3] for p in xyz_selected], dtype=float)

        # -----------------------------
        # TOP-80%-CHARGE FILTER (NEW)
        # Keep highest-charge voxels until cumulative charge reaches 80% of total charge.
        # -----------------------------
        amps_rank = np.nan_to_num(amps, nan=0.0, posinf=0.0, neginf=0.0)
        amps_rank = np.maximum(amps_rank, 0.0)  # treat negative as 0 for "deposited charge"
        total_q = float(np.sum(amps_rank))

        if total_q > 0.0:
            order = np.argsort(amps_rank)[::-1]  # descending by charge
            csum = np.cumsum(amps_rank[order])
            cutoff = 0.80 * total_q

            # indices in sorted order to keep
            keep_sorted = np.nonzero(csum <= cutoff)[0]
            if keep_sorted.size == 0:
                # if the top voxel alone exceeds 80%, keep it
                keep_sorted = np.array([0], dtype=int)
            else:
                # include the first voxel that crosses the threshold
                last = keep_sorted[-1]
                if last + 1 < order.size:
                    keep_sorted = np.append(keep_sorted, last + 1)

            keep_idx = order[keep_sorted]

            xs = xs[keep_idx]
            ys = ys[keep_idx]
            zs = zs[keep_idx]
            amps = amps[keep_idx]
        # -----------------------------
        # end TOP-80%-CHARGE FILTER
        # -----------------------------

        dy_eff = dy * np.ones_like(xs)

        vmin = float(np.min(amps))
        vmax = float(np.max(amps))
        if vmax <= vmin:
            vmax = vmin + 1.0

        norm = mpl.colors.Normalize(vmin=vmin, vmax=vmax)
        colors = cmap3d(norm(amps))

        ax3d.bar3d(
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
        fig.colorbar(sm, ax=ax3d, fraction=0.04, pad=0.15, label="Charge (norm)")

        pad_cm = 1.0
        if has_truth:
            xs_lim = np.concatenate([xs, [float(true_x)]])
            ys_lim = np.concatenate([ys, [float(true_y)]])
            zs_lim = np.concatenate([zs, [float(true_z)]])
        else:
            xs_lim, ys_lim, zs_lim = xs, ys, zs

        xlim_final = (float(np.min(xs_lim) - pad_cm), float(np.max(xs_lim) + pad_cm))
        ylim_final = (float(np.min(ys_lim) - pad_cm), float(np.max(ys_lim) + pad_cm))
        zlim_final = (float(np.min(zs_lim) - pad_cm), float(np.max(zs_lim) + pad_cm))

        ax3d.set_xlim(*xlim_final)
        ax3d.set_ylim(*ylim_final)
        ax3d.set_zlim(*zlim_final)

    else:
        ax3d.text2D(0.05, 0.95, "No 3D points", transform=ax3d.transAxes, va="top")

        if has_truth:
            pad_cm = 5.0
            xlim_final = (float(true_x - pad_cm), float(true_x + pad_cm))
            ylim_final = (float(true_y - pad_cm), float(true_y + pad_cm))
            zlim_final = (float(true_z - pad_cm), float(true_z + pad_cm))
            ax3d.set_xlim(*xlim_final)
            ax3d.set_ylim(*ylim_final)
            ax3d.set_zlim(*zlim_final)
        else:
            xlim_final = ax3d.get_xlim()
            ylim_final = ax3d.get_ylim()
            zlim_final = ax3d.get_zlim()

    # -----------------------------
    # Truth momentum arrow
    # -----------------------------
    if has_true_mom:
        mx, my, mz = float(true_mom_x), float(true_mom_y), float(true_mom_z)
        v = np.array([mx, my, mz], dtype=float)
        vnorm = float(np.linalg.norm(v))

        if vnorm > 0.0:
            x0l, x1l = ax3d.get_xlim()
            y0l, y1l = ax3d.get_ylim()
            z0l, z1l = ax3d.get_zlim()

            span = max(abs(x1l - x0l), abs(y1l - y0l), abs(z1l - z0l))
            desired_len = span / 3.0
            u = (v / vnorm) * desired_len

            if has_truth:
                ox, oy, oz = float(true_x), float(true_y), float(true_z)
            else:
                ox, oy, oz = (x0l + x1l) / 2.0, (y0l + y1l) / 2.0, (z0l + z1l) / 2.0

            ax3d.quiver(
                ox, oy, oz,
                float(u[0]), float(u[1]), float(u[2]),
                color="red",
                arrow_length_ratio=0.15,
                linewidth=2.0,
                label=true_mom_label,
            )

    if has_truth or has_true_mom:
        ax3d.legend(loc="upper left")

    ax3d.view_init(elev=20, azim=-65)

    plt.tight_layout(pad=0.7, rect=[0, 0, 1, 0.96])
    return fig, axes

# -----------------------------
# Parsing helper
# -----------------------------
def _resolve_input_files(args) -> List[str]:
    """
    Returns an ordered list of ROOT files to process.
    Priority:
      1) --clusters-file (single file)
      2) --clusters-dir (directory; take first n ROOT-like files)
      3) JSON config: clusters_file or clusters_dir (+ optional n_files)
    """
    # JSON can provide defaults
    cfg: Dict[str, Any] = {}
    if args.json:
        with open(args.json, "r") as f:
            cfg = json.load(f)

    clusters_file = args.clusters_file or cfg.get("clusters_file")
    clusters_dir = args.clusters_dir or cfg.get("clusters_dir")
    n_files = args.n_files if args.n_files is not None else cfg.get("n_files", 1)

    # 1) Single file
    if clusters_file:
        p = Path(clusters_file)
        if not p.is_file():
            raise SystemExit(f"ERROR: --clusters-file not found: {p}")
        return [str(p)]

    # 2) Directory
    if clusters_dir:
        d = Path(clusters_dir)
        if not d.is_dir():
            raise SystemExit(f"ERROR: --clusters-dir not found or not a directory: {d}")

        # tweak patterns if your files use something else
        patterns = ("*.root", "*.ROOT")
        files: List[Path] = []
        for pat in patterns:
            files.extend(d.glob(pat))

        # stable order: lexicographic by path (you can change to mtime if you want)
        files = sorted(set(files), key=lambda p: str(p))

        if not files:
            raise SystemExit(f"ERROR: No ROOT files found in directory: {d}")

        n = int(n_files)
        if n <= 0:
            raise SystemExit("ERROR: -n must be >= 1")

        return [str(p) for p in files[:n]]

    raise SystemExit("ERROR: provide --clusters-file or --clusters-dir (or JSON with clusters_file/clusters_dir).")


# -----------------------------
# CLI
# -----------------------------

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List, Tuple

import matplotlib.pyplot as plt

# ... your existing imports/types:
# MatchedClustersLoader, MatchedGroup, etc.


def _resolve_input_files(args) -> List[str]:
    """
    Returns an ordered list of ROOT files to process.
    Priority:
      1) --clusters-file (single file)
      2) --clusters-dir (directory; take first n ROOT-like files)
      3) JSON config: clusters_file or clusters_dir (+ optional n_files)
    """
    # JSON can provide defaults
    cfg: Dict[str, Any] = {}
    if args.json:
        with open(args.json, "r") as f:
            cfg = json.load(f)

    clusters_file = args.clusters_file or cfg.get("clusters_file")
    clusters_dir = args.clusters_dir or cfg.get("clusters_dir")
    n_files = args.n_files if args.n_files is not None else cfg.get("n_files", 1)

    # 1) Single file
    if clusters_file:
        p = Path(clusters_file)
        if not p.is_file():
            raise SystemExit(f"ERROR: --clusters-file not found: {p}")
        return [str(p)]

    # 2) Directory
    if clusters_dir:
        d = Path(clusters_dir)
        if not d.is_dir():
            raise SystemExit(f"ERROR: --clusters-dir not found or not a directory: {d}")

        # tweak patterns if your files use something else
        patterns = ("*.root", "*.ROOT")
        files: List[Path] = []
        for pat in patterns:
            files.extend(d.glob(pat))

        # stable order: lexicographic by path (you can change to mtime if you want)
        files = sorted(set(files), key=lambda p: str(p))

        if not files:
            raise SystemExit(f"ERROR: No ROOT files found in directory: {d}")

        n = int(n_files)
        if n <= 0:
            raise SystemExit("ERROR: -n must be >= 1")

        return [str(p) for p in files[:n]]

    raise SystemExit("ERROR: provide --clusters-file or --clusters-dir (or JSON with clusters_file/clusters_dir).")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Sequential viewer for matched clusters (U/V/X) with waveform shapes + 3D reco panel"
    )

    # Existing
    parser.add_argument("--clusters-file", help="Input matched_clusters ROOT file")
    parser.add_argument("-j", "--json", help="Optional JSON config (can provide clusters_file or clusters_dir)")

    # New
    parser.add_argument("--clusters-dir", help="Directory containing matched_clusters ROOT files")
    parser.add_argument("-n", "--n-files", type=int, default=None,
                        help="If using --clusters-dir, load the first N files (default: 1, or json n_files)")

    # ... keep the rest of your args unchanged ...
    parser.add_argument("--expand", type=float, default=1.5)
    parser.add_argument("--figsize", type=float, nargs=2, default=(14, 10))
    parser.add_argument("--keep-negative", action="store_true")
    parser.add_argument("--no-tdc-to-tpc", action="store_true")
    parser.add_argument("--tdc-factor", type=float, default=32.0)
    parser.add_argument("--draw-mode", default="pentagon", choices=["pentagon", "triangle", "rectangle"])
    parser.add_argument("--thr-u", type=float, default=70.0)
    parser.add_argument("--thr-v", type=float, default=70.0)
    parser.add_argument("--thr-x", type=float, default=60.0)
    parser.add_argument("--omp-iters", type=int, default=30, help="Maximum OMP iterations")
    parser.add_argument("--omp-max-reuse", type=int, default=1, help="Max reuse per (tick,wire) bin")
    parser.add_argument("--omp-stop-l1", type=float, default=1e-6, help="OMP stop threshold (L1 residual)")
    parser.add_argument("--show-all-hypotheses", action="store_true",
                        help="Also plot all UVX-consistent hypotheses (faint) in 3D panel")
    parser.add_argument("--use-peak-tick", action="store_true",
                        help="Use peak-tick charge placement (instead of uniform TOT distribution)")
    parser.add_argument("--expand-dt", type=int, default=1,
                        help="Timing tolerance in ticks for TP placement (±expand_dt)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")

    args = parser.parse_args()

    files = _resolve_input_files(args)

    print(f"Will process {len(files)} file(s).")
    for fpath in files:
        print(f"  - {fpath}")

    plt.ion()

    for file_idx, clusters_file in enumerate(files, 1):
        print(f"\n=== [{file_idx}/{len(files)}] Loading: {clusters_file} ===")

        loader = MatchedClustersLoader(
            clusters_file,
            tdc_to_tpc=not args.no_tdc_to_tpc,
            tdc_to_tpc_factor=args.tdc_factor,
            verbose=args.verbose,
        )

        result = loader.load(build_matches=True, require_nonnegative_match_id=not args.keep_negative)
        groups_dict: Dict[Tuple[int, int], MatchedGroup] = result.get("matches_by_id", {})
        if not groups_dict:
            print("No matched groups found in file; skipping.")
            continue

        groups = sorted(groups_dict.values(), key=lambda g: (g.event_id, g.match_id))

        print(f"Loaded {len(groups)} matched groups")
        print("Close window to advance. Ctrl+C to quit.\n")

        for i, group in enumerate(groups, 1):
            if group.event_id == 11168:
                continue

            print(f"[{i}/{len(groups)}] Event {group.event_id}, match_id {group.match_id}")
            print(f"Truth_position: x={group.true_x} y={group.true_y}, z={group.true_z}")

            U_id, V_id, X_id = xyz_to_UVX_wires_lut(
                group.true_x, group.true_y, group.true_z,
                u_y_sorted=U_y_sorted, u_u_sorted=U_u_sorted,
                v_y_sorted=V_y_sorted, v_v_sorted=V_v_sorted
            )

            print(f"Backtracked ids: X:{X_id} U:{U_id} V:{V_id}")

            views_obj = extract_matched_event_views_object(group, order=("X", "U", "V"))

            # -----------------------------
            # 3D reconstruction (OMP)
            # -----------------------------
            xyz_selected = None
            xyz_all = None

            try:
                cand = reco_3d.generate_candidates(
                    group,
                    use_peak_tick=bool(args.use_peak_tick),
                    expand_dt=int(args.expand_dt),
                )

                xyz_hyp = reco_3d.process_candidates_to_xyz(cand)

                if args.show_all_hypotheses:
                    xyz_all = [(c[1], c[2], c[3]) for c in xyz_hyp]

                Y_U, Y_V, Y_X = reco_3d.build_measurements_from_candidates(cand)

                omp_res = reco_3d.omp_select_min_support(
                    xyz_hyp,
                    Y_U, Y_V, Y_X,
                    max_iters=int(args.omp_iters),
                    stop_l1=float(args.omp_stop_l1),
                    max_reuse_per_bin=int(args.omp_max_reuse),
                )

                cloud_res = reco_3d.reconstruct_cloud_dense(
                    xyz_hyp,
                    Y_U, Y_V, Y_X,
                    max_passes=5,
                    prefer_triples=1.2,
                    softmax_temp=1.0,
                )

                xyz_selected = [(cand_sel[1], cand_sel[2], cand_sel[3], float(amp))
                                for (amp, cand_sel) in cloud_res.selected]

                if args.verbose:
                    print(f"  3D: hypotheses={len(xyz_hyp)} selected={len(omp_res.selected)} "
                          f"residual_L1={omp_res.residual_norm:.3g}")

            except Exception as e:
                print(f"  [WARN] 3D reco failed: {e}")

            fig, _ = plot_matched_event_views(
                views_obj,
                draw_mode=args.draw_mode,
                thr_u=args.thr_u,
                thr_v=args.thr_v,
                thr_x=args.thr_x,
                expand=args.expand,
                figsize=tuple(args.figsize),
                xyz_selected=xyz_selected,
                xyz_all=xyz_all,
                true_x=group.true_x,
                true_y=group.true_y,
                true_z=group.true_z,
                true_U_id=U_id,
                true_V_id=V_id,
                true_X_id=X_id,
                true_mom_x=group.true_mom_x,
                true_mom_y=group.true_mom_y,
                true_mom_z=group.true_mom_z,
            )

            if fig is None:
                continue

            plt.show(block=True)
            plt.close(fig)

    print("\nDone. No more matched clusters.")


if __name__ == "__main__":
    main()
