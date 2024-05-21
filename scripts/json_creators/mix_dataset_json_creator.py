import argparse
import json

def main(args):
    
    #This script will create the text files that the json file then references
    
    # Define variables
    base_path = "/afs/cern.ch/work/h/hakins/private/npy_dataset"
    types = ["es-cc_lab/main_track", "es-cc_lab/blip", "es-cc-bkg-truth"]
    dataset_path_img = f"{args.cut}dataset/dataset_img.npy"
    dataset_path_dir = f"{args.cut}dataset/dataset_label_true_dir.npy"

    # Generate paths
    paths_img = [f"{base_path}/{type}/{dataset_path_img}" for type in types]
    paths_dir = [f"{base_path}/{type}/{dataset_path_dir}" for type in types]


    # Write paths to a text file
    output_file_img = f"/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/npy_files/img/ds-mix-mt-vs-all_{args.cut}.txt"
    with open(output_file_img, "w") as file:
        for path in paths_img:
            file.write(path + "\n")
    output_file_dir = f"/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/npy_files/true_dir/ds-mix-mt-vs-all_{args.cut}.txt"
    with open(output_file_dir, "w") as file:
        for path in paths_dir:
            file.write(path + "\n")

    print("Img Text file created successfully:", output_file_img)
    print("Dir Text file created successfully:", output_file_dir)

    
    

    

    # Define the data for the JSON file
    data = {
    "datasets_img": f"/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/npy_files/img/ds-mix-mt-vs-all_{args.cut}.txt",
    "datasets_process_label": f"/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/npy_files/process/ds-mix-mt-vs-all.txt",
    "datasets_true_dir_label": f"/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/npy_files/true_dir/ds-mix-mt-vs-all_{args.cut}.txt",
    "remove_process_labels": [99],
    "shuffle": 1,
    "balance": 1,
    "cut": args.cut
}

    # Write the data to the JSON file
    with open(args.output_file, "w") as json_file:
        json.dump(data, json_file, indent=4)

    print("JSON file created successfully at:", args.output_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a JSON configuration file")
    parser.add_argument("--cut", type=int, help="ADC cut being made", required=True)
    parser.add_argument("--output_file", type=str, help="Path to the output JSON file", required=True)
    args = parser.parse_args()
    
    main(args)
