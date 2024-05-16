#!/bin/bash
echo "*****************************************************************************"
echo "Running"

cut=20000
#Create loop that adds 20000 to cut every iteration until 200000
while [ $cut -le 200000 ]
do
    echo "Current cut value: $cut"
    cd /afs/cern.ch/work/h/hakins/private/online-pointing-utils/scripts
    #Main track dataset
    echo "Main track cluster to root"
    ./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/main_tracks/main_track_${cut}.json
    echo "Main track cluster to dataset"
    ./clusters_to_dataset.sh 0
    #blips
    echo "Blips cluster to root"
    ./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/blips/blips_${cut}.json
    echo "Blips cluster to dataset"
    ./clusters_to_dataset.sh 1
    #bkg
    echo "BKG cluster to root"
    ./cluster_to_root.sh --input_file /afs/cern.ch/work/h/hakins/private/json/cluster_to_root/bkg/bkg_${cut}.json
    echo "BKG cluster to dataset"
    ./clusters_to_dataset.sh 2

    #mix datasets
    echo "Mixing Dataset"
    ./mix_datasets.sh

    #create output folder for model if not already there
    OUTPUT_FOLDER=/eos/user/h/hakins/dune/ML/mt_identifier/ds-mix-mt-vs-all/${cut}/
    if [ ! -d "$OUTPUT_FOLDER" ]; then
            mkdir -p "$OUTPUT_FOLDER"
        fi
    #train model with hyperopt 
    echo "running run_mt_identifier"
    cd /afs/cern.ch/work/h/hakins/private/ml_for_pointing/scripts
    source run_mt_identifier.sh --output_folder ${OUTPUT_FOLDER}
 
    
    # Increment cut by 20000 for the next iteration
    ((cut += 20000))
done