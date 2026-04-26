from __future__ import annotations

import numpy as np

from collections import defaultdict
from typing import Dict, DefaultDict, List, Tuple, Any, NamedTuple, Optional, Sequence

from space_transformations import (
    X_wire_to_X_sign,
    UX_wires_to_yz,
    VX_wires_to_yz,
    xyz_to_UVX_wires_lut,
    build_uv_sorted_luts_one_apa
)

WireCharge = Tuple[int, float]
Hits = Sequence[WireCharge]
CandidateRowQ = Tuple[int, List[List[WireCharge]]]

# Allow "missing plane" for partial-view candidates
XYZCandidate = Tuple[
    int,           # tick
    float, float, float,   # x,y,z
    Optional[int], Optional[int], Optional[int],  # u_wire, v_wire, x_wire
    float, float, float,   # qU, qV, qX (0.0 if missing plane)
]

# -------------------------------------------------------------------------
# Geometry / detector constants
# -------------------------------------------------------------------------

#: Drift velocity in cm per TPC tick
DRIFT_VELOCITY_CM_PER_TICK = 0.0805

#: Physical gap between planes in cm
PLANE_GAP_CM = 0.47

#: Time gap between planes in ticks
GAP_TICKS = - int(round(PLANE_GAP_CM / DRIFT_VELOCITY_CM_PER_TICK))


# -------------------------------------------------------------------------
# Precomputed LUT support arrays (built once at import)
# -------------------------------------------------------------------------

U_y_sorted, U_u_sorted, V_y_sorted, V_v_sorted = build_uv_sorted_luts_one_apa(
    error_margin=1e-4
)

# -------------------------------------------------------------------------
# Helper: TP payload -> tick → (wire → charge) map
# -------------------------------------------------------------------------

def _cluster_tick_to_wirecharge(
    cluster: Any,
    *,
    use_peak_tick: bool = False,
    expand_dt: int = 0,
) -> Dict[int, Dict[int, float]]:
    """
    Convert a ClusterItem TP payload into a mapping from ticks to per-wire charge.

    This function builds a sparse representation of the plane "image" directly
    from trigger primitives (TPs):

        tick -> {wire_id: charge_at_that_tick, ...}

    Charge is derived from TP adc_integral and distributed over time.

    Parameters
    ----------
    cluster
        ClusterItem containing TP arrays:
        - ch: wire/channel ids
        - tstart: TP start tick (TPC ticks if converted in loader)
        - sot: samples over threshold (TOT width)
        - stopeak: samples to peak
        - adc_integral: integrated ADC for the TP
    use_peak_tick
        If True, assign the full adc_integral to the single peak tick
        (tstart + stopeak). If False, distribute adc_integral uniformly
        over the TOT ticks [tstart, tstart+sot-1].
    expand_dt
        Timing tolerance: expand each tick by ±expand_dt. If expand_dt > 0,
        the per-tick charge is split evenly across the expanded ticks to avoid
        artificially inflating total charge.

    Returns
    -------
    Dict[int, Dict[int, float]]
        Mapping: tick -> dict(wire -> charge).
    """
    tick_to_wirecharge: DefaultDict[int, DefaultDict[int, float]] = defaultdict(lambda: defaultdict(float))

    ch = getattr(cluster, "ch", []) or []
    tstart = getattr(cluster, "tstart", []) or []
    sot = getattr(cluster, "sot", []) or []
    stopeak = getattr(cluster, "stopeak", []) or []
    adc_integral = getattr(cluster, "adc_integral", []) or []

    n = min(len(ch), len(tstart), len(adc_integral))
    #n = [108]

    for i in range(n):
        wire = int(ch[i])
        t0 = int(round(float(tstart[i])))
        q_total = float(adc_integral[i])

        if q_total <= 0.0:
            continue

        if use_peak_tick:
            t_peak = int(t0 + (int(stopeak[i]) if i < len(stopeak) else 0))
            ticks = [t_peak]
            q_per_tick = q_total
        else:
            width = int(sot[i]) if i < len(sot) else 1
            width = max(width, 1)
            ticks = list(range(t0, t0 + width))
            q_per_tick = q_total / float(width)

        # Apply optional ±expand_dt timing tolerance.
        # We split the per-tick charge across expanded ticks to preserve total charge.
        if expand_dt > 0:
            spread = 2 * expand_dt + 1
            q_per_tick_spread = q_per_tick / float(spread)
            for t in ticks:
                for dt in range(-expand_dt, expand_dt + 1):
                    tick_to_wirecharge[t + dt][wire] += q_per_tick_spread
        else:
            for t in ticks:
                tick_to_wirecharge[t][wire] += q_per_tick

    # Convert nested defaultdicts to plain dicts
    return {t: dict(wq) for t, wq in tick_to_wirecharge.items()}


# -------------------------------------------------------------------------
# Build per-tick wire candidates
# -------------------------------------------------------------------------

def generate_candidates(
    matchedCluster: Any,
    *,
    use_peak_tick: bool = False,
    expand_dt: int = 0,
    gap_ticks: int = GAP_TICKS,
) -> Dict[str, Any]:
    """
    Generate per-tick U/V/X wire candidates with per-wire charge.

    For each collection-plane (X) tick t, this returns:
      - X hits at (t)
      - V hits at (t - gap_ticks)
      - U hits at (t - 2*gap_ticks)

    Each hit list is a list of (wire_id, charge) tuples, which is the required
    input to implement OMP (the charge values form your measurement vector Y).

    Parameters
    ----------
    matchedCluster
        MatchedTriplet or MatchedGroup-like object containing U, V, X clusters.
    use_peak_tick
        If True, use only TP peak ticks for charge placement.
    expand_dt
        Expand TP tick ranges by ±expand_dt (timing tolerance).
    gap_ticks
        Plane offset in ticks.

    Returns
    -------
    Dict[str, Any]
        {
            "apa": int,
            "candidates": [
                (tick, [u_hits, v_hits, x_hits]),
                ...
            ]
        }
        where u_hits/v_hits/x_hits are List[Tuple[int, float]].
    """
    # Extract plane clusters
    if hasattr(matchedCluster, "U") and hasattr(matchedCluster, "V") and hasattr(matchedCluster, "X"):
        Uc = matchedCluster.U
        Vc = matchedCluster.V
        Xc = matchedCluster.X
    elif hasattr(matchedCluster, "get"):
        Uc = matchedCluster.get("U")
        Vc = matchedCluster.get("V")
        Xc = matchedCluster.get("X")
    elif hasattr(matchedCluster, "clusters"):
        Uc = matchedCluster.clusters.get("U")
        Vc = matchedCluster.clusters.get("V")
        Xc = matchedCluster.clusters.get("X")
    else:
        raise TypeError("matchedCluster must contain U/V/X clusters")

    if Xc is None:
        return {"apa": -1, "candidates": []}

    apa = int(getattr(Xc, "apa", -1))

    # tick -> {wire: charge}
    X_tick_q = _cluster_tick_to_wirecharge(Xc, use_peak_tick=use_peak_tick, expand_dt=expand_dt)
    V_tick_q = _cluster_tick_to_wirecharge(Vc, use_peak_tick=use_peak_tick, expand_dt=expand_dt) if Vc else {}
    U_tick_q = _cluster_tick_to_wirecharge(Uc, use_peak_tick=use_peak_tick, expand_dt=expand_dt) if Uc else {}

    collection_ticks = sorted(X_tick_q.keys())

    candidates: List[Tuple[int, List[List[Tuple[int, float]]]]] = []

    for tick in collection_ticks:
        # Convert dicts to sorted (wire, charge) lists for determinism/debuggability
        x_hits = sorted(X_tick_q.get(tick - 2 * gap_ticks, {}).items())
        v_hits = sorted(V_tick_q.get(tick - gap_ticks,     {}).items()) if V_tick_q else []
        u_hits = sorted(U_tick_q.get(tick,                 {}).items()) if U_tick_q else []

        candidates.append((tick, [u_hits, v_hits, x_hits]))

    return {"apa": apa, "candidates": candidates}


# -------------------------------------------------------------------------
# Convert wire candidates → 3D points
# -------------------------------------------------------------------------

def process_candidates_to_xyz(candidates: Dict[str, Any]) -> List[XYZCandidate]:
    apa = int(candidates["apa"])
    cand_ids: List[CandidateRowQ] = candidates["candidates"]

    out: List[XYZCandidate] = []

    for tick, hits in cand_ids:
        u_hits, v_hits, x_hits = hits
        if not x_hits:
            continue

        u_q = {int(w): float(q) for (w, q) in u_hits} if u_hits else {}
        v_q = {int(w): float(q) for (w, q) in v_hits} if v_hits else {}
        x_q = {int(w): float(q) for (w, q) in x_hits}

        u_ids = list(u_q.keys())
        v_ids = list(v_q.keys())
        x_ids = list(x_q.keys())

        u_set, v_set, x_set = set(u_ids), set(v_ids), set(x_ids)

        x = (tick * DRIFT_VELOCITY_CM_PER_TICK + 3 + 2 * PLANE_GAP_CM) * X_wire_to_X_sign(x_ids[0], apa) ## + 3 from geometry mismatch (FIX this hacky stuff!!)

        have_u = len(u_ids) > 0
        have_v = len(v_ids) > 0
        if not have_u and not have_v:
            continue

        triples: List[XYZCandidate] = []
        doubles: List[XYZCandidate] = []

        # -------------------------
        # 1) Try to build triples (only possible if both U and V exist)
        # -------------------------
        if have_u and have_v:
            for x_wire in x_ids:
                for u_wire in u_ids:
                    prelim_y, prelim_z = UX_wires_to_yz(u_wire, x_wire, apa)

                    u_pb, v_pb, x_pb = xyz_to_UVX_wires_lut(
                        x, prelim_y, prelim_z,
                        U_y_sorted, U_u_sorted,
                        V_y_sorted, V_v_sorted,
                        error_margin=1e-3,
                        flip_face_from_x=False,
                    )

                    # Strict triple overlap
                    if (int(u_pb) == int(u_wire)) and (int(x_pb) == int(x_wire)) and (int(v_pb) in v_set):
                        triples.append(
                            (
                                int(tick),
                                float(x), float(prelim_y), float(prelim_z),
                                int(u_wire), int(v_pb), int(x_wire),
                                float(u_q.get(int(u_wire), 0.0)),
                                float(v_q.get(int(v_pb), 0.0)),
                                float(x_q.get(int(x_wire), 0.0)),
                            )
                        )

        # If any triple exists, emit ONLY triples and skip doubles
        if triples:
            out.extend(triples)
            continue

        # -------------------------
        # 2) No triples -> emit both double overlaps (UX and VX) if available
        # -------------------------

        # UX doubles
        if have_u:
            for x_wire in x_ids:
                for u_wire in u_ids:
                    prelim_y, prelim_z = UX_wires_to_yz(u_wire, x_wire, apa)

                    u_pb, v_pb, x_pb = xyz_to_UVX_wires_lut(
                        x, prelim_y, prelim_z,
                        U_y_sorted, U_u_sorted,
                        V_y_sorted, V_v_sorted,
                        error_margin=1e-3,
                        flip_face_from_x=False,
                    )

                    # Require the pair you generated from to round-trip (UX)
                    #if (int(u_pb) != int(u_wire)) or (int(x_pb) != int(x_wire)):
                    #    continue

                    doubles.append(
                        (
                            int(tick),
                            float(x), float(prelim_y), float(prelim_z),
                            int(u_wire), None, int(x_wire),
                            float(u_q.get(int(u_wire), 0.0)),
                            0.0,
                            float(x_q.get(int(x_wire), 0.0)),
                        )
                    )

        # VX doubles
        if have_v:
            for x_wire in x_ids:
                for v_wire in v_ids:
                    prelim_y, prelim_z = VX_wires_to_yz(v_wire, x_wire, apa)

                    u_pb, v_pb, x_pb = xyz_to_UVX_wires_lut(
                        x, prelim_y, prelim_z,
                        U_y_sorted, U_u_sorted,
                        V_y_sorted, V_v_sorted,
                        error_margin=1e-3,
                        flip_face_from_x=False,
                    )

                    # Require the pair you generated from to round-trip (VX)
                    #if (int(v_pb) != int(v_wire)) or (int(x_pb) != int(x_wire)):
                    #    continue

                    doubles.append(
                        (
                            int(tick),
                            float(x), float(prelim_y), float(prelim_z),
                            None, int(v_wire), int(x_wire),
                            0.0,
                            float(v_q.get(int(v_wire), 0.0)),
                            float(x_q.get(int(x_wire), 0.0)),
                        )
                    )

        out.extend(doubles)

    return out



def build_measurements_from_candidates(
    candidates: Dict[str, Any],
) -> Tuple[Dict[Tuple[int, int], float], Dict[Tuple[int, int], float], Dict[Tuple[int, int], float]]:
    """
    Build per-plane measurement dictionaries Y_U, Y_V, Y_X from charge-aware candidates.

    Parameters
    ----------
    candidates
        Output of generate_candidates() (charge-aware), i.e. per tick:
            u_hits: [(wire, charge)]
            v_hits: [(wire, charge)]
            x_hits: [(wire, charge)]

    Returns
    -------
    (Y_U, Y_V, Y_X)
        Each is a dict mapping (tick, wire) -> charge.
    """
    Y_U: Dict[Tuple[int, int], float] = {}
    Y_V: Dict[Tuple[int, int], float] = {}
    Y_X: Dict[Tuple[int, int], float] = {}

    for tick, (u_hits, v_hits, x_hits) in candidates["candidates"]:
        for w, q in u_hits:
            Y_U[(int(tick), int(w))] = Y_U.get((int(tick), int(w)), 0.0) + float(q)
        for w, q in v_hits:
            Y_V[(int(tick), int(w))] = Y_V.get((int(tick), int(w)), 0.0) + float(q)
        for w, q in x_hits:
            Y_X[(int(tick), int(w))] = Y_X.get((int(tick), int(w)), 0.0) + float(q)

    return Y_U, Y_V, Y_X


class OMPResult(NamedTuple):
    """
    Result of OMP selection.

    selected
        List of selected candidates with fitted amplitudes.
        Each item is (amplitude, candidate).
    residual_norm
        Final residual L1 norm across all measurement bins.
    """
    selected: List[Tuple[float, XYZCandidate]]
    residual_norm: float


def omp_select_min_support(
    xyz_candidates: List[XYZCandidate],
    Y_U: Dict[Tuple[int, int], float],
    Y_V: Dict[Tuple[int, int], float],
    Y_X: Dict[Tuple[int, int], float],
    *,
    max_iters: int = 200,
    stop_l1: float = 1e-3,
    min_amp: float = 1e-6,

    # Reuse control
    max_reuse_per_bin: int = 10,         # raise default; 2 tends to kill long structures
    hard_reuse_cap: bool = True,         # if False, use soft penalty instead of hard skip
    reuse_soft_penalty: float = 0.15,    # only used when hard_reuse_cap=False

    # Scoring behavior
    plane_weight_X: float = 1.5,
    plane_weight_U: float = 1.0,
    plane_weight_V: float = 1.0,
    nplane_bonus: float = 0.5,          # \ preference for 3-plane over 2-plane
    new_xbin_bonus: float = 0.1,         # set ~0.1–0.5 to encourage extension (tracks)

    # Optional: soft missing-plane penalty (NOT a gate)
    missing_plane_soft_penalty: float = 0.01,  # e.g. 0.2 to mildly downweight 2-plane when missing plane has signal
    missing_plane_eps: float = 60.0,          # your zero-supp threshold (per tick sum)
) -> OMPResult:
    """
    Greedy nonnegative OMP that favors *coverage* (sum of residuals) rather than peaks.

    Candidate amplitude is still chosen safely as min(residuals on used bins).
    Score favors explaining lots of charge, lightly prefers 3-plane candidates, and can
    optionally encourage selecting previously-unused X bins.

    Works for both blobs and tracks.
    """

    # Residuals start as measurements
    R_U = dict(Y_U)
    R_V = dict(Y_V)
    R_X = dict(Y_X)

    reuse_U: Dict[Tuple[int, int], int] = defaultdict(int)
    reuse_V: Dict[Tuple[int, int], int] = defaultdict(int)
    reuse_X: Dict[Tuple[int, int], int] = defaultdict(int)

    selected: List[Tuple[float, XYZCandidate]] = []

    # Per-tick residual sums for optional missing-plane soft penalty
    Rsum_U: DefaultDict[int, float] = defaultdict(float)
    Rsum_V: DefaultDict[int, float] = defaultdict(float)
    for (t, _w), q in R_U.items():
        Rsum_U[int(t)] += float(q)
    for (t, _w), q in R_V.items():
        Rsum_V[int(t)] += float(q)

    used_X_bins: set[Tuple[int, int]] = set()

    def residual_l1() -> float:
        return (
            sum(abs(v) for v in R_U.values())
            + sum(abs(v) for v in R_V.values())
            + sum(abs(v) for v in R_X.values())
        )

    def reuse_factor(count: int) -> float:
        """Soft penalty factor in (0,1] for higher reuse counts."""
        # 1 / (1 + alpha*count) is simple + stable
        return 1.0 / (1.0 + reuse_soft_penalty * float(count))

    for _ in range(max_iters):
        if residual_l1() <= stop_l1:
            break

        best_score = -1.0
        best_idx: Optional[int] = None
        best_amp = 0.0

        for i, cand in enumerate(xyz_candidates):
            tick, _x, _y, _z, u_w, v_w, x_w, _qU, _qV, _qX = cand
            if x_w is None:
                continue

            kx = (int(tick), int(x_w))
            rx = R_X.get(kx, 0.0)
            if rx <= 0.0:
                continue

            # Hard or soft reuse control on X
            if hard_reuse_cap:
                if reuse_X[kx] >= max_reuse_per_bin:
                    continue
                fx = 1.0
            else:
                fx = reuse_factor(reuse_X[kx])

            residuals: List[float] = [rx]
            weights: List[float] = [plane_weight_X]
            nplanes = 1

            # U plane if present
            if u_w is not None:
                ku = (int(tick), int(u_w))
                ru = R_U.get(ku, 0.0)
                if ru <= 0.0:
                    continue
                if hard_reuse_cap:
                    if reuse_U[ku] >= max_reuse_per_bin:
                        continue
                    fu = 1.0
                else:
                    fu = reuse_factor(reuse_U[ku])
                residuals.append(ru)
                weights.append(plane_weight_U * fu)
                nplanes += 1

            # V plane if present
            if v_w is not None:
                kv = (int(tick), int(v_w))
                rv = R_V.get(kv, 0.0)
                if rv <= 0.0:
                    continue
                if hard_reuse_cap:
                    if reuse_V[kv] >= max_reuse_per_bin:
                        continue
                    fv = 1.0
                else:
                    fv = reuse_factor(reuse_V[kv])
                residuals.append(rv)
                weights.append(plane_weight_V * fv)
                nplanes += 1

            # Need at least 2 planes (X + one induction)
            if nplanes < 2:
                continue

            # Safe nonnegative amplitude
            a = min(residuals)
            if a < min_amp:
                continue

            # Coverage-based score (weighted sum), with light nplane preference
            score = 0.0
            for r, w in zip(residuals, weights):
                score += w * r
            score *= (1.0 + nplane_bonus * float(nplanes - 2))  # 2-plane baseline; 3-plane gets boost

            # Encourage exploring new X bins (helps tracks extend; blobs mostly unaffected)
            if new_xbin_bonus > 0.0 and kx not in used_X_bins:
                score *= (1.0 + float(new_xbin_bonus))

            # Optional: soft missing-plane downweight (NOT a gate)
            if missing_plane_soft_penalty > 0.0 and nplanes == 2:
                if u_w is not None and v_w is None:
                    missing_sum = Rsum_V.get(int(tick), 0.0)
                elif v_w is not None and u_w is None:
                    missing_sum = Rsum_U.get(int(tick), 0.0)
                else:
                    missing_sum = 0.0

                if missing_sum > missing_plane_eps:
                    score *= max(0.0, 1.0 - float(missing_plane_soft_penalty))

            # Apply X reuse factor last (so repeated X bins naturally get less attractive)
            score *= fx

            if score > best_score:
                best_score = score
                best_idx = i
                best_amp = a

        if best_idx is None:
            break

        # Apply best candidate
        cand = xyz_candidates[best_idx]
        tick, _x, _y, _z, u_w, v_w, x_w, _qU, _qV, _qX = cand
        a = best_amp

        # Subtract from X
        kx = (int(tick), int(x_w))
        old_rx = R_X.get(kx, 0.0)
        new_rx = max(0.0, old_rx - a)
        R_X[kx] = new_rx
        reuse_X[kx] += 1
        used_X_bins.add(kx)

        # Subtract from U if present
        if u_w is not None:
            ku = (int(tick), int(u_w))
            old_ru = R_U.get(ku, 0.0)
            new_ru = max(0.0, old_ru - a)
            R_U[ku] = new_ru
            reuse_U[ku] += 1
            Rsum_U[int(tick)] -= (old_ru - new_ru)

        # Subtract from V if present
        if v_w is not None:
            kv = (int(tick), int(v_w))
            old_rv = R_V.get(kv, 0.0)
            new_rv = max(0.0, old_rv - a)
            R_V[kv] = new_rv
            reuse_V[kv] += 1
            Rsum_V[int(tick)] -= (old_rv - new_rv)

        selected.append((a, cand))

    return OMPResult(selected=selected, residual_norm=residual_l1())




def reconstruct_cloud_dense(
    xyz_candidates: List[XYZCandidate],
    Y_U: Dict[Tuple[int, int], float],
    Y_V: Dict[Tuple[int, int], float],
    Y_X: Dict[Tuple[int, int], float],
    *,
    min_amp: float = 1e-6,
    max_passes: int = 5,            # small number of refinement sweeps
    ux_weight: float = 1.0,
    vx_weight: float = 1.0,
    x_weight: float = 1.0,
    prefer_triples: float = 2.,    # >1 boosts triple hypotheses vs doubles
    softmax_temp: float = 1.0,      # 1.0 linear, <1 sharper, >1 flatter
) -> OMPResult:
    """
    Dense, charge-preserving cloud reconstruction.

    - Never collapses to a few points like sparse OMP.
    - Uses X bins as anchors and distributes X charge across candidate hypotheses.
    - Iteratively respects U/V capacity by clamping to available residuals.

    Returns OMPResult:
      selected: List[(amplitude, candidate)]
      residual_norm: final residual L1
    """

    # Residual capacities
    R_U = dict(Y_U)
    R_V = dict(Y_V)
    R_X = dict(Y_X)

    # Group candidates by X bin
    by_xbin: DefaultDict[Tuple[int, int], List[int]] = defaultdict(list)
    for i, cand in enumerate(xyz_candidates):
        t, _x, _y, _z, u_w, v_w, x_w, _qU, _qV, _qX = cand
        if x_w is None:
            continue
        by_xbin[(int(t), int(x_w))].append(i)

    assigned_amp = [0.0] * len(xyz_candidates)

    def residual_l1() -> float:
        return (
            sum(abs(v) for v in R_U.values())
            + sum(abs(v) for v in R_V.values())
            + sum(abs(v) for v in R_X.values())
        )

    def score_for_candidate(cand: XYZCandidate) -> float:
        t, _x, _y, _z, u_w, v_w, x_w, _qU, _qV, _qX = cand

        # Use current residuals as “compatibility”
        s = 0.0
        if x_w is not None:
            s += x_weight * max(0.0, R_X.get((int(t), int(x_w)), 0.0))
        if u_w is not None:
            s += ux_weight * max(0.0, R_U.get((int(t), int(u_w)), 0.0))
        if v_w is not None:
            s += vx_weight * max(0.0, R_V.get((int(t), int(v_w)), 0.0))

        # Slight preference for triples
        nplanes = 1 + (1 if u_w is not None else 0) + (1 if v_w is not None else 0)
        if nplanes == 3:
            s *= prefer_triples

        # Temperature: flatten or sharpen
        if softmax_temp <= 0.0:
            return s
        return s ** (1.0 / float(softmax_temp)) if s > 0.0 else 0.0

    for _pass in range(max_passes):
        any_change = False

        for (t, xw), idxs in by_xbin.items():
            rx = R_X.get((t, xw), 0.0)
            if rx <= min_amp:
                continue

            # Compute weights for candidates anchored to this X bin
            weights: List[float] = []
            cands: List[int] = []
            for i in idxs:
                w = score_for_candidate(xyz_candidates[i])
                if w > 0.0:
                    weights.append(w)
                    cands.append(i)

            if not cands:
                continue

            wsum = sum(weights)
            if wsum <= 0.0:
                continue

            # Propose splitting X residual proportionally
            proposals = [rx * (w / wsum) for w in weights]

            # Clamp proposals by U/V capacities
            for prop, i in zip(proposals, cands):
                if prop <= min_amp:
                    continue

                cand = xyz_candidates[i]
                tt, _x, _y, _z, u_w, v_w, x_w2, _qU, _qV, _qX = cand

                if int(tt) != t or int(x_w2) != xw:
                    continue  # should not happen; just be safe

                cap = prop

                # X cap
                cap = min(cap, R_X.get((t, xw), 0.0))

                # U/V caps if present
                if u_w is not None:
                    cap = min(cap, R_U.get((t, int(u_w)), 0.0))
                if v_w is not None:
                    cap = min(cap, R_V.get((t, int(v_w)), 0.0))

                if cap <= min_amp:
                    continue

                # Apply subtraction
                R_X[(t, xw)] = max(0.0, R_X.get((t, xw), 0.0) - cap)
                if u_w is not None:
                    ku = (t, int(u_w))
                    R_U[ku] = max(0.0, R_U.get(ku, 0.0) - cap)
                if v_w is not None:
                    kv = (t, int(v_w))
                    R_V[kv] = max(0.0, R_V.get(kv, 0.0) - cap)

                assigned_amp[i] += cap
                any_change = True

        if not any_change:
            break

    selected = [(a, xyz_candidates[i]) for i, a in enumerate(assigned_amp) if a > min_amp]
    return OMPResult(selected=selected, residual_norm=residual_l1())
