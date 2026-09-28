#!/usr/bin/env python3
"""
Compare CT scores on original vs radiological-masked volume images.

Reads one or more npz files written by radmask_score_ct.py and prints:
  - median P(ES) and efficiency vs reconstructed energy, per variant
  - the same split by radiological multiplicity (the Table 7 test of the CT v80 study)
  - working points: threshold matched to a target CC rejection, ES efficiency overall
    and for E > 10 MeV, and the fraction of ES pointing information kept
  - AUC, marginal and conditional in energy bins
"""
import argparse, glob
import numpy as np

E_BINS = [(0, 3), (3, 5), (5, 7), (7, 10), (10, 15), (15, 25), (25, 1e9)]
E_LAB = ['0-3', '3-5', '5-7', '7-10', '10-15', '15-25', '>25']
# theta68 (deg) of the ES electron vs neutrino, from Table 0 of the CT v80 study
THETA68 = [36.5, 24.5, 19.2, 14.9, 10.9, 7.1, 4.6]


def auc(sig, bkg):
    if len(sig) == 0 or len(bkg) == 0:
        return np.nan
    x = np.concatenate([sig, bkg])
    r = np.argsort(np.argsort(x)) + 1.0
    # average ranks for ties
    order = np.argsort(x); xs = x[order]
    i = 0
    while i < len(xs):
        j = i
        while j + 1 < len(xs) and xs[j + 1] == xs[i]:
            j += 1
        if j > i:
            r[order[i:j + 1]] = np.mean(r[order[i:j + 1]])
        i = j + 1
    n1 = len(sig)
    return (r[:n1].sum() - n1 * (n1 + 1) / 2.0) / (n1 * len(bkg))


def thr_for_rejection(p_cc, target):
    """smallest threshold with CC rejection >= target (CC rejected when P(ES) < thr)"""
    s = np.sort(p_cc)
    k = int(np.ceil(target * len(s))) - 1
    k = min(max(k, 0), len(s) - 1)
    return np.nextafter(s[k], np.inf)


def weights_energy(E):
    w = np.zeros(len(E))
    for (lo, hi), t in zip(E_BINS, THETA68):
        w[(E >= lo) & (E < hi)] = 1.0 / t ** 2
    return w


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--files', nargs='+', required=True)
    ap.add_argument('--rejection', type=float, default=0.945)
    a = ap.parse_args()

    parts = []
    for pat in a.files:
        for f in sorted(glob.glob(pat)):
            parts.append(dict(np.load(f)))
    keys = set(parts[0])
    for p in parts:
        keys &= set(p)
    d = {k: np.concatenate([p[k] for p in parts]) for k in keys}
    variants = sorted([k[5:] for k in d if k.startswith('p_es_')],
                      key=lambda v: (v != 'orig', v))
    lab, E = d['label'], d['energy']
    es, cc = lab == 0, lab == 1
    print(f'{len(lab)} volumes: ES={es.sum()} CC={cc.sum()}; '
          f'<n_radiological/volume> = {d["n_nonmar"].mean():.2f}')
    print(f'variants: {variants}\n')

    print('=== median P(ES) and efficiency vs reconstructed energy ===')
    hdr = f"{'E [MeV]':>8} {'N ES':>6} " + ' '.join(f'{v[:11]:>12}' for v in variants) + \
          f"   {'N CC':>6} " + ' '.join(f'{v[:11]:>12}' for v in variants)
    print(hdr)
    for (lo, hi), l in zip(E_BINS, E_LAB):
        se, sc = es & (E >= lo) & (E < hi), cc & (E >= lo) & (E < hi)
        if se.sum() == 0 and sc.sum() == 0:
            continue
        row = f'{l:>8} {int(se.sum()):>6} '
        row += ' '.join(f'{np.median(d["p_es_"+v][se]):>12.3f}' for v in variants)
        row += f'   {int(sc.sum()):>6} '
        row += ' '.join(f'{np.median(d["p_es_"+v][sc]):>12.3f}' for v in variants)
        print(row)

    print('\n=== clean ES (n_marley==1), median P(ES) split by radiological multiplicity ===')
    clean = es & (d['n_mar'] == 1)
    print(f"{'E [MeV]':>8} {'N<=1':>6} " + ' '.join(f'{v[:9]:>10}' for v in variants) +
          f"   {'N>=4':>6} " + ' '.join(f'{v[:9]:>10}' for v in variants))
    for (lo, hi), l in zip(E_BINS, E_LAB):
        lo_s = clean & (E >= lo) & (E < hi) & (d['n_nonmar'] <= 1)
        hi_s = clean & (E >= lo) & (E < hi) & (d['n_nonmar'] >= 4)
        if lo_s.sum() + hi_s.sum() == 0:
            continue
        row = f'{l:>8} {int(lo_s.sum()):>6} '
        row += ' '.join(f'{np.median(d["p_es_"+v][lo_s]):>10.3f}' if lo_s.sum() else f'{"-":>10}'
                        for v in variants)
        row += f'   {int(hi_s.sum()):>6} '
        row += ' '.join(f'{np.median(d["p_es_"+v][hi_s]):>10.3f}' if hi_s.sum() else f'{"-":>10}'
                        for v in variants)
        print(row)

    print(f'\n=== working points at matched CC rejection {a.rejection} ===')
    w = weights_energy(E)
    print(f"{'variant':>12} {'thr':>8} {'ES eff':>8} {'ES eff E>10':>12} "
          f"{'CC rej E>10':>12} {'ptg info kept':>14} {'AUC':>7}")
    for v in variants:
        p = d['p_es_' + v]
        t = thr_for_rejection(p[cc], a.rejection)
        sel = p >= t
        hi = E > 10
        eff = sel[es].mean()
        eff_hi = sel[es & hi].mean() if (es & hi).sum() else np.nan
        rej_hi = (~sel)[cc & hi].mean() if (cc & hi).sum() else np.nan
        info = w[es & sel].sum() / w[es].sum()
        print(f'{v:>12} {t:>8.4f} {eff:>8.3f} {eff_hi:>12.3f} {rej_hi:>12.3f} '
              f'{info:>14.3f} {auc(p[es], p[cc]):>7.4f}')

    print('\n=== conditional AUC in energy bins ===')
    print(f"{'E [MeV]':>8} {'N ES':>6} {'N CC':>6} " + ' '.join(f'{v[:11]:>12}' for v in variants))
    for (lo, hi), l in zip(E_BINS, E_LAB):
        se, sc = es & (E >= lo) & (E < hi), cc & (E >= lo) & (E < hi)
        if se.sum() < 20 or sc.sum() < 20:
            continue
        print(f'{l:>8} {int(se.sum()):>6} {int(sc.sum()):>6} ' +
              ' '.join(f'{auc(d["p_es_"+v][se], d["p_es_"+v][sc]):>12.4f}' for v in variants))

    print('\n=== paired score shift, volumes where the mask removed >=1 cluster ===')
    base = d['p_es_orig']
    for v in variants:
        if v == 'orig':
            continue
        dp = d['p_es_' + v] - base
        ch = np.abs(dp) > 1e-6
        for l, name in ((0, 'ES'), (1, 'CC')):
            s = ch & (lab == l)
            s10 = s & (E > 10)
            if s.sum() == 0:
                continue
            print(f'{v:>12} {name}: changed {s.sum()}/{int((lab==l).sum())} '
                  f'({s.sum()/max((lab==l).sum(),1):.2f}); median dP(ES) = {np.median(dp[s]):+.4f}; '
                  f'E>10: N={s10.sum()} median dP = {np.median(dp[s10]):+.4f}'
                  if s10.sum() else '')


if __name__ == '__main__':
    main()
