#!/usr/bin/env python3
"""
Apply the radiological-cluster mask directly to already-rendered volume images.

For samples whose cluster ROOT files have been pruned (the CT cats 400-621) the mask
cannot be applied at cluster level, so it is applied at image level with the blob
approximation of radmask_image.py.  On the dev cats, where both can be produced, the
two give pixel-identical images (see radmask_validate.py).

Writes a NEW product folder; never modifies the input.

Usage:
  apply_radmask_to_images.py --in-dir <.../X> --out-dir <.../X> [--radius 40] [--energy-keep 2.0]
  apply_radmask_to_images.py --base-dir <sample base> --cat-lo 400 --cat-hi 621 \
        --in-fmt '{cat}_volume_images_tick3_ch2_min2_tot3_e3p0/X' \
        --out-base <writable base> --out-suffix _radmask
"""
import argparse, glob, os, sys, time
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from radmask_image import blob_radmask


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('--in-dir'); p.add_argument('--out-dir')
    p.add_argument('--base-dir'); p.add_argument('--out-base')
    p.add_argument('--cat-lo', type=int); p.add_argument('--cat-hi', type=int)
    p.add_argument('--in-fmt', default='{cat}_volume_images_tick3_ch2_min2_tot3_e3p0/X')
    p.add_argument('--out-suffix', default='_radmask')
    p.add_argument('--radius', type=float, default=40.0)
    p.add_argument('--energy-keep', type=float, default=2.0)
    p.add_argument('-f', '--override', action='store_true')
    return p.parse_args()


def process_dir(in_dir, out_dir, radius, ekeep, override=False):
    os.makedirs(out_dir, exist_ok=True)
    files = sorted(glob.glob(os.path.join(in_dir, '*.npz')))
    n_vol = n_masked = 0
    for f in files:
        of = os.path.join(out_dir, os.path.basename(f))
        if os.path.exists(of) and not override:
            continue
        try:
            z = np.load(f, allow_pickle=True)
            imgs, meta = z['images'], z['metadata']
        except Exception as e:
            print(f'  bad file {f}: {e}'); continue
        out_imgs, out_meta = [], []
        for i in range(len(imgs)):
            im = np.asarray(imgs[i], dtype=np.float32)
            masked, info = blob_radmask(im, radius, ekeep, return_info=True)
            md = dict(meta[i]) if isinstance(meta[i], dict) else {}
            md.update({
                'radmask_applied': True,
                'radmask_mode': 'image_blob',
                'radmask_radius_cm': float(radius),
                'radmask_energy_keep_mev': float(ekeep),
                'n_blobs_in_image': int(info['n_blobs']),
                'n_blobs_masked': int(info['n_masked']),
                'n_clusters_in_volume_unmasked': int(md.get('n_clusters_in_volume', -1)),
                'n_marley_clusters_unmasked': int(md.get('n_marley_clusters', -1)),
                'n_non_marley_clusters_unmasked': int(md.get('n_non_marley_clusters', -1)),
            })
            out_imgs.append(masked.astype(np.float32)); out_meta.append(md)
            n_vol += 1; n_masked += int(info['n_masked'])
        np.savez_compressed(of, images=np.array(out_imgs, dtype=object),
                            metadata=np.array(out_meta, dtype=object))
    return len(files), n_vol, n_masked


def main():
    a = parse_args()
    jobs = []
    if a.in_dir:
        jobs.append((a.in_dir, a.out_dir))
    else:
        assert a.base_dir and a.cat_lo is not None and a.cat_hi is not None
        out_base = a.out_base or a.base_dir
        for c in range(a.cat_lo, a.cat_hi + 1):
            cat = f'cat{c:06d}'
            ind = os.path.join(a.base_dir, cat, a.in_fmt.format(cat=cat))
            if not os.path.isdir(ind):
                continue
            rel = a.in_fmt.format(cat=cat)
            head, tail = os.path.split(rel)          # ('<prod>', 'X')
            outd = os.path.join(out_base, cat, head + a.out_suffix, tail)
            jobs.append((ind, outd))
    print(f'{len(jobs)} directories, radius={a.radius} cm energy_keep={a.energy_keep} MeV')
    t0 = time.time()
    for ind, outd in jobs:
        nf, nv, nm = process_dir(ind, outd, a.radius, a.energy_keep, a.override)
        print(f'  {ind} -> {outd}: {nf} files, {nv} volumes, {nm} blobs masked '
              f'({time.time()-t0:.0f}s)', flush=True)
    print(f'done in {time.time()-t0:.0f}s')


if __name__ == '__main__':
    sys.exit(main())
