FILENAME=/eos/home-e/evilla/dune/sn-data/tpstream_standardHF_thresh30_nonoise_newLabels_23evts.txt
OUTFOLDER=/afs/cern.ch/work/d/dapullia/public/dune/dataset_study/new_labels/
# OUTNAME=tpstream_standardHF_thresh30_nonoise.root
# TICK_LIMITS=(2 5 10 25 50 100)
# CHANNEL_LIMIT=(1 2 3 5 10 20)
# MIN_TPS_TO_GROUP=(1 1 1 1)
# PLANE=(0 1 2)
# USE_ONLY_SUPERNOVA=1
TICK_LIMITS=10
CHANNEL_LIMIT=3
MIN_TPS_TO_GROUP=2
PLANE=2
USE_ONLY_SUPERNOVA=0

# parse aarguments with flags


# compile the code


cd ../app/grouper_to_root
make clean
make
# cycle over the different tick limits and channel limits
for (( j=0; j<${#PLANE[@]}; j++ ));
do
    for (( i=0; i<${#TICK_LIMITS[@]}; i++ ));
    do
        ./grouper_to_root.exe $FILENAME $OUTFOLDER ${TICK_LIMITS[$i]} ${CHANNEL_LIMIT[$i]} ${MIN_TPS_TO_GROUP[$i]} ${PLANE[$j]} $USE_ONLY_SUPERNOVA
    done
done
cd ../../scripts


