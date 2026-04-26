#!/usr/bin/env python3
"""
matched_cluster_objects.py

Single-file utilities to LOAD matched-cluster objects from matched_clusters ROOT files.

Goal:
- Reuse your existing "viewer" style ClusterItem container (TP-level fields)
- Add minimal new structure to represent U/V/X matches
- Skip plotting/UI entirely
- Provide a clean API to:
    1) read clusters per plane (with TP arrays)
    2) build matched groups using match_id (and match_type if present)
    3) optionally build triplets (U+V+X) when available

Typical usage:
    from matched_cluster_objects import MatchedClustersLoader

    loader = MatchedClustersLoader("/path/to/file_matched.root")
    result = loader.load(build_matches=True)

    # clusters per plane
    u_clusters = result["clusters"]["U"]

    # all match groups keyed by match_id
    groups = result["matches_by_id"]  # {match_id: MatchedGroup}

    # triplets (only those with U,V,X present)
    triplets = result["triplets"]

Notes:
- This assumes matched_clusters files are stored in:
    clusters/clusters_tree_{U,V,X}
  (it also tries legacy trees if needed)
- Matching is done by (event, match_id) in case match_id values repeat across events.
- If your files also contain X-only partner branches (matching_clusterId_U/V),
  you can extend the branch list in _read_plane_clusters() similarly, but this
  file focuses on match_id-based grouping as in your Python analyzer.

"""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Iterable, Any

import numpy as np
import uproot


# -----------------------------
# Parameters (kept as-is, minimal)
# -----------------------------

class Parameters:
    """Load and manage parameters from .dat files (optional; not required for matching)."""

    def __init__(self, params_dir: str = "parameters"):
        self.params_dir = Path(params_dir)
        self.params: Dict[str, Any] = {}
        self.load_parameters()

    def load_parameters(self):
        for dat_file in ["display.dat", "timing.dat", "geometry.dat"]:
            file_path = self.params_dir / dat_file
            if file_path.exists():
                self._parse_dat_file(file_path)

    def _parse_dat_file(self, file_path: Path):
        with open(file_path, "r") as f:
            for line in f:
                line = line.strip()
                if line.startswith("<") and line.endswith(">"):
                    content = line[1:-1].strip()
                    if "=" in content:
                        key, value_str = content.split("=", 1)
                        key = key.strip()
                        value_str = value_str.strip()
                        try:
                            if "." in value_str or "e" in value_str.lower():
                                value = float(value_str)
                            else:
                                value = int(value_str)
                        except ValueError:
                            value = value_str
                        self.params[key] = value

    def get(self, key: str, default=None):
        return self.params.get(key, default)


# -----------------------------
# Core object model
# -----------------------------

@dataclass
class ClusterItem:
    """
    A TP-carrying cluster container (very close to your viewer's ClusterItem).
    Fields are expanded minimally to support matched-cluster workflows.
    """

    plane: str = ""
    event_id: int = -1

    # Cluster-level info
    cluster_id: int = -1
    n_tps: int = 0
    true_label: str = ""
    is_es_interaction: bool = False
    true_neutrino_energy: float = 0.0
    true_particle_energy: float = 0.0
    total_charge: float = 0.0
    total_energy: float = 0.0
    marley_tp_fraction: float = 0.0
    is_main_cluster: bool = False

    # Matching info (for matched_clusters files)
    match_id: int = -1
    match_type: int = -1

    # Truth info
    true_x = -999.0
    true_y = -999.0
    true_z = -999.0

    true_mom_x = -999.0
    true_mom_y = -999.0
    true_mom_z = -999.0

    # TP arrays (TPC ticks where relevant is left to user; we store as read)
    ch: List[int] = field(default_factory=list)
    tstart: List[float] = field(default_factory=list)   # can be TDC or TPC ticks depending on input branch; see loader options
    sot: List[int] = field(default_factory=list)
    stopeak: List[int] = field(default_factory=list)
    adc_peak: List[float] = field(default_factory=list)
    adc_integral: List[float] = field(default_factory=list)

    @property
    def is_marley(self) -> bool:
        # Match your analyzer idea: fraction > 0 indicates MARLEY content
        return float(self.marley_tp_fraction) > 0.0

    def summary(self) -> str:
        return (
            f"ClusterItem(plane={self.plane}, event={self.event_id}, cluster_id={self.cluster_id}, "
            f"match_id={self.match_id}, match_type={self.match_type}, n_tps={self.n_tps}, "
            f"marley_frac={self.marley_tp_fraction:.3f}, label={self.true_label})"
        )


@dataclass
class MatchedGroup:
    """
    Represents one matched object keyed by (event_id, match_id),
    potentially containing up to one cluster per plane.
    """
    event_id: int
    match_id: int
    clusters: Dict[str, ClusterItem] = field(default_factory=dict)

    # Truth
    @property
    def true_x(self) -> float:
        for p in ("X", "U", "V"):
            c = self.clusters.get(p)
            if c is not None:
                return c.true_x
        return -999.0

    @property
    def true_y(self) -> float:
        for p in ("X", "U", "V"):
            c = self.clusters.get(p)
            if c is not None:
                return c.true_y
        return -999.0
    
    @property
    def true_z(self) -> float:
        for p in ("X", "U", "V"):
            c = self.clusters.get(p)
            if c is not None:
                return c.true_z
        return -999.0
    
    @property
    def true_mom_x(self) -> float:
        for p in ("X", "U", "V"):
            c = self.clusters.get(p)
            if c is not None:
                return c.true_mom_x
        return -999.0

    @property
    def true_mom_y(self) -> float:
        for p in ("X", "U", "V"):
            c = self.clusters.get(p)
            if c is not None:
                return c.true_mom_y
        return -999.0
    
    @property
    def true_mom_z(self) -> float:
        for p in ("X", "U", "V"):
            c = self.clusters.get(p)
            if c is not None:
                return c.true_mom_z
        return -999.0

    # Optional: if you want to track multiple clusters per plane (in case of ambiguity),
    # you can extend this to store lists. For now we keep "best" per plane.
    # (Best selection policy is handled in loader.)

    def has_plane(self, plane: str) -> bool:
        return plane in self.clusters

    def get(self, plane: str) -> Optional[ClusterItem]:
        return self.clusters.get(plane)

    def planes_present(self) -> Tuple[str, ...]:
        return tuple(sorted(self.clusters.keys()))

    def is_triplet(self) -> bool:
        return all(p in self.clusters for p in ("U", "V", "X"))


@dataclass
class MatchedTriplet:
    """Convenience wrapper for complete U/V/X matched groups."""
    event_id: int
    match_id: int
    U: ClusterItem
    V: ClusterItem
    X: ClusterItem

    def as_group(self) -> MatchedGroup:
        return MatchedGroup(
            event_id=self.event_id,
            match_id=self.match_id,
            clusters={"U": self.U, "V": self.V, "X": self.X},
        )


# -----------------------------
# Loader
# -----------------------------

class MatchedClustersLoader:
    """
    Load clusters (with TP payload) from a matched_clusters ROOT file and build matched objects.
    """

    def __init__(
        self,
        clusters_file: str,
        *,
        tdc_to_tpc: bool = True,
        tdc_to_tpc_factor: float = 32.0,
        verbose: bool = False,
    ):
        self.clusters_file = str(clusters_file)
        self.tdc_to_tpc = bool(tdc_to_tpc)
        self.tdc_to_tpc_factor = float(tdc_to_tpc_factor)
        self.verbose = bool(verbose)

    def load(
        self,
        *,
        build_matches: bool = True,
        require_nonnegative_match_id: bool = True,
        prefer_main_cluster: bool = True,
        prefer_higher_charge: bool = True,
    ) -> Dict[str, Any]:
        """
        Returns a dict with:
            clusters: {"U":[ClusterItem...], "V":[...], "X":[...]}
            matches_by_id: {(event_id, match_id): MatchedGroup}  (if build_matches)
            triplets: [MatchedTriplet...]                         (if build_matches)
        """
        clusters_by_plane: Dict[str, List[ClusterItem]] = {"U": [], "V": [], "X": []}

        with uproot.open(self.clusters_file) as f:
            for plane in ("U", "V", "X"):
                tree = self._find_tree(f, plane)
                if tree is None:
                    if self.verbose:
                        print(f"[load] No tree found for plane {plane}")
                    continue
                clusters_by_plane[plane] = self._read_plane_clusters(tree, plane)

        out: Dict[str, Any] = {"clusters": clusters_by_plane}

        if not build_matches:
            return out

        matches_by_key = self._build_match_groups(
            clusters_by_plane,
            require_nonnegative_match_id=require_nonnegative_match_id,
            prefer_main_cluster=prefer_main_cluster,
            prefer_higher_charge=prefer_higher_charge,
        )

        triplets = self._build_triplets(matches_by_key)

        out["matches_by_id"] = matches_by_key
        out["triplets"] = triplets
        return out

    def _find_tree(self, f: uproot.ReadOnlyDirectory, plane: str):
        """
        Try matched_clusters layout first:
            clusters/clusters_tree_{plane}
        Then fallbacks:
            clusters_tree_{plane}
            discarded/clusters_tree_{plane}  (rare for matched, but harmless)
        """
        candidates = [
            f"clusters/clusters_tree_{plane}",
            f"clusters_tree_{plane}",
            f"discarded/clusters_tree_{plane}",
        ]
        for name in candidates:
            if name in f:
                return f[name]
        return None

    def _read_plane_clusters(self, tree, plane: str) -> List[ClusterItem]:
        """
        Read a plane tree into ClusterItem objects (including TP arrays).
        This merges your analyzer's branch needs + viewer's TP payload.
        """
        # Keep branch list minimal but sufficient.
        branches = [
            "event",
            "cluster_id",
            "n_tps",
            "true_label",
            "is_es_interaction",
            "true_neutrino_energy",
            "true_particle_energy",
            "total_charge",
            "total_energy",
            "marley_tp_fraction",
            "is_main_cluster",
            "match_id",
            "match_type",
            # TP payload
            "tp_detector",
            "tp_detector_channel",
            "tp_time_start",
            "tp_samples_over_threshold",
            "tp_samples_to_peak",
            "tp_adc_peak",
            "tp_adc_integral",
            # Some truth inputs
            "true_pos_x",
            "true_pos_y",
            "true_pos_z",

            "true_mom_x",
            "true_mom_y",
            "true_mom_z"
        ]

        # Some files might not have every branch; uproot will throw if missing.
        # So we filter to those actually present.
        available = set(tree.keys())
        use_branches = [b for b in branches if b in available]

        if self.verbose:
            missing = [b for b in branches if b not in available]
            if missing:
                print(f"[read_plane_clusters] plane={plane} missing branches: {missing}")

        arrays = tree.arrays(use_branches, library="np")
        n_entries = len(arrays["event"]) if "event" in arrays else 0

        items: List[ClusterItem] = []

        for i in range(n_entries):
            item = ClusterItem()
            item.plane = plane

            item.event_id = int(arrays["event"][i]) if "event" in arrays else -1
            item.cluster_id = int(arrays["cluster_id"][i]) if "cluster_id" in arrays else -1
            item.n_tps = int(arrays["n_tps"][i]) if "n_tps" in arrays else 0

            # labels can be bytes
            tl = arrays["true_label"][i] if "true_label" in arrays else ""
            if isinstance(tl, bytes):
                tl = tl.decode("utf-8", errors="replace")
            item.true_label = str(tl)

            item.is_es_interaction = bool(arrays["is_es_interaction"][i]) if "is_es_interaction" in arrays else False
            item.true_neutrino_energy = float(arrays["true_neutrino_energy"][i]) if "true_neutrino_energy" in arrays else 0.0
            item.true_particle_energy = float(arrays["true_particle_energy"][i]) if "true_particle_energy" in arrays else 0.0
            item.total_charge = float(arrays["total_charge"][i]) if "total_charge" in arrays else 0.0
            item.total_energy = float(arrays["total_energy"][i]) if "total_energy" in arrays else 0.0
            item.marley_tp_fraction = float(arrays["marley_tp_fraction"][i]) if "marley_tp_fraction" in arrays else 0.0
            item.is_main_cluster = bool(arrays["is_main_cluster"][i]) if "is_main_cluster" in arrays else False

            item.match_id = int(arrays["match_id"][i]) if "match_id" in arrays else -1
            item.match_type = int(arrays["match_type"][i]) if "match_type" in arrays else -1

            # TP arrays
            # Get APA index from collection view
            item.apa = int(arrays["tp_detector"][i][-1]) if "tp_detector" in arrays else -1
            item.ch = list(arrays["tp_detector_channel"][i]) if "tp_detector_channel" in arrays else []
            tstart = arrays["tp_time_start"][i] if "tp_time_start" in arrays else []
            if self.tdc_to_tpc and len(tstart) > 0:
                item.tstart = [float(ts) / self.tdc_to_tpc_factor for ts in tstart]
            else:
                item.tstart = [float(ts) for ts in tstart] if len(tstart) > 0 else []

            # Truth info
            item.true_x = float(arrays["true_pos_x"][i]) if "true_pos_x" in arrays else -999
            item.true_y = float(arrays["true_pos_y"][i]) if "true_pos_y" in arrays else -999
            item.true_z = float(arrays["true_pos_z"][i]) if "true_pos_z" in arrays else -999

            item.true_mom_x = float(arrays["true_mom_x"][i]) if "true_mom_x" in arrays else -999
            item.true_mom_y = float(arrays["true_mom_y"][i]) if "true_mom_y" in arrays else -999
            item.true_mom_z = float(arrays["true_mom_z"][i]) if "true_mom_z" in arrays else -999

            item.sot = list(arrays["tp_samples_over_threshold"][i]) if "tp_samples_over_threshold" in arrays else []
            item.stopeak = list(arrays["tp_samples_to_peak"][i]) if "tp_samples_to_peak" in arrays else []
            item.adc_peak = [float(x) for x in arrays["tp_adc_peak"][i]] if "tp_adc_peak" in arrays else []
            item.adc_integral = [float(x) for x in arrays["tp_adc_integral"][i]] if "tp_adc_integral" in arrays else []

            items.append(item)

        if self.verbose:
            print(f"[read_plane_clusters] plane={plane} loaded {len(items)} clusters")

        return items

    def _build_match_groups(
        self,
        clusters_by_plane: Dict[str, List[ClusterItem]],
        *,
        require_nonnegative_match_id: bool,
        prefer_main_cluster: bool,
        prefer_higher_charge: bool,
    ) -> Dict[Tuple[int, int], MatchedGroup]:
        """
        Build MatchedGroup objects keyed by (event_id, match_id).
        Resolves collisions (multiple clusters same plane same key) by a simple policy:
            1) prefer is_main_cluster (if enabled)
            2) then prefer higher total_charge (if enabled)
            3) else keep first
        """
        groups: Dict[Tuple[int, int], MatchedGroup] = {}

        def better(a: ClusterItem, b: ClusterItem) -> ClusterItem:
            # return the preferred one between a and b
            if prefer_main_cluster and (a.is_main_cluster != b.is_main_cluster):
                return a if a.is_main_cluster else b
            if prefer_higher_charge and (a.total_charge != b.total_charge):
                return a if a.total_charge > b.total_charge else b
            # tie-break: keep a (earlier)
            return a

        for plane, clusters in clusters_by_plane.items():
            for c in clusters:
                if require_nonnegative_match_id and c.match_id < 0:
                    continue

                key = (c.event_id, c.match_id)
                if key not in groups:
                    groups[key] = MatchedGroup(event_id=c.event_id, match_id=c.match_id)

                g = groups[key]
                if plane not in g.clusters:
                    g.clusters[plane] = c
                else:
                    g.clusters[plane] = better(g.clusters[plane], c)

        if self.verbose:
            n_groups = len(groups)
            n_trip = sum(1 for g in groups.values() if g.is_triplet())
            print(f"[build_match_groups] groups={n_groups}, triplets={n_trip}")

        return groups

    def _build_triplets(self, groups: Dict[Tuple[int, int], MatchedGroup]) -> List[MatchedTriplet]:
        triplets: List[MatchedTriplet] = []
        for (_, _), g in groups.items():
            if not g.is_triplet():
                continue
            triplets.append(
                MatchedTriplet(
                    event_id=g.event_id,
                    match_id=g.match_id,
                    U=g.clusters["U"],
                    V=g.clusters["V"],
                    X=g.clusters["X"],
                )
            )
        return triplets


# -----------------------------
# Optional CLI (no plotting)
# -----------------------------

def _iter_triplets_summary(triplets: Iterable[MatchedTriplet], n: int = 10) -> List[str]:
    out = []
    for i, t in enumerate(triplets):
        if i >= n:
            break
        out.append(
            f"[{i}] event={t.event_id} match_id={t.match_id} "
            f"U(n_tps={t.U.n_tps}, charge={t.U.total_charge:.0f}) "
            f"V(n_tps={t.V.n_tps}, charge={t.V.total_charge:.0f}) "
            f"X(n_tps={t.X.n_tps}, charge={t.X.total_charge:.0f})"
        )
    return out


def main():
    parser = argparse.ArgumentParser(
        description="Load matched-cluster objects from a matched_clusters ROOT file (no plotting)."
    )
    parser.add_argument("--clusters-file", help="Input matched_clusters ROOT file")
    parser.add_argument("-j", "--json", help="Optional JSON config (can provide clusters_file)")
    parser.add_argument("--no-matches", action="store_true", help="Only load clusters, do not build match groups")
    parser.add_argument("--keep-negative", action="store_true", help="Do not require match_id >= 0")
    parser.add_argument("--no-tdc-to-tpc", action="store_true", help="Do not convert tp_time_start from TDC to TPC ticks")
    parser.add_argument("--tdc-factor", type=float, default=32.0, help="TDC->TPC conversion factor (default 32)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    if args.json:
        with open(args.json, "r") as f:
            cfg = json.load(f)
        if not args.clusters_file and "clusters_file" in cfg:
            args.clusters_file = cfg["clusters_file"]

    if not args.clusters_file:
        raise SystemExit("ERROR: provide --clusters-file (or JSON with clusters_file).")

    loader = MatchedClustersLoader(
        args.clusters_file,
        tdc_to_tpc=not args.no_tdc_to_tpc,
        tdc_to_tpc_factor=args.tdc_factor,
        verbose=args.verbose,
    )

    result = loader.load(
        build_matches=not args.no_matches,
        require_nonnegative_match_id=not args.keep_negative,
    )

    # Print a lightweight summary
    clusters = result["clusters"]
    print(f"Loaded file: {args.clusters_file}")
    for plane in ("U", "V", "X"):
        print(f"  {plane}: {len(clusters.get(plane, []))} clusters")

    if not args.no_matches:
        groups = result["matches_by_id"]
        triplets = result["triplets"]
        print(f"Built match groups: {len(groups)} (keyed by (event, match_id))")
        print(f"Triplets (U+V+X present): {len(triplets)}")
        for line in _iter_triplets_summary(triplets, n=10):
            print("  " + line)


if __name__ == "__main__":
    main()
