#!/usr/bin/env python3
"""
Check the image-level (blob) approximation against the exact cluster-level mask.

Compares, volume by volume, the pre-rendered `*_radmask*` product (produced by
create_volumes.py --radmask from the cluster ROOT files) with blob_radmask() applied to
the corresponding unmasked image.
"""
import argparse, glob, os, sys
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from radmask_image import blob_radmask


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--base-dir', required=True)
    ap.add_argument('--cat-lo', type=int, required=True)
    ap.add_argument('--cat-hi', type=int, required=True)
    ap.add_argument('--orig-fmt', default='{cat}_volume_images_tick3_ch2_min2_tot3_e3p0/X')
    ap.add_argument('--masked-fmt', default='{cat}_volume_images_tick3_ch2_min2_tot3_e3p0_radmask/X')
    ap.add_argument('--radius', type=float, default=40.0)
    ap.add_argument('--energy-keep', type=float, default=2.0)
    ap.add_argument('--max-files-per-cat', type=int, default=1000)
    a = ap.parse_args()

    n = n_id = 0
    dADC = []
    nmask_ex, nmask_ap = [], []
    nblob, nclus = [], []
    for c in range(a.cat_lo, a.cat_hi + 1):
        cat = f'cat{c:06d}'
        d0 = os.path.join(a.base_dir, cat, a.orig_fmt.format(cat=cat))
        d1 = os.path.join(a.base_dir, cat, a.masked_fmt.format(cat=cat))
        if not (os.path.isdir(d0) and os.path.isdir(d1)):
            continue
        for f1 in sorted(glob.glob(os.path.join(d1, '*.npz')))[:a.max_files_per_cat]:
            f0 = os.path.join(d0, os.path.basename(f1))
            if not os.path.exists(f0):
                continue
            try:
                z0 = np.load(f0, allow_pickle=True); i0, m0 = z0['images'], z0['metadata']
                z1 = np.load(f1, allow_pickle=True); i1, m1 = z1['images'], z1['metadata']
            except Exception as e:
                print('bad', f1, e); continue
            if len(i0) != len(i1):
                continue
            for i in range(len(i0)):
                im = np.asarray(i0[i], dtype=np.float32)
                ex = np.asarray(i1[i], dtype=np.float32)
                ap_img, info = blob_radmask(im, a.radius, a.energy_keep, return_info=True)
                n += 1
                if np.array_equal(ap_img, ex):
                    n_id += 1
                dADC.append((ap_img.sum() - ex.sum()) / max(im.sum(), 1e-9))
                nmask_ex.append(int(m1[i].get('n_clusters_masked', -1)))
                nmask_ap.append(int(info['n_masked']))
                nblob.append(int(info['n_blobs']))
                nclus.append(int(m0[i].get('n_clusters_in_volume', -1)))
        print(f'  {cat}: {n} volumes, identical {n_id}', flush=True)

    dADC = np.array(dADC); ex = np.array(nmask_ex); apn = np.array(nmask_ap)
    nb = np.array(nblob); nc = np.array(nclus)
    print(f'\nvolumes compared: {n}')
    print(f'pixel-identical images: {n_id} ({n_id/max(n,1):.4f})')
    print(f'relative ADC difference: mean={dADC.mean():+.5f} rms={dADC.std():.5f} '
          f'max|.|={np.abs(dADC).max():.5f}; |diff|>1% in {(np.abs(dADC)>0.01).sum()} volumes')
    print(f'n masked, exact vs blob: equal in {(ex==apn).sum()} ({(ex==apn).mean():.4f}); '
          f'totals {ex.sum()} vs {apn.sum()}')
    print(f'blob count vs cluster count: equal in {(nb==nc).mean():.4f}; '
          f'<blobs>={nb.mean():.2f} <clusters>={nc.mean():.2f}')


if __name__ == '__main__':
    main()
