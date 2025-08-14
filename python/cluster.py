import numpy as np
import sys
import ROOT
import functools

class cluster: # Only good for HD geometry 1x2x6
    def __init__(self, tps):
        self.tps_ = tps
        self.true_dir_ = None
        self.reco_pos_ = None
        self.n_tps_ = tps.shape[0]
        self.min_distance_from_true_pos_ = None
        self.true_label_ = None
        self.supernova_tp_fraction_ = None
        self.true_event_number_ = None
        self.true_neutrino_energy_ = None
        self.true_particle_energy_ = None  
        self.true_interaction_ = None
        self.total_charge_ = None
        self.total_energy_ = None

    def __len__(self):
        return self.n_tps_

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

    def get_true_event_number(self):
        return self.true_event_number_

    def get_true_neutrino_energy(self):
        return self.true_neutrino_energy_

    def get_true_particle_energy(self):  
        return self.true_particle_energy_

    def get_true_interaction(self):
        return self.true_interaction_

    def get_total_charge(self):
        return self.total_charge_

    def get_total_energy(self):
        return self.total_energy_

    def set_variables(self, true_dir=np.array([0, 0, 0]), reco_pos=np.array([0, 0, 0]), true_label=0, 
                      supernova_tp_fraction=0, min_distance_from_true_pos=0, true_event_number=0, 
                      true_neutrino_energy=0, true_particle_energy=0, true_interaction=0, total_charge=0, total_energy=0):   
        
        self.true_dir_ = true_dir
        self.reco_pos_ = reco_pos
        self.true_label_ = true_label
        self.supernova_tp_fraction_ = supernova_tp_fraction
        self.min_distance_from_true_pos_ = min_distance_from_true_pos
        self.true_event_number_ = true_event_number
        self.true_neutrino_energy_ = true_neutrino_energy
        self.true_particle_energy_ = true_particle_energy  
        self.true_interaction_ = true_interaction
        self.total_charge_ = total_charge
        self.total_energy_ = total_energy

@functools.lru_cache(maxsize=None)
def read_root_file_to_clusters(filename):
    # Create a structured array with column names
    dt = np.dtype([('time_start', int),
                     ('samples_over_threshold', int),
                     ('samples_to_peak', int),
                     ('detector_channel', int),
                     ('detector', int),
                     ('adc_integral', int),
                     ('adc_peak', int)])    
                   
    file = ROOT.TFile.Open(filename, "READ")
    clusters = []
    event_numbers = []
    for i in file.GetListOfKeys():
        # get the TTree
        tree = file.Get(i.GetName())
        print(f"Tree name: {tree.GetName()}")
        tree.Print()
        counter = 0
        for entry in tree:
            counter += 1
            if counter % 1000 == 0:
                print(f"cluster {counter}")
            nrows = entry.nrows
            event = entry.event
            event_numbers.append(event)
            tps = np.empty((nrows), dtype=dt)
            for j in range(nrows):
                tps[j]["time_start"] = entry.tp_time_start[j]
                tps[j]["samples_over_threshold"] = entry.tp_samples_over_threshold[j]
                tps[j]["samples_to_peak"] = entry.tp_samples_to_peak[j]
                tps[j]["detector_channel"] = entry.tp_detector_channel[j]
                tps[j]["detector"] = entry.tp_detector[j]
                tps[j]["adc_integral"] = entry.tp_adc_integral[j]
                tps[j]["adc_peak"] = entry.tp_adc_peak[j]
            # create the cluster
            this_cluster = cluster(tps) # constructor of the cluster
            reco_pos = np.array([entry.reco_pos_x, entry.reco_pos_y, entry.reco_pos_z])
            true_dir = np.array([entry.true_dir_x, entry.true_dir_y, entry.true_dir_z])
            this_cluster.set_variables(
                true_dir=true_dir, 
                reco_pos=reco_pos, 
                true_label=entry.true_label, 
                supernova_tp_fraction=entry.supernova_tp_fraction, 
                min_distance_from_true_pos=entry.min_distance_from_true_pos, 
                true_event_number=event,
                true_neutrino_energy=entry.true_neutrino_energy,
                true_particle_energy=entry.true_particle_energy,  
                true_interaction=entry.true_interaction,
                total_charge=entry.total_charge,
                total_energy=entry.total_energy
            )
            clusters.append(this_cluster)
        break
    return clusters, event_numbers
