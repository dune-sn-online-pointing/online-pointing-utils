#!/usr/bin/env python3
"""
Score a channel-tagging model on volume images with and without the radiological mask.

Three image variants can be scored in the same pass, volume by volume, so that the
scores are directly comparable event by event:
  orig    : the existing (unmasked) volume image
  masked  : a pre-rendered `*_radmask*` product (cluster-level mask, exact)
  blobR_E : the image-level (blob) approximation of the same mask, computed on the fly

Preprocessing reproduces the v80 trainer exactly: log1p(raw ADC), float16,
no per-image normalization, image-only input.  Images are scored in chunks so memory
stays bounded.

Output: one npz with per-volume label / energy / multiplicities / scores.
"""
import argparse, glob, os, sys
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from radmask_image import blob_radmask

IMAGE_SHAPE = (208, 1242)


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('--model', required=True)
    p.add_argument('--base-dir', required=True)
    p.add_argument('--cat-lo', type=int, required=True)
    p.add_argument('--cat-hi', type=int, required=True)
    p.add_argument('--orig-fmt', default='{cat}_volume_images_tick3_ch2_min2_tot3_e3p0/X')
    p.add_argument('--masked-fmt', default=None)
    p.add_argument('--blob-mask', default=None, help='e.g. 40:2.0,30:2.0')
    p.add_argument('--max-per-class', type=int, default=1000000)
    p.add_argument('--batch-size', type=int, default=32)
    p.add_argument('--chunk', type=int, default=256, help='volumes held in memory per predict call')
    p.add_argument('--out', required=True)
    return p.parse_args()


def cat_dir(base, cat, fmt):
    c = f'cat{cat:06d}'
    return os.path.join(base, c, fmt.format(cat=c))


class Scorer:
    def __init__(self, model, variants, batch_size, chunk):
        self.model, self.variants = model, variants
        self.bs, self.chunk = batch_size, chunk
        self.buf = {v: [] for v in variants}
        self.out = {v: [] for v in variants}

    def add(self, images):
        for v, im in images.items():
            self.buf[v].append(np.log1p(im).astype(np.float16))
        if len(self.buf[self.variants[0]]) >= self.chunk:
            self.flush()

    def flush(self):
        if not self.buf[self.variants[0]]:
            return
        for v in self.variants:
            X = np.stack(self.buf[v]).astype(np.float16)[..., np.newaxis]
            p = np.asarray(self.model.predict(X, batch_size=self.bs, verbose=0))
            self.out[v].append(p[:, 0] if p.shape[1] == 2 else 1.0 - p[:, 0])
            self.buf[v] = []
            del X

    def result(self):
        self.flush()
        return {v: np.concatenate(self.out[v]) if self.out[v] else np.array([])
                for v in self.variants}


def main():
    a = parse_args()
    from tensorflow import keras
    model = keras.models.load_model(a.model, compile=False)

    blob_variants = []
    if a.blob_mask:
        for tok in a.blob_mask.split(','):
            R, E = tok.split(':')
            blob_variants.append((float(R), float(E)))

    variants = ['orig'] + (['masked'] if a.masked_fmt else []) + \
               [f'blob{R:g}_{E:g}' for R, E in blob_variants]
    sc = Scorer(model, variants, a.batch_size, a.chunk)

    cols = {k: [] for k in ('label', 'cat', 'energy', 'n_clus', 'n_mar', 'n_nonmar',
                            'adc_orig', 'pix_orig', 'nclus_masked', 'nmar_masked',
                            'nnonmar_masked', 'adc_masked')}
    counts = {0: 0, 1: 0}
    n_done = 0
    for cat in range(a.cat_lo, a.cat_hi + 1):
        d0 = cat_dir(a.base_dir, cat, a.orig_fmt)
        if not os.path.isdir(d0):
            continue
        d1 = cat_dir(a.base_dir, cat, a.masked_fmt) if a.masked_fmt else None
        if d1 and not os.path.isdir(d1):
            print(f'  cat {cat}: no masked product, skipped'); continue
        for label, pref in ((0, 'es_'), (1, 'cc_')):
            for f0 in sorted(glob.glob(os.path.join(d0, pref + '*.npz'))):
                if counts[label] >= a.max_per_class:
                    break
                try:
                    z0 = np.load(f0, allow_pickle=True)
                    i0, m0 = z0['images'], z0['metadata']
                except Exception as e:
                    print(f'  bad file {f0}: {e}'); continue
                i1 = m1 = None
                if d1:
                    f1 = os.path.join(d1, os.path.basename(f0))
                    if not os.path.exists(f1):
                        continue
                    try:
                        z1 = np.load(f1, allow_pickle=True)
                        i1, m1 = z1['images'], z1['metadata']
                    except Exception as e:
                        print(f'  bad masked file {f1}: {e}'); continue
                    if len(i1) != len(i0):
                        print(f'  length mismatch {f0}: {len(i0)} vs {len(i1)}'); continue
                for i in range(len(i0)):
                    if counts[label] >= a.max_per_class:
                        break
                    im = np.asarray(i0[i], dtype=np.float32)
                    if im.shape != IMAGE_SHAPE:
                        continue
                    md = m0[i]
                    images = {'orig': im}
                    if i1 is not None:
                        im1 = np.asarray(i1[i], dtype=np.float32)
                        images['masked'] = im1
                        cols['adc_masked'].append(float(im1.sum()))
                        cols['nclus_masked'].append(int(m1[i].get('n_clusters_masked', -1)))
                        cols['nmar_masked'].append(int(m1[i].get('n_marley_clusters_masked', -1)))
                        cols['nnonmar_masked'].append(int(m1[i].get('n_non_marley_clusters_masked', -1)))
                    for R, E in blob_variants:
                        images[f'blob{R:g}_{E:g}'] = blob_radmask(im, R, E)
                    cols['label'].append(label); cols['cat'].append(cat)
                    cols['energy'].append(float(md.get('particle_energy', np.nan)))
                    cols['n_clus'].append(int(md.get('n_clusters_in_volume', 0)))
                    cols['n_mar'].append(int(md.get('n_marley_clusters', 0)))
                    cols['n_nonmar'].append(int(md.get('n_non_marley_clusters', 0)))
                    cols['adc_orig'].append(float(im.sum()))
                    cols['pix_orig'].append(int(np.count_nonzero(im)))
                    sc.add(images)
                    counts[label] += 1; n_done += 1
        print(f'  cat {cat}: ES={counts[0]} CC={counts[1]}', flush=True)

    scores = sc.result()
    out = {k: np.array(v) for k, v in cols.items() if len(v)}
    for v, p in scores.items():
        out['p_es_' + v] = p
    n = len(out['label'])
    for k in out:
        assert len(out[k]) == n, (k, len(out[k]), n)
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    np.savez_compressed(a.out, **out)
    print('wrote', a.out, 'n =', n, 'variants =', variants)


if __name__ == '__main__':
    sys.exit(main())
