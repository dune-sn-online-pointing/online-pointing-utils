#!/usr/bin/env python3
"""
Truth validation of the radiological-cluster mask.

Input: the satellite-level table written by radmask_cluster_scan.py.
Output: the purity / signal-loss tables of the mask, as a function of the
reconstructed main-cluster energy, separately for ES and CC volumes.

The mask itself is truth-free (distance to the main track + reconstructed cluster
charge); `is_marley` is used here only to score it.
"""
import argparse
import numpy as np

E_BINS = [(0, 3), (3, 5), (5, 7), (7, 10), (10, 15), (15, 25), (25, 1e9)]
E_LAB = ['0-3', '3-5', '5-7', '7-10', '10-15', '15-25', '>25']


def masked_flag(r, R, Ek):
    m = r['dcen'] >= R
    if Ek > 0:
        m &= r['cE'] < Ek
    return m


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--rows', required=True)
    ap.add_argument('--radius', type=float, default=40.0)
    ap.add_argument('--energy-keep', type=float, default=2.0)
    ap.add_argument('--grid', action='store_true', help='also print the (R,Ekeep) grid')
    ap.add_argument('--rad-per-volume', type=float, default=None,
                    help='rescale the radiological rate to this many per volume when '
                         'quoting the purity of the masked sample (e.g. 2.35 for the '
                         'CT training/test cats); default: as in the input sample')
    a = ap.parse_args()
    r = np.load(a.rows)
    v = np.load(a.rows.replace('.npy', '_vols.npy'))
    R, Ek = a.radius, a.energy_keep
    msk = masked_flag(r, R, Ek)

    print(f'sample: {len(v)} volumes, {len(r)} satellite clusters')
    for t in ('ES', 'CC'):
        s = v['itype'] == t
        print(f'  {t}: {s.sum()} volumes, <n_marley_sat>={v["nmar"][s].mean()-1:.3f} '
              f'<n_radiological>={v["nnon"][s].mean():.3f} per volume')
    print(f'\nmask: keep satellite if d < {R} cm or E_cluster >= {Ek} MeV\n')

    hdr = (f"{'E [MeV]':>8} {'N vol':>7} {'N rad':>7} {'N mar':>7} | "
           f"{'rad masked':>10} {'mar masked':>10} | {'<rad/vol> before':>16} {'after':>7} | "
           f"{'purity masked':>13}")
    for t in ('ES', 'CC'):
        print(f'--- {t} volumes ---')
        print(hdr)
        for (lo, hi), lab in zip(E_BINS, E_LAB):
            sr = (r['itype'] == t) & (r['E'] >= lo) & (r['E'] < hi)
            sv = (v['itype'] == t) & (v['E'] >= lo) & (v['E'] < hi)
            nvol = int(sv.sum())
            if nvol == 0:
                continue
            rad = sr & (r['marley'] == 0)
            mar = sr & (r['marley'] == 1)
            f_rad = msk[rad].mean() if rad.sum() else np.nan
            f_mar = msk[mar].mean() if mar.sum() else np.nan
            before = rad.sum() / nvol
            after = (rad & ~msk).sum() / nvol
            nm_rad, nm_mar = (rad & msk).sum(), (mar & msk).sum()
            if a.rad_per_volume:
                w = a.rad_per_volume / before if before > 0 else 1.0
                pur = (nm_rad * w) / (nm_rad * w + nm_mar) if (nm_rad * w + nm_mar) else np.nan
            else:
                pur = nm_rad / (nm_rad + nm_mar) if (nm_rad + nm_mar) else np.nan
            print(f'{lab:>8} {nvol:>7} {int(rad.sum()):>7} {int(mar.sum()):>7} | '
                  f'{f_rad:>10.3f} {f_mar:>10.3f} | {before:>16.3f} {after:>7.3f} | {pur:>13.3f}')
        print()

    # survival of the CC de-excitation gammas, by distance
    print('--- survival of MARLEY satellite clusters vs their distance from the main track ---')
    print(f"{'d [cm]':>10} {'N ES':>7} {'kept':>7} {'N CC':>7} {'kept':>7} {'N rad':>8} {'kept':>7}")
    edges = [0, 5, 10, 15, 20, 25, 30, 35, 40, 50, 75]
    for lo, hi in zip(edges[:-1], edges[1:]):
        sd = (r['dcen'] >= lo) & (r['dcen'] < hi)
        row = [f'{lo}-{hi}'.rjust(10)]
        for t in ('ES', 'CC'):
            s = sd & (r['itype'] == t) & (r['marley'] == 1)
            row.append(f'{int(s.sum()):>7}')
            row.append(f'{(~msk[s]).mean():>7.3f}' if s.sum() else f'{"-":>7}')
        s = sd & (r['marley'] == 0)
        row.append(f'{int(s.sum()):>8}')
        row.append(f'{(~msk[s]).mean():>7.3f}' if s.sum() else f'{"-":>7}')
        print(' '.join(row))

    if a.grid:
        print('\n--- (R, Ekeep) grid: radiologicals removed / MARLEY satellites lost ---')
        print(f"{'R':>4} {'Ekeep':>6} | {'rad rm':>7} {'mar loss ES':>11} {'mar loss CC':>11}")
        for RR in (25, 30, 35, 40, 45, 50):
            for EE in (-1, 1.5, 2.0, 2.5, 3.0):
                mm = masked_flag(r, RR, EE)
                rad = r['marley'] == 0
                me = (r['marley'] == 1) & (r['itype'] == 'ES')
                mc = (r['marley'] == 1) & (r['itype'] == 'CC')
                print(f'{RR:>4} {EE:>6.1f} | {mm[rad].mean():>7.3f} '
                      f'{mm[me].mean():>11.3f} {mm[mc].mean():>11.3f}')


if __name__ == '__main__':
    main()
