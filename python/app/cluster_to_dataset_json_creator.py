import argparse
import json

def main(args):
    # Define the data for the JSON file
    input_file = "dummy" #dummy

    if args.type == "main_track":
        print("MAIN TRACK")
        input_file = f"/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc_lab/main_tracks/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1_cut_{args.cut}.root"
    elif args.type == "blip":
        print("Blip")
        input_file = f"/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc_lab/blips/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1_cut_{args.cut}.root"
    elif args.type == "bkg":
        print("BKG")
        input_file =f"/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth/X/bkg/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1_cut_{args.cut}.root"
    elif args.type == "benchmark":
        print("Benchmark")
        input_file =f"/afs/cern.ch/work/h/hakins/private/root_cluster_files/benchmark/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1_cut_{args.cut}.root"

    data = {
        "input_file": input_file,
        "make_fixed_size": True,
        "img_width": 40,
        "img_height": 250,
        "min_tps_to_cluster": 1,
        "drift_direction": 0,
        "save_img_dataset": 1,
        "save_label_process": 1,
        "save_label_direction": 1,
        "x_margin" : 2,
        "y_margin" : 5
    }

    # Write the data to the JSON file
    with open(args.output_file, "w") as json_file:
        json.dump(data, json_file, indent=4)

    print("JSON file created successfully at:", args.output_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a JSON configuration file")
    parser.add_argument("--cut", type=int, help="ADC cut being made", required=True)
    parser.add_argument("--type",type=str, help="Type of event", required=True)
    parser.add_argument("--output_file", type=str, help="Path to the output JSON file", required=True)

    args = parser.parse_args()
    
    main(args)
