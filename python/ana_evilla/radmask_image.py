#!/usr/bin/env python3
"""
Image-level (blob) approximation of the truth-free radiological-cluster mask.

`create_volumes.py --radmask` removes non-associated *clusters* before rendering, which
requires the cluster ROOT files.  For samples whose intermediates have been pruned
(the CT training/validation/test cats 400-621) only the rendered volume images survive,
so the same selection has to be reconstructed from the image itself.

A cluster is a set of TPs grouped within `channel_limit` channels and `tick_limit` ticks,
so in the rendered image it is a connected blob.  Labelling the blobs and applying the
same (distance, charge) rule to each blob reproduces the cluster-level mask closely;
`radmask_validate_image_mask.py` quantifies the agreement against the exact product.

Geometry conventions are taken from create_volumes.py:
  image shape (208, 1242) = (100 cm / 0.479 cm per channel, 100 cm / 0.0805 cm per tick)
  the main-cluster centre sits at pixel (n_ch/2, n_t/2).
"""

import numpy as np
from scipy import ndimage

CHANNEL_PITCH_CM = 0.479
TIME_TICK_CM = 0.0805
ADC_TO_MEV_COLLECTION = 3600.0

# Cluster TPs live within +-channel_limit channels / +-tick_limit ticks of each other;
# after pentagon rendering they are contiguous in time, so a modest dilation in the
# channel direction is enough to glue one cluster into one blob.
DEFAULT_STRUCT = np.ones((5, 3), dtype=bool)


def label_blobs(img, structure=DEFAULT_STRUCT):
    """Connected components of the nonzero pixels, after a small dilation."""
    mask = img > 0
    if not mask.any():
        return np.zeros_like(mask, dtype=np.int32), 0
    grown = ndimage.binary_dilation(mask, structure=structure)
    lab, n = ndimage.label(grown)
    lab = lab * mask  # keep the labels only on real (undilated) pixels
    return lab.astype(np.int32), n


def blob_radmask(img, radius_cm=40.0, energy_keep_mev=2.0,
                 structure=DEFAULT_STRUCT, adc_to_mev=ADC_TO_MEV_COLLECTION,
                 return_info=False):
    """Return a copy of `img` with the non-associated blobs zeroed.

    Rule (identical in spirit to select_associated_clusters):
      keep a blob if its charge-weighted centroid is within `radius_cm` of the image
      centre, or if its integrated ADC / adc_to_mev >= energy_keep_mev (<=0 disables).
      The blob holding the main track (the one containing / nearest to the centre) is
      always kept.
    """
    img = np.asarray(img, dtype=np.float32)
    n_ch, n_t = img.shape
    c_ch, c_t = n_ch / 2.0, n_t / 2.0

    lab, n = label_blobs(img, structure)
    if n == 0:
        return (img.copy(), dict(n_blobs=0, n_masked=0)) if return_info else img.copy()

    idx = np.arange(1, n + 1)
    sums = ndimage.sum(img, lab, idx)
    cen = ndimage.center_of_mass(img, lab, idx)
    cen = np.asarray(cen, dtype=float).reshape(-1, 2)
    d_ch = (cen[:, 0] - c_ch) * CHANNEL_PITCH_CM
    d_t = (cen[:, 1] - c_t) * TIME_TICK_CM
    dist = np.hypot(d_ch, d_t)

    keep = dist < radius_cm
    if energy_keep_mev is not None and energy_keep_mev > 0:
        keep |= (sums / adc_to_mev) >= energy_keep_mev

    # protect the main-track blob
    main_lab = lab[int(c_ch), int(c_t)]
    if main_lab == 0:
        main_lab = idx[int(np.argmin(dist))]
    keep[main_lab - 1] = True

    out = img.copy()
    drop = idx[~keep]
    if len(drop):
        out[np.isin(lab, drop)] = 0.0

    if return_info:
        info = dict(n_blobs=int(n), n_masked=int((~keep).sum()),
                    blob_dist_cm=dist, blob_adc=sums, blob_keep=keep)
        return out, info
    return out
