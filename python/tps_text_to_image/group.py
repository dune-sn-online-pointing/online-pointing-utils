import numpy as np
import sys
sys.path.append('/afs/cern.ch/work/d/dapullia/public/dune/group_plotting/python/libs')
from definitions import *
import ROOT


def read_root_file_to_groups(filename):
    file = ROOT.TFile.Open(filename, "READ")
    groups = []
    event_numbers = []
    for i in file.GetListOfKeys():
        # get the TTree
        tree = file.Get(i.GetName())
        print(f"Tree name: {tree.GetName()}")
        tree.Print()
        counter = 0
        for entry in tree:
            counter += 1
            if counter % 10000 == 0:
                print(f"Group {counter}")
            # nrows = entry.NRows
            # event = entry.Event
            # matrix = entry.Matrix
            nrows = entry.NRows
            event = entry.Event
            event_numbers.append(event )
            # Matrix is <class cppyy.gbl.std.vector<vector<int> > at 0x16365890>
            # extract the matrix from the vector<vector<int>> object to numpy array
            m = np.empty((nrows, 12), dtype=int)
            for j in range(nrows):
                for k in range(12):
                    m[j][k] = entry.Matrix[j][k]
            this_group = Group(m)
            this_group.true_dir_x, this_group.true_dir_y, this_group.true_dir_z = entry.TrueX, entry.TrueY, entry.TrueZ
            groups.append(this_group)
        break
    return groups, event_numbers


class Group: # Only good for HD geometry 1x2x6
    def __init__(self, tps):
        self.tps = tps
        self.n_tps = tps.shape[0]
        self.apa = np.unique(tps[:, idx['channel']] // 2560)
        self.is_clean, self.supernova_fraction, self.total_charge = self.extract_info()
        self.contains_supernova = self.supernova_fraction != 0
        self.event_number = np.unique(tps[:, idx['event_number']])
        self.track_time_lenght = (np.max(tps[:, idx['time_start']]+tps[:, idx['time_over_threshold']]) - np.min(tps[:, idx['time_start']]))*0.08
        self.track_channel_lenght = (np.max(tps[:, idx['channel']]) - np.min(tps[:, idx['channel']]))*0.5
        self.track_total_lenght = np.sqrt(self.track_time_lenght**2 + self.track_channel_lenght**2)
        self.track_total_neutrino_charge = np.sum(tps[:, idx['adc_integral']][tps[:, idx['mc_truth']] == 1])
        self.x, self.y, self.z = None, None, None
        self.true_x, self.true_y, self.true_z = None, None, None
        self.distance = None
        self.calculate_distance_from_true()
        self.true_dir_x, self.true_dir_y, self.true_dir_z = None, None, None

    def __len__(self):
        return self.n_tps
    def __str__(self):
        return str(self.tps)
    def __repr__(self):
        return str(self.tps)
    def get_pos(self):
        return self.x, self.y, self.z
    def get_true_pos(self):
        return self.true_x, self.true_y, self.true_z
    def get_apa(self):
        return self.apa
    def is_clean(self):
        return self.is_clean
    def supernova_fraction(self):
        return self.supernova_fraction
    def extract_info(self):
        supernova_counter = 0
        total_charge = 0
        for tp in self.tps:
            total_charge += tp[idx['adc_integral']]
            if tp[idx['mc_truth']] == 1:
                supernova_counter += 1
        supernova_fraction = supernova_counter / self.n_tps
        is_clean = supernova_fraction == 0 or supernova_fraction == 1
        return is_clean, supernova_fraction, total_charge

    def calculate_distance_from_true(self):
        z_apa_offset = self.tps[:, idx['channel']] //(2560*2) * (apa_lenght_in_cm + offest_between_apa_in_cm)
        z_channel_offset = ((self.tps[:, idx['channel']] % 2560 - 1600) % 480) * wire_pitch_in_cm_collection
        z = wire_pitch_in_cm_collection + z_apa_offset + z_channel_offset
        y = np.zeros(len(z))
        x_signs = np.where((self.tps[:, idx['channel']] % 2560-2080)<0, -1, 1)
        x = ((self.tps[:, idx['time_peak']] - 4792* self.tps[:, idx['event_number']] )* time_tick_in_cm + apa_width_in_cm/2) * x_signs

        true_x, true_y, true_z = self.tps[:,idx['true_x']], self.tps[:, idx['true_y']], self.tps[:, idx['true_z']]

        dists = np.sqrt((x - true_x)**2 + (z - true_z)**2)
        # find the minimum distance
        argmin = np.argmin(dists)
        self.x, self.y, self.z = x[argmin], y[argmin], z[argmin]
        self.true_x, self.true_y, self.true_z = true_x[argmin], true_y[argmin], true_z[argmin]
        self.distance = dists[argmin]

def save_groups(groups, name, save_txt=False, save_npy=False, out_dir_groups='data/'):
    if save_npy:
        np.save(out_dir_groups + name, groups)
    if save_txt:
        with open(out_dir_groups + name+ ".txt", "w") as f:
            for i, group in enumerate(groups):
                f.write("\nGroup %d\n\n" % i)
                for tp in group:
                    f.write("%s %s %s %s %s %s %s %s %s \n" % (tp['time_start'], tp['time_over_threshold'], tp['time_peak'], tp['channel'], tp['adc_integral'], tp['adc_peak'], tp['mc_truth'], tp['event_number'], tp['plane']))

