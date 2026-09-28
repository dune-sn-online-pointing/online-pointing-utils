#!/usr/bin/env python3
"""
Turn the before/after json produced by matchfix_validation.py into the markdown
tables of docs/three_plane_matching_fix_validation.md.

  python3 matchfix_tables.py --before before_es.json --after after_es.json
"""

import argparse
import json

E_BIN_LABELS = ['3-5', '5-10', '10-20', '20-30', '30+']


def pct(a, b):
    return 100.0 * a / b if b else float('nan')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--before', required=True)
    ap.add_argument('--after', required=True)
    ap.add_argument('--label', default='ES (es_* files)')
    ap.add_argument('--per-cat', action='store_true')
    args = ap.parse_args()

    b = json.load(open(args.before))
    a = json.load(open(args.after))
    cb, ca = b['combined'], a['combined']
    n = cb['n_x_main']

    print(f"### Overall — {args.label}, 50 cats\n")
    print("| quantity | before | after | delta |")
    print("|---|---|---|---|")
    rows = [
        ('X main-track clusters', 'n_x_main', False),
        ('`match_id != -1`', 'n_x_main_matched', True),
        ('**3-plane main-track match**', 'n_3plane', True),
        ('wrong partner (match_id on a non-main cluster)', 'n_partner_bg_any', True),
        ('... in U', 'n_partner_bg_u', True),
        ('... in V', 'n_partner_bg_v', True),
        ('orphan match_id (in X only, no U/V)', 'n_orphan', True),
        ('3-plane and E_true > 3 MeV (pointing input)', 'n_pointing', True),
    ]
    for label, key, as_pct in rows:
        vb, va = cb[key], ca[key]
        if as_pct:
            print(f"| {label} | {vb} ({pct(vb, n):.1f}%) | {va} ({pct(va, ca['n_x_main']):.1f}%) | "
                  f"{va - vb:+d} ({pct(va, ca['n_x_main']) - pct(vb, n):+.1f} pts) |")
        else:
            print(f"| {label} | {vb} | {va} | {va - vb:+d} |")

    print(f"\n### 3-plane match fraction vs true electron energy\n")
    print("| E_true [MeV] | N | before | after | delta |")
    print("|---|---|---|---|---|")
    for i, lab in enumerate(E_BIN_LABELS):
        tb, ob = cb['ebin_tot'][i], cb['ebin_ok'][i]
        ta, oa = ca['ebin_tot'][i], ca['ebin_ok'][i]
        assert tb == ta, f"denominator changed in bin {lab}: {tb} vs {ta}"
        print(f"| {lab} | {tb} | {ob} ({pct(ob, tb):.1f}%) | {oa} ({pct(oa, ta):.1f}%) | "
              f"{pct(oa, ta) - pct(ob, tb):+.1f} pts |")
    print(f"| **all** | {n} | {cb['n_3plane']} ({pct(cb['n_3plane'], n):.1f}%) | "
          f"{ca['n_3plane']} ({pct(ca['n_3plane'], ca['n_x_main']):.1f}%) | "
          f"{pct(ca['n_3plane'], ca['n_x_main']) - pct(cb['n_3plane'], n):+.1f} pts |")

    print("\n### Partner purity (the partner actually written into the match)\n")
    print("| quantity | before | after |")
    print("|---|---|---|")
    for label, key in [('partner from a different event', 'n_ok_partner_diff_event'),
                       ('partner with MARLEY fraction < 0.5', 'n_ok_partner_non_marley'),
                       ('partner on a different APA', 'n_ok_partner_diff_apa')]:
        print(f"| {label} | {cb[key]} | {ca[key]} |")
    print("\nBreakdown of the wrong-partner (non-main) cases:\n")
    print("| quantity | before | after |")
    print("|---|---|---|")
    for label, key in [('different event', 'n_partner_diff_event'),
                       ('non-MARLEY cluster', 'n_partner_non_marley'),
                       ('MARLEY fragment of the same event', 'n_partner_marley_frag'),
                       ('different APA', 'n_partner_diff_apa')]:
        print(f"| {label} | {cb[key]} | {ca[key]} |")

    print("\n### match_type distribution (clusters with `match_id != -1`)\n")
    print("| plane | before | after |")
    print("|---|---|---|")
    for pl in ('x', 'u', 'v'):
        kb = cb['match_type_%s' % pl]
        ka = ca['match_type_%s' % pl]
        print(f"| {pl.upper()} | {kb} | {ka} |")

    if args.per_cat:
        print("\n### Per cat\n")
        print("| cat | X main | 3-plane before | 3-plane after | pointing before | pointing after |")
        print("|---|---|---|---|---|---|")
        pb = {r['cat']: r for r in b['per_cat']}
        pa = {r['cat']: r for r in a['per_cat']}
        tb = ta = tpb = tpa = tn = 0
        for c in sorted(pb):
            if c not in pa:
                print(f"| {c} | {pb[c]['n_x_main']} | {pb[c]['n_3plane']} | MISSING | "
                      f"{pb[c]['n_pointing']} | MISSING |")
                continue
            rb, ra = pb[c], pa[c]
            nn = rb['n_x_main']
            tn += nn
            tb += rb['n_3plane']
            ta += ra['n_3plane']
            tpb += rb['n_pointing']
            tpa += ra['n_pointing']
            print(f"| {c} | {nn} | {rb['n_3plane']} ({pct(rb['n_3plane'], nn):.1f}%) | "
                  f"{ra['n_3plane']} ({pct(ra['n_3plane'], ra['n_x_main']):.1f}%) | "
                  f"{rb['n_pointing']} | {ra['n_pointing']} |")
        print(f"| **total** | {tn} | {tb} ({pct(tb, tn):.1f}%) | {ta} ({pct(ta, tn):.1f}%) | "
              f"{tpb} | {tpa} |")


if __name__ == '__main__':
    main()
