#!/usr/bin/env python3
"""Truth-level scan behind the radiological-mask validation.

Reads the matched-cluster ROOT files of a set of cats, reproduces exactly the
volume-cluster selection of create_volumes.py, and writes one row per satellite cluster
with the quantities the mask uses (distance to the main track, reconstructed cluster
energy) plus the truth flag (is_marley) that the mask must NOT use.  A companion
*_vols.npy holds one row per volume.

Usage:
  radmask_cluster_scan.py <cat000623,cat000624,...> <max files per cat> <out.npy>
    [--base <sample base dir>]
"""
import sys, glob, os
import numpy as np
sys.path.insert(0, '/afs/cern.ch/work/e/evilla/private/dune/refactor-online-utils/python/app')
import create_volumes as cv

def scan(files, plane='X'):
    rows = []   # per satellite cluster
    vols = []   # per volume
    for cf in files:
        clusters = cv.load_clusters_from_file(cf, plane=plane, verbose=False,
                                              use_global_channels=(plane=='X'))
        if not clusters:
            continue
        mains = [c for c in clusters if c['is_main_cluster']]
        center_key = 'center_channel'
        chan_key = 'channels'
        if clusters[0].get('channels_global') is not None:
            center_key = 'center_channel_global'; chan_key = 'channels_global'
        for m in mains:
            vc = cv.get_clusters_in_volume(clusters, m[center_key], m['center_time_tpc'],
                                           cv.VOLUME_SIZE_CM, event=m['event'], channel_key=center_key)
            if not vc:
                continue
            is_es = m['is_es_interaction']
            if not m['is_marley']:
                itype = 'BG'
            else:
                itype = 'ES' if is_es else 'CC'
            E = m.get('reco_energy_mev', -1.0)
            # main-track TP coordinates for nearest-TP distance
            mch = np.asarray(m[chan_key], dtype=float)*cv.CHANNEL_PITCH_CM
            mtt = np.asarray(m['times_tpc'], dtype=float)*cv.DRIFT_VELOCITY_CM_PER_TICK
            nsat = 0
            for c in vc:
                if c['cluster_id'] == m['cluster_id']:
                    continue
                dch = (c[center_key]-m[center_key])*cv.CHANNEL_PITCH_CM
                dt  = (c['center_time_tpc']-m['center_time_tpc'])*cv.DRIFT_VELOCITY_CM_PER_TICK
                dcen = np.hypot(dch, dt)
                cch = np.asarray(c[chan_key], dtype=float)*cv.CHANNEL_PITCH_CM
                ctt = np.asarray(c['times_tpc'], dtype=float)*cv.DRIFT_VELOCITY_CM_PER_TICK
                # nearest TP-to-TP distance between satellite and main track
                dmin = np.sqrt(((cch[:,None]-mch[None,:])**2 + (ctt[:,None]-mtt[None,:])**2).min())
                rows.append((itype, E, float(dcen), float(dmin), int(c['is_marley']),
                             float(c['reco_energy_mev']), int(c['n_tps']), float(c['total_charge']),
                             int(c['is_discarded']), int(c.get('match_id',-1)), int(c.get('match_type',-1)),
                             float(np.ptp(cch)) if len(cch)>1 else 0.0, float(np.ptp(ctt)) if len(ctt)>1 else 0.0))
                nsat += 1
            vols.append((itype, E, len(vc), sum(1 for c in vc if c['is_marley']),
                         len(vc)-sum(1 for c in vc if c['is_marley']), int(m['n_tps']),
                         int(m.get('match_id',-1)), float(m['reco_energy_mev'])))
    return rows, vols

if __name__ == '__main__':
    base = '/eos/user/e/evilla/dune/sn-tps/mixture_dev_samples'
    cats = sys.argv[1].split(',') if len(sys.argv)>1 else ['cat000623']
    nf = int(sys.argv[2]) if len(sys.argv)>2 else 10
    files = []
    for cat in cats:
        d = f'{base}/{cat}/{cat}_matched_clusters_tick3_ch2_min2_tot3_e3p0'
        files += sorted(glob.glob(d+'/*_matched.root'))[:nf]
        files += sorted(glob.glob(d+'/es_*_matched.root'))[:nf]
    files = sorted(set(files))
    print(f'{len(files)} files', flush=True)
    rows, vols = scan(files)
    np.save(sys.argv[3] if len(sys.argv)>3 else '/tmp/rows.npy',
            np.array(rows, dtype=[('itype','U3'),('E','f8'),('dcen','f8'),('dmin','f8'),
                                  ('marley','i4'),('cE','f8'),('ntps','i4'),('q','f8'),('disc','i4'),
                                  ('match_id','i4'),('match_type','i4'),('extch','f8'),('extt','f8')]))
    np.save((sys.argv[3] if len(sys.argv)>3 else '/tmp/rows.npy').replace('.npy','_vols.npy'),
            np.array(vols, dtype=[('itype','U3'),('E','f8'),('nclus','i4'),('nmar','i4'),('nnon','i4'),('ntps','i4'),('mmatch','i4'),('mE','f8')]))
    print('saved', len(rows), 'satellites,', len(vols), 'volumes')
