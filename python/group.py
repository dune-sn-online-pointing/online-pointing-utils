import numpy as np
import sys
import ROOT

class Group: # Only good for HD geometry 1x2x6
    def __init__(self, tps):
        self.tps_ = tps
        self.true_dir_ = None
        self.reco_pos_ = None
        self.n_tps_ = tps.shape[0]
        self.min_distance_from_true_pos_ = None
        self.true_label_ = None
        self.supernova_tp_fraction_ = None
    def __len__(self):
        return self.n_tps_
    def __str__(self):
        return str(self.tps_)
    def __repr__(self):
        return str(self.tps_)
    def get_tps(self):
        return self.tps_
    def get_reco_pos(self):
        return self.reco_pos_
    def get_true_dir(self):
        return self.true_dir_
    def get_true_label(self):
        return self.true_label_
    def get_supernova_tp_fraction(self):
        return self.supernova_tp_fraction_
    def get_min_distance_from_true_pos(self):
        return self.min_distance_from_true_pos_
    def set_variables(self, true_dir = np.array([0,0,0]), reco_pos = np.array([0,0,0]), true_label = 0, supernova_tp_fraction = 0, min_distance_from_true_pos = 0):
        self.true_dir_ = true_dir
        self.reco_pos_ = reco_pos
        self.true_label_ = true_label
        self.supernova_tp_fraction_ = supernova_tp_fraction
        self.min_distance_from_true_pos_ = min_distance_from_true_pos


def read_root_file_to_groups(filename):
        # Create a structured array with column names
    dt = np.dtype([('time_start', float), 
                    ('time_over_threshold', float),
                    ('time_peak', float),
                    ('channel', int),
                    ('adc_integral', int),
                    ('adc_peak', int),
                    ('detid', int),
                    ('type', int),
                    ('algorithm', int),
                    ('version', int),
                    ('flag', int)])

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
            if counter % 100 == 0:
                break
                print(f"Group {counter}")
            nrows = entry.nrows
            event = entry.event
            event_numbers.append(event )
            try:
                np_m = np.array(entry.matrix)
                print(f"Shape of the matrix: {np_m.shape}")
            except:
                print(f"Error reading the matrix for event {event}")
                continue
            m = np.empty((nrows), dtype=dt)
            # for j in range(nrows):
            #     m[j]["time_start"] = entry.matrix[j][0]
            #     m[j]["time_over_threshold"] = entry.matrix[j][1]
            #     m[j]["time_peak"] = entry.matrix[j][2]
            #     m[j]["channel"] = entry.matrix[j][3]
            #     m[j]["adc_integral"] = entry.matrix[j][4]
            #     m[j]["adc_peak"] = entry.matrix[j][5]
            #     m[j]["detid"] = entry.matrix[j][6]
            #     m[j]["type"] = entry.matrix[j][7]
            #     m[j]["algorithm"] = entry.matrix[j][8]
            #     m[j]["version"] = entry.matrix[j][9]
            #     m[j]["flag"] = entry.matrix[j][10]
            m["time_start"] = np_m[:,0]
            m["time_over_threshold"] = np_m[:,1]
            m["time_peak"] = np_m[:,2]
            m["channel"] = np_m[:,3]
            m["adc_integral"] = np_m[:,4]
            m["adc_peak"] = np_m[:,5]
            m["detid"] = np_m[:,6]
            m["type"] = np_m[:,7]
            m["algorithm"] = np_m[:,8]
            m["version"] = np_m[:,9]
            m["flag"] = np_m[:,10]

            this_group = Group(m)
            reco_pos = np.array([entry.reco_pos_x, entry.reco_pos_y, entry.reco_pos_z])
            true_dir = np.array([entry.true_dir_x, entry.true_dir_y, entry.true_dir_z])
            this_group.set_variables(true_dir = true_dir, reco_pos = reco_pos, true_label = entry.true_label, supernova_tp_fraction = entry.supernova_tp_fraction, min_distance_from_true_pos = entry.min_distance_from_true_pos)
            groups.append(this_group)
        break
    return groups, event_numbers

