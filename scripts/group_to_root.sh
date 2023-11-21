FILENAME=/eos/home-e/evilla/dune/sn-data/prodmarley_nue_spectrum_radiological_decay0_dune10kt_refactored_1x2x6_modified-TPdump_standardHF_noiseless_fixed-30events/tpstream_standardHF_thresh30_nonoise.txt
OUTFOLDER=/afs/cern.ch/work/d/dapullia/public/dune/dataset_study/
TICK_LIMITS=(3 5 10 20)
CHANNEL_LIMIT=(10 25 50 100)
MIN_TPS_TO_GROUP=(1 1 1 1)

# parse aarguments with flags


# compile the code


cd ../cpp/grouper_to_root
make clean
make
# cycle over the different tick limits and channel limits
for (( i=0; i<${#TICK_LIMITS[@]}; i++ ));
do
    ./grouper_to_root.exe $FILENAME $OUTFOLDER ${TICK_LIMITS[$i]} ${CHANNEL_LIMIT[$i]} ${MIN_TPS_TO_GROUP[$i]} 
done
cd ../../scripts


