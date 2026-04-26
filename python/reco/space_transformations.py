import numpy as np

### GEOMETRY CONSTANTS ---- FIGURE OUT HOW TO LOAD THESE LATER ###
#----------------------------------------------------------------#

# Drift velocity in cm/tick
DRIFT_VELOCITY = 0.0805

# APA plane angles (+/- for U/V)
APA_ANGLE_DEG = 54.3

# Wire pitches, measured in the direction perpendicular to the collection wires
X_WIRE_PITCH = 0.479
U_WIRE_PITCH = 0.4669 / np.sin(np.deg2rad(APA_ANGLE_DEG))
V_WIRE_PITCH = 0.4669 / np.sin(np.deg2rad(APA_ANGLE_DEG))

# APA sizes and offsets (cm)
APA_HEIGHT = 598.4
APA_LENGTH = 230.0
PLANE_SPACING = 0.47
APA_OFFSET = 2.4

# Channel numbers
CHANNELS_PER_APA = 2560
INDUCTION_CHANNELS = 800
COLLECTION_CHANNELS = 960

U_PLANE_START = 0
V_PLANE_START = 800
X_PLANE_START = 1600
X_PLANE_FLIP = 2080

# Conversion factors (maybe not needed?)
X_ADC_TO_MEV = 3600.0
U_ADC_TO_MEV = 900.0
V_ADC_TO_MEV = 900.0

#----------------------------------------------------------------#
#--------------- Wire IDs to physical coordinates ---------------#
#----------------------------------------------------------------#

def X_wire_to_z(x_wire_id: int, apa_id: int) -> float:
    """
    Convert a collection-plane (X plane) wire/channel ID to the global z coordinate.

    Parameters
    ----------
    x_wire_id:
        Channel/wire identifier that must belong to the collection (X) plane.
    apa_id:
        APA identifier. The code assumes APAs are arranged in pairs along z, with
        `int(apa_id/2)` selecting which APA pair sets the global z offset.

    Returns
    -------
    float
        Global z coordinate corresponding to the provided X-plane wire/channel.

    Raises
    ------
    ValueError
        If `x_wire_id` is outside the collection-plane channel range.

    Notes
    -----
    This uses the geometry constants:
      - X_PLANE_START, CHANNELS_PER_APA, COLLECTION_CHANNELS
      - APA_LENGTH, APA_OFFSET, X_WIRE_PITCH
    """
    if (x_wire_id < X_PLANE_START) or (x_wire_id >= CHANNELS_PER_APA):
        raise ValueError("wire id does not correspond to collection plane")

    z_apa_offset = int(apa_id / 2) * (APA_LENGTH + APA_OFFSET)
    z_channel_offset = ((x_wire_id - X_PLANE_START) % (COLLECTION_CHANNELS / 2)) * X_WIRE_PITCH
    # The + X_WIRE_PITCH here appears to define the first wire center at one pitch from the origin.
    return X_WIRE_PITCH + z_apa_offset + z_channel_offset


def X_wire_to_X_sign(x_wire_id: int, apa_id: int) -> int:
    """
    Determine the X-side sign (+1 or -1) of a collection-plane wire within an APA.

    Parameters
    ----------
    x_wire_id:
        X-plane channel/wire identifier.
    apa_id:
        APA identifier.

    Returns
    -------
    int
        -1 if the wire is on the "negative-X side" of the APA, +1 otherwise.

    Notes
    -----
    `apa_id` parity may encode "face", but it is not used here (the computed
    `is_top_apa` was unused in the original code and has been removed).
    """
    return -1 if x_wire_id < X_PLANE_FLIP else +1


def UX_wires_to_yz(
    u_wire_id: int,
    x_wire_id: int,
    apa_id: int,
    error_margin: float = 1e-4,
    forced_face: int | None = None,
) -> tuple[float, float]:
    """
    Compute (y, z) for the intersection of a U-plane wire with an X-plane wire.

    The X wire provides the global z coordinate. The U wire plus APA face and X-side
    determine the corresponding y coordinate, including wrap-around logic.

    Parameters
    ----------
    u_wire_id:
        U-plane wire ID. Must satisfy:
            U_PLANE_START <= u_wire_id < U_PLANE_START + INDUCTION_CHANNELS
    x_wire_id:
        X-plane wire/channel ID (collection plane).
    apa_id:
        APA identifier. `apa_id % 2` is treated as the APA face unless `forced_face`
        is provided.
    error_margin:
        Small tolerance used when comparing z to wrap/turn boundaries to avoid
        numerical edge issues.
    forced_face:
        If provided, overrides the face used in the calculation (0 or 1).

    Returns
    -------
    (float, float)
        Tuple of (y_final, z_global).

    Raises
    ------
    ValueError
        If wire IDs are out of range, local z is outside expected APA bounds, or
        face/sign are invalid.

    Notes
    -----
    Geometry constants used:
      - APA_LENGTH, APA_OFFSET, APA_ANGLE_DEG, APA_HEIGHT
      - U_PLANE_START, INDUCTION_CHANNELS, U_WIRE_PITCH
      - plus X-plane helpers.
    """
    z_global = X_wire_to_z(x_wire_id, apa_id)
    x_sign = X_wire_to_X_sign(x_wire_id, apa_id)

    # Convert global z to local APA z coordinate (within the APA pair segment).
    z0 = z_global - int(apa_id / 2) * (APA_LENGTH + APA_OFFSET)

    if not (U_PLANE_START <= u_wire_id < U_PLANE_START + INDUCTION_CHANNELS):
        raise ValueError("u_wire_id must be in [0, 799] for U plane")

    if not (0.0 <= z0 <= APA_LENGTH + error_margin):
        raise ValueError("local z value is outside expected APA local range")

    angular_coeff = float(np.tan(np.deg2rad(APA_ANGLE_DEG)))
    face = int(forced_face) if forced_face is not None else (apa_id % 2)

    half = INDUCTION_CHANNELS / 2

    if face == 0:
        if x_sign < 0:
            if u_wire_id < half:
                distance_before_turn = u_wire_id * U_WIRE_PITCH
                if z0 > distance_before_turn + error_margin:
                    y = (distance_before_turn + 2 * APA_LENGTH - z0) * angular_coeff
                else:
                    y = (distance_before_turn - z0) * angular_coeff
            else:
                y = (APA_LENGTH + (u_wire_id - half) * U_WIRE_PITCH - z0) * angular_coeff

        elif x_sign > 0:
            if u_wire_id >= half:
                boundary = (INDUCTION_CHANNELS - 1 - u_wire_id) * U_WIRE_PITCH
                if z0 < boundary - error_margin:
                    distance_before_turn = (u_wire_id - half) * U_WIRE_PITCH
                    y = (distance_before_turn + APA_LENGTH + z0) * angular_coeff
                else:
                    y = (z0 - boundary) * angular_coeff
            else:
                y = (z0 + u_wire_id * U_WIRE_PITCH) * angular_coeff
        else:
            raise ValueError("x_sign must be non-zero")

    elif face == 1:
        if x_sign < 0:
            if u_wire_id < half:
                boundary = (half - 1 - u_wire_id) * U_WIRE_PITCH
                if z0 < boundary - error_margin:
                    distance_before_turn = u_wire_id * U_WIRE_PITCH
                    y = (distance_before_turn + APA_LENGTH + z0) * angular_coeff
                else:
                    y = (z0 - boundary) * angular_coeff
            else:
                y = (z0 + (u_wire_id - half) * U_WIRE_PITCH) * angular_coeff

        elif x_sign > 0:
            if u_wire_id >= half:
                distance_before_turn = (u_wire_id - half) * U_WIRE_PITCH
                if z0 > distance_before_turn + error_margin:
                    y = (distance_before_turn + 2 * APA_LENGTH - z0) * angular_coeff
                else:
                    y = (distance_before_turn - z0) * angular_coeff
            else:
                y = (APA_LENGTH - z0 + u_wire_id * U_WIRE_PITCH) * angular_coeff
        else:
            raise ValueError("x_sign must be non-zero")
    else:
        raise ValueError("Invalid APA face (expected 0 or 1)")

    # Map to final y depending on face orientation.
    y_final = (y - APA_HEIGHT) if face == 0 else (APA_HEIGHT - y)
    return y_final, z_global


def VX_wires_to_yz(
    v_wire_id: int,
    x_wire_id: int,
    apa_id: int,
    error_margin: float = 1e-4,
    forced_face: int | None = None,
) -> tuple[float, float]:
    """
    Compute (y, z) for the intersection of a V-plane wire with an X-plane wire.

    The X wire provides the global z coordinate. The V wire plus APA face and X-side
    determine the corresponding y coordinate, including wrap-around logic.

    Parameters
    ----------
    v_wire_id:
        V-plane wire ID. Must satisfy:
            V_PLANE_START <= v_wire_id < V_PLANE_START + INDUCTION_CHANNELS
    x_wire_id:
        X-plane wire/channel ID (collection plane).
    apa_id:
        APA identifier. `apa_id % 2` is treated as the APA face unless `forced_face`
        is provided.
    error_margin:
        Small tolerance used when comparing z to wrap/turn boundaries to avoid
        numerical edge issues.
    forced_face:
        If provided, overrides the face used in the calculation (0 or 1).

    Returns
    -------
    (float, float)
        Tuple of (y_final, z_global).

    Raises
    ------
    ValueError
        If wire IDs are out of range, local z is outside expected APA bounds, or
        face/sign are invalid.

    Notes
    -----
    Geometry constants used:
      - APA_LENGTH, APA_OFFSET, APA_ANGLE_DEG, APA_HEIGHT
      - V_PLANE_START, INDUCTION_CHANNELS, V_WIRE_PITCH
      - plus X-plane helpers.
    """
    z_global = X_wire_to_z(x_wire_id, apa_id)
    x_sign = X_wire_to_X_sign(x_wire_id, apa_id)

    z0 = z_global - int(apa_id / 2) * (APA_LENGTH + APA_OFFSET)

    if not (V_PLANE_START <= v_wire_id < V_PLANE_START + INDUCTION_CHANNELS):
        raise ValueError("v_wire_id must be in [800, 1599]")

    if not (0.0 <= z0 <= APA_LENGTH + error_margin):
        raise ValueError("local z outside APA range")

    angular_coeff = float(np.tan(np.deg2rad(APA_ANGLE_DEG)))
    face = int(forced_face) if forced_face is not None else (apa_id % 2)

    v0 = V_PLANE_START
    v_mid = V_PLANE_START + (INDUCTION_CHANNELS // 2)
    v_end = V_PLANE_START + INDUCTION_CHANNELS - 1

    if face == 0:
        if x_sign < 0:
            if v_wire_id < v_mid:
                boundary = (v_mid - 1 - v_wire_id) * V_WIRE_PITCH
                if z0 < boundary - error_margin:
                    until_turn = (v_wire_id - v0) * V_WIRE_PITCH
                    y = (until_turn + APA_LENGTH + z0) * angular_coeff
                else:
                    y = (z0 - boundary) * angular_coeff
            else:
                y = (z0 + (v_wire_id - v_mid) * V_WIRE_PITCH) * angular_coeff

        elif x_sign > 0:
            if v_wire_id >= v_mid:
                boundary = (v_wire_id - v_mid) * V_WIRE_PITCH
                if z0 > boundary + error_margin:
                    until_turn = (v_wire_id - v_mid) * V_WIRE_PITCH
                    y = (until_turn + 2 * APA_LENGTH - z0) * angular_coeff
                else:
                    y = (boundary - z0) * angular_coeff
            else:
                y = (APA_LENGTH - z0 + (v_wire_id - v0) * V_WIRE_PITCH) * angular_coeff
        else:
            raise ValueError("x_sign must be non-zero")

    elif face == 1:
        if x_sign < 0:
            if v_wire_id < v_mid:
                boundary = (v_wire_id - v0) * V_WIRE_PITCH
                if z0 > boundary + error_margin:
                    until_turn = boundary
                    y = (until_turn + 2 * APA_LENGTH - z0) * angular_coeff
                else:
                    y = (boundary - z0) * angular_coeff
            else:
                y = (APA_LENGTH - z0 + (v_wire_id - v_mid) * V_WIRE_PITCH) * angular_coeff

        elif x_sign > 0:
            if v_wire_id >= v_mid:
                boundary = (v_end - v_wire_id) * V_WIRE_PITCH
                if z0 < boundary - error_margin:
                    until_turn = (v_wire_id - v_mid) * V_WIRE_PITCH
                    y = (until_turn + APA_LENGTH + z0) * angular_coeff
                else:
                    y = (z0 - boundary) * angular_coeff
            else:
                y = (z0 + (v_wire_id - v0) * V_WIRE_PITCH) * angular_coeff
        else:
            raise ValueError("x_sign must be non-zero")
    else:
        raise ValueError("Invalid APA face (expected 0 or 1)")

    y_final = (y - APA_HEIGHT) if face == 0 else (APA_HEIGHT - y)
    return y_final, z_global


def best_UVX_match_to_yz(
    u_wire_id: int,
    v_wire_id: int,
    x_wire_id: int,
    apa_id: int,
    error_margin: float = 1e-4,
):
    """
    Find the closest (y, z) pair from U+X and V+X intersections over all
    forced-face combinations.

    Tries all (u_face, v_face) in {0,1} × {0,1} and returns the pair with
    minimal separation in the (y, z) plane.

    Returns
    -------
    tuple[np.ndarray, np.ndarray]
        (u_yz, v_yz), where each is a shape-(2,) array [y, z].
    """
    best_dist = None
    best_u = None
    best_v = None

    for u_face in (0, 1):
        for v_face in (0, 1):

            try:
                u = np.asarray(
                    UX_wires_to_yz(
                        u_wire_id,
                        x_wire_id,
                        apa_id,
                        error_margin=error_margin,
                        forced_face=u_face,
                    ),
                    dtype=float,
                )

                v = np.asarray(
                    VX_wires_to_yz(
                        v_wire_id,
                        x_wire_id,
                        apa_id,
                        error_margin=error_margin,
                        forced_face=v_face,
                    ),
                    dtype=float,
                )

            except Exception:
                continue

            dist = np.linalg.norm(u - v)

            if best_dist is None or dist < best_dist:
                best_dist = dist
                best_u = u
                best_v = v

    if best_u is None:
        raise ValueError("No valid U/V face combination found")

    return best_u, best_v


#----------------------------------------------------------------#
#--------------- Physical coordinates to wire IDs ---------------#
#----------------------------------------------------------------#

HALF_X = COLLECTION_CHANNELS // 2  # 480


def build_uv_sorted_luts_one_apa(error_margin: float = 1e-4):
    """
   Build sorted lookup tables that map (face, x-side, local-X index) -> y(wire).

    The goal is fast inverse mapping from (x, y, z) to (U, V, X) wire/channel IDs.
    For a fixed X wire (hence fixed z), each induction wire (U/V) maps to a single
    y at that z. We precompute these y-values for one local APA segment and sort
    them so that inverse lookup becomes a binary search (`np.searchsorted`).

    Assumptions
    -----------
    - All APAs share the same *local* (y,z) wire geometry.
    - The only difference between APAs is a global z offset handled elsewhere
      (e.g. via `z_to_X_wire`).
    - Face is encoded as 0/1 and is passed through `forced_face` in forward maps.
    - X-side is encoded as:
        xside=0 for x_wire_id < X_PLANE_FLIP  (negative-X side)
        xside=1 for x_wire_id >= X_PLANE_FLIP (positive-X side)

    Parameters
    ----------
    error_margin:
        Numerical tolerance passed to the forward mapping functions to avoid
        turn/wrap boundary issues.

    Returns
    -------
    (U_y_sorted, U_u_sorted, V_y_sorted, V_v_sorted)
        U_y_sorted : float32, shape (2, 2, HALF_X, 800)
            Sorted y-values for U wires at each (face, xside, k).
        U_u_sorted : int16, shape (2, 2, HALF_X, 800)
            Indices 0..799 corresponding to U_y_sorted entries.
        V_y_sorted : float32, shape (2, 2, HALF_X, 800)
            Sorted y-values for V wires at each (face, xside, k).
        V_v_sorted : int16, shape (2, 2, HALF_X, 800)
            Indices 0..799 corresponding to V_y_sorted entries; add V_PLANE_START
            to get the actual V wire ID.
    """
    # Raw y tables (unsorted). NaN marks invalid/non-real intersections.
    u_y = np.full((2, 2, HALF_X, INDUCTION_CHANNELS), np.nan, dtype=np.float32)
    v_y = np.full((2, 2, HALF_X, INDUCTION_CHANNELS), np.nan, dtype=np.float32)

    # Build for a single APA-pair segment (apa_pair=0) so apa_id == face.
    for face in (0, 1):
        apa_id = face

        for xside in (0, 1):
            # Choose a representative X wire ID base for the desired x-side.
            base_x = X_PLANE_START + (0 if xside == 0 else HALF_X)

            for k in range(HALF_X):
                x_wire_id = base_x + k

                # U plane: u indices are 0..799.
                for u in range(INDUCTION_CHANNELS):
                    try:
                        y, _ = UX_wires_to_yz(
                            u,
                            x_wire_id,
                            apa_id,
                            error_margin=error_margin,
                            forced_face=face,
                        )
                        u_y[face, xside, k, u] = y
                    except Exception:
                        # Keep NaN for invalid solutions.
                        pass

                # V plane: store v_local = v_wire_id - V_PLANE_START in 0..799.
                for v_local in range(INDUCTION_CHANNELS):
                    v_wire_id = V_PLANE_START + v_local
                    try:
                        y, _ = VX_wires_to_yz(
                            v_wire_id,
                            x_wire_id,
                            apa_id,
                            error_margin=error_margin,
                            forced_face=face,
                        )
                        v_y[face, xside, k, v_local] = y
                    except Exception:
                        pass

    def _sort_last_axis(y_table: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        """
        Sort along the wire axis (last axis). Invalid entries (NaN) are pushed to
        the end by replacing them with +inf before sorting.
        """
        y_finite = np.where(np.isfinite(y_table), y_table, np.float32(np.inf))
        order = np.argsort(y_finite, axis=-1)
        y_sorted = np.take_along_axis(y_finite, order, axis=-1).astype(
            np.float32, copy=False
        )
        idx_sorted = order.astype(np.int16, copy=False)
        return y_sorted, idx_sorted

    u_y_sorted, u_u_sorted = _sort_last_axis(u_y)
    v_y_sorted, v_v_sorted = _sort_last_axis(v_y)

    return u_y_sorted, u_u_sorted, v_y_sorted, v_v_sorted


def _face_from_x(x_global: float, flip: bool = False) -> int:
    """
    Map signed global x to an APA face index (0 or 1).

    Parameters
    ----------
    x_global:
        Signed global drift coordinate.
    flip:
        If True, swap the mapping (useful if the detector convention is opposite).

    Returns
    -------
    int
        Face index (0 or 1).
    """
    face = 0 if x_global < 0 else 1
    return 1 - face if flip else face


def _xside_from_x_wire(x_wire_id: int) -> int:
    """
    Determine which X-side half of the APA an X wire belongs to.

    Returns
    -------
    int
        0 for negative-X side (x_wire_id < X_PLANE_FLIP),
        1 for positive-X side (x_wire_id >= X_PLANE_FLIP).
    """
    return 0 if x_wire_id < X_PLANE_FLIP else 1


def _nearest_sorted(y_sorted: np.ndarray, idx_sorted: np.ndarray, y: float) -> int:
    """
    Return the original index whose sorted y-value is closest to the query y.

    This is a O(log N) lookup via binary search plus a 2-point comparison.

    Parameters
    ----------
    y_sorted:
        Sorted y-values, shape (N,).
    idx_sorted:
        Original indices corresponding to y_sorted, shape (N,).
    y:
        Query y-value.

    Returns
    -------
    int
        Original index (e.g. u in [0..799] or v_local in [0..799]).
    """
    j = int(np.searchsorted(y_sorted, y, side="left"))

    if j <= 0:
        return int(idx_sorted[0])
    if j >= y_sorted.shape[0]:
        return int(idx_sorted[-1])

    y0 = float(y_sorted[j - 1])
    y1 = float(y_sorted[j])
    return int(idx_sorted[j - 1] if abs(y - y0) <= abs(y - y1) else idx_sorted[j])


def z_to_X_wire(
    x_global: float,
    z_global: float,
    error_margin: float = 1e-3,
) -> int:
    """
    Convert global (x, z) to the collection-plane (X) wire/channel ID.

    This inverts the forward mapping:

        z_global = X_WIRE_PITCH + apa_pair * (APA_LENGTH + APA_OFFSET) + k * X_WIRE_PITCH

    where:
        apa_pair = int(apa_id / 2)
        k        = (x_wire_id - X_PLANE_START) % (COLLECTION_CHANNELS/2)

    The sign of x_global selects which half of the collection plane is used:
        x_global < 0  -> x_wire_id in [X_PLANE_START .. X_PLANE_START+HALF_X-1]
        x_global >= 0 -> x_wire_id in [X_PLANE_START+HALF_X .. X_PLANE_START+2*HALF_X-1]

    Parameters
    ----------
    x_global:
        Signed global drift coordinate; used only to choose the X-side half.
    z_global:
        Global z coordinate in cm.
    error_margin:
        Small tolerance used when inferring the APA-pair index.

    Returns
    -------
    int
        X-plane channel/wire ID.
    """
    seg_len = APA_LENGTH + APA_OFFSET

    # Infer the APA-pair index (apa_id//2) along global z.
    apa_pair = int(np.floor((z_global - X_WIRE_PITCH) / seg_len + error_margin))

    # Local z inside the segment.
    z0 = z_global - apa_pair * seg_len

    # Invert k from z0.
    k = int(np.round((z0 - X_WIRE_PITCH) / X_WIRE_PITCH))
    if not (0 <= k < HALF_X):
        raise ValueError(f"Derived k={k} out of range for z0={z0}")

    # Choose side based on x_global.
    if x_global < 0:
        x_wire_id = X_PLANE_START + k
    else:
        x_wire_id = X_PLANE_START + HALF_X + k

    return int(x_wire_id)


def xyz_to_UVX_wires_lut(
    x_global: float,
    y_global: float,
    z_global: float,
    u_y_sorted: np.ndarray,
    u_u_sorted: np.ndarray,
    v_y_sorted: np.ndarray,
    v_v_sorted: np.ndarray,
    *,
    error_margin: float = 1e-3,
    y_offset: float = 0.0,
    flip_face_from_x: bool | None = None,
) -> tuple[int, int, int, int]:
    """
    Map a single physical (x, y, z) point to (U wire, V wire, X wire), using LUTs.

    This function supports two modes for choosing APA face:

    1) Automatic (default): try both faces (0 and 1) and select the one that
       minimizes a forward-model y residual.
    2) Forced mapping: if `flip_face_from_x` is not None, choose face directly
       from the sign of x_global with an optional flip:
          face = 0 if x_global < 0 else 1
          face = 1 - face  (if flip_face_from_x is True)

    Parameters
    ----------
    x_global, y_global, z_global:
        Physical coordinates. `x_global` is used to choose the X-side half in
        `z_to_X_wire`. `y_global` is used to select the nearest induction wires.
    u_y_sorted, u_u_sorted, v_y_sorted, v_v_sorted:
        Sorted LUTs returned by `build_uv_sorted_luts_one_apa()`.
    error_margin:
        Numerical tolerance used in `z_to_X_wire` and the forward checks.
    y_offset:
        Optional constant offset added to y_global before lookup, to match the
        y-coordinate frame used by the forward geometry.
    flip_face_from_x:
        If None (default), select face by trying both and minimizing residual.
        If True/False, select face from sign(x_global) and optionally flip it.

    Returns
    -------
    (u_wire_id, v_wire_id, x_wire_id, face)
        U-plane wire index in [0..799], V-plane wire ID in [800..1599],
        X-plane wire ID, and selected face (0 or 1).
    """
    # 1) X wire and local bin
    x_wire = z_to_X_wire(x_global, z_global, error_margin=error_margin)
    xside = _xside_from_x_wire(x_wire)
    k = int((x_wire - X_PLANE_START) % HALF_X)

    # 2) infer apa_pair for global z -> apa_id construction
    seg_len = APA_LENGTH + APA_OFFSET
    apa_pair = int(np.floor((z_global - X_WIRE_PITCH) / seg_len + error_margin))

    # 3) query y in the forward-geometry frame (optional constant shift)
    yq = y_global + float(y_offset)

    def _eval_face(face: int) -> tuple[float, int, int]:
        """Return (err, u, v) for a given face."""
        u = _nearest_sorted(u_y_sorted[face, xside, k], u_u_sorted[face, xside, k], yq)
        v_local = _nearest_sorted(v_y_sorted[face, xside, k], v_v_sorted[face, xside, k], yq)
        v = int(V_PLANE_START + v_local)

        apa_id = 2 * apa_pair + face
        yu2, _ = UX_wires_to_yz(
            u, x_wire, apa_id, error_margin=error_margin, forced_face=face
        )
        yv2, _ = VX_wires_to_yz(
            v, x_wire, apa_id, error_margin=error_margin, forced_face=face
        )

        # Squared residual in y for U and V predictions.
        err = float((yu2 - yq) ** 2 + (yv2 - yq) ** 2)
        return err, int(u), int(v)

    # Face selection:
    if flip_face_from_x is not None:
        face = 0 if x_global < 0 else 1
        if flip_face_from_x:
            face = 1 - face
        err, u, v = _eval_face(face)
    else:
        err0, u0, v0 = _eval_face(0)
        err1, u1, v1 = _eval_face(1)
        if err0 <= err1:
            face, u, v = 0, u0, v0
        else:
            face, u, v = 1, u1, v1

    return int(u), int(v), int(x_wire) # , int(face)



def debug_v_choice(
    x_global: float,
    y_reco: float,
    z_reco: float,
    v_a: int,
    v_b: int,
    *,
    error_margin: float = 1e-3,
    flip_face_from_x: bool = False,
) -> None:
    """
    Compare two candidate V wires by forward-mapping them to y at the event z.

    This is a diagnostic helper to verify that the chosen V wire is truly the
    nearest (in |dy|) to the provided reco point under the forward model.

    Parameters
    ----------
    x_global:
        Signed global drift coordinate (affects face choice and X wire side).
    y_reco, z_reco:
        Reconstructed (y, z) point to compare against.
    v_a, v_b:
        Two V wire IDs to compare.
    error_margin:
        Numerical tolerance used when inferring the APA-pair index and when calling
        the forward map.
    flip_face_from_x:
        If True, swap the x->face mapping.
    """
    x_wire = z_to_X_wire(x_global, z_reco, error_margin=error_margin)

    seg_len = APA_LENGTH + APA_OFFSET
    apa_pair = int(np.floor((z_reco - X_WIRE_PITCH) / seg_len + error_margin))
    face = _face_from_x(x_global, flip=flip_face_from_x)
    apa_id = 2 * apa_pair + face

    y_a, z_a = VX_wires_to_yz(
        v_a,
        x_wire,
        apa_id,
        error_margin=error_margin,
        forced_face=face,
    )
    y_b, z_b = VX_wires_to_yz(
        v_b,
        x_wire,
        apa_id,
        error_margin=error_margin,
        forced_face=face,
    )

    print("x_wire =", x_wire, "apa_id =", apa_id, "face =", face, "apa_pair =", apa_pair)
    print("reco   : y =", y_reco, "z =", z_reco)
    print(
        f"V={v_a}: y={y_a:.6f} z={z_a:.6f}  "
        f"|dy|={abs(y_a - y_reco):.6f}  |dz|={abs(z_a - z_reco):.6f}"
    )
    print(
        f"V={v_b}: y={y_b:.6f} z={z_b:.6f}  "
        f"|dy|={abs(y_b - y_reco):.6f}  |dz|={abs(z_b - z_reco):.6f}"
    )
