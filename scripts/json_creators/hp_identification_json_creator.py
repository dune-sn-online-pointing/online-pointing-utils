import argparse
import json

def main(args):
    # Define the data for the JSON file


    data = {
    "input_data": f"/afs/cern.ch/work/h/hakins/private/npy_dataset/ds-mix-mt-vs-all/dataset_img_{args.cut}.npy",
    "input_label": f"/afs/cern.ch/work/h/hakins/private/npy_dataset/ds-mix-mt-vs-all/dataset_label_process_{args.cut}.npy",
    "output_folder": "/",
    "model_name": "hyperopt_simple_cnn",
    "load_model": False,
    "dataset_parameters": {
        "remove_y_direction": 0, 
        "train_fraction": 0.8,
        "val_fraction": 0.1,
        "test_fraction": 0.1,
        "aug_coefficient": 1,
        "prob_per_flip": 0.5
    },
    "model_parameters": {
        "input_shape": [250, 40, 1],   
        "hp_max_evals": 1, 
        "space_options": {
            "n_conv_layers": [1, 2, 3, 4],
            "n_dense_layers": [2, 3, 4],
            "n_filters": [16, 32, 64],
            "kernel_size": [1, 3, 5],
            "n_dense_units": [32, 64, 128],
            "learning_rate": [0.0001, 0.001],
            "decay_rate": [0.90, 0.999]
        }
    }
}

    # Write the data to the JSON file
    with open(args.output_file, "w") as json_file:
        json.dump(data, json_file, indent=4)

    print("JSON file created successfully at:", args.output_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a JSON configuration file")
    parser.add_argument("--cut", type=int, help="ADC cut being made", required=True)
    #parser.add_argument("--type",type=str, help="Type of event", required=True)
    parser.add_argument("--output_file", type=str, help="Path to the output JSON file", required=True)

    args = parser.parse_args()
    
    main(args)
