import argparse
import json

def main(args):
    # Define the data for the JSON file
# Define the data for the JSON file
    input_file = "dummy" #dummy
    output_folder = "dummy"

    if args.type == "main_track":
        print("MAIN TRACK")
        input_file = "/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/es-cc_lab.txt"
        output_folder = "/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc_lab/main_tracks/"
    elif args.type == "blip":
        print("Blip")
        input_file = "/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/es-cc_lab.txt"
        output_folder="/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc_lab/blips/"
    elif args.type == "bkg":
        print("BKG")
        input_file ="/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/es-cc-bkg-truth.txt"
        output_folder = "/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth/X/bkg/"

    data = {
    "filename": input_file,
    "output_folder": output_folder,
    "supernova_option": 1,
    "main_track_option": 1,
    "tick_limit": 3,
    "channel_limit": 1,
    "min_tps_to_cluster": 1,
    "plane": 2,
    "max_events_per_filename": 1000,
    "adc_integral_cut": args.cut
}

    


    # Write the data to the JSON file
    with open(args.output_file, "w") as json_file:
        json.dump(data, json_file, indent=4)

    print("JSON file created successfully at:", args.output_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a JSON configuration file")
    parser.add_argument("--cut", type=int, help="ADC cut being made", required=True)
    parser.add_argument("--type", type=str, help="type (main_track, blip, bkg)", required=True)
    parser.add_argument("--output_file", type=str, help="Path to the output JSON file", required=True)

    args = parser.parse_args()
    
    main(args)
