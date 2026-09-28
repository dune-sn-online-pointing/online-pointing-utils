#!/usr/bin/env python3
"""
Robustness of the CT working point against the radiological rate.

The burst samples contain two radiological regimes (~2.4 non-MARLEY clusters per volume
in the CT train/test cats, ~0.64 in the mixture-dev cats 623-672).  This script compares
the two, with and without the radiological mask, at a threshold fixed on ONE of them -
i.e. exactly what happens when a model tuned on one sample is deployed on another.
"""
import argparse, glob
import numpy as np


def load(pats):
    parts = []
    for p in pats:
        for f in sorted(glob.glob(p)):
            parts.append(dict(np.load(f)))
    keys = set(parts[0])
    for p in parts:
        keys &= set(p)
    return {k: np.concatenate([p[k] for p in parts]) for k in keys}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--a', nargs='+', required=True, help='sample A (reference, e.g. test cats)')
    ap.add_argument('--b', nargs='+', required=True, help='sample B (e.g. dev cats)')
    ap.add_argument('--label-a', default='A'); ap.add_argument('--label-b', default='B')
    ap.add_argument('--thresholds', default='0.5,0.8')
    a = ap.parse_args()
    A, B = load(a.a), load(a.b)
    variants = sorted(set(k[5:] for k in A if k.startswith('p_es_')) &
                      set(k[5:] for k in B if k.startswith('p_es_')),
                      key=lambda v: (v != 'orig', v))
    print(f'{a.label_a}: {len(A["label"])} volumes, <n_rad/vol> = {A["n_nonmar"].mean():.2f}')
    print(f'{a.label_b}: {len(B["label"])} volumes, <n_rad/vol> = {B["n_nonmar"].mean():.2f}')
    print(f'variants: {variants}\n')
    for thr in [float(t) for t in a.thresholds.split(',')]:
        print(f'--- fixed threshold P(ES) > {thr} ---')
        print(f"{'variant':>12} | {'ES eff '+a.label_a:>14} {'ES eff '+a.label_b:>14} {'ratio':>7} "
              f"| {'CC rej '+a.label_a:>14} {'CC rej '+a.label_b:>14} {'delta':>8}")
        for v in variants:
            ea = (A['p_es_' + v] > thr)[A['label'] == 0].mean()
            eb = (B['p_es_' + v] > thr)[B['label'] == 0].mean()
            ra = (A['p_es_' + v] <= thr)[A['label'] == 1].mean()
            rb = (B['p_es_' + v] <= thr)[B['label'] == 1].mean()
            print(f'{v:>12} | {ea:>14.4f} {eb:>14.4f} {eb/max(ea,1e-9):>7.3f} '
                  f'| {ra:>14.4f} {rb:>14.4f} {rb-ra:>+8.4f}')
        print()
    print('--- median P(ES), matched in reconstructed energy, between the two samples ---')
    ebins = [(0, 3), (3, 5), (5, 7), (7, 10), (10, 15), (15, 25)]
    for lab, cls in ((0, 'ES'), (1, 'CC')):
        print(f'  {cls}:')
        print(f"{'E [MeV]':>8} " + ' '.join(f'{v[:9]:>10} {"diff":>7}' for v in variants))
        for lo, hi in ebins:
            sa = (A['label'] == lab) & (A['energy'] >= lo) & (A['energy'] < hi)
            sb = (B['label'] == lab) & (B['energy'] >= lo) & (B['energy'] < hi)
            if sa.sum() < 20 or sb.sum() < 20:
                continue
            row = f'{lo}-{hi:<4}'.rjust(8) + ' '
            for v in variants:
                ma, mb = np.median(A['p_es_' + v][sa]), np.median(B['p_es_' + v][sb])
                row += f'{ma:>10.3f} {mb-ma:>+7.3f} '
            print(row)


if __name__ == '__main__':
    main()
