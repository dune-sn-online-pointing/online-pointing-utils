# FILENAME=/eos/home-e/evilla/dune/sn-data/tpstream_standardHF_thresh30_nonoise_newLabels_23evts.txt
# FILENAME=/eos/home-e/evilla/dune/sn-data/prodmarley_nue_flat_dune10kt_1x2x6_modified-TPdump_standardHF_noiseless_fixed-100events/tpstream_standardHF_thresh30_nonoise.txt
# FILENAME=/eos/user/e/evilla/dune/sn-data/prodmarley_nue_spectrum_radiological_decay0_dune10kt_refactored_1x2x6_modified-TPdump_standardHF_noiseless_fixed-30events/tpstream_standardHF_thresh30_nonoise.txt
# OUTFOLDER=/eos/user/d/dapullia/tp_dataset/emaprod/new_lab_multirun/
# OUTFOLDER=/afs/cern.ch/work/d/dapullia/public/dune/dataset_study/bkg_and_sn/
# OUTFOLDER=/afs/cern.ch/work/d/dapullia/public/dune/dataset_study/only_sn/
# OUTNAME=tpstream_standardHF_thresh30_nonoise.root
# TICK_LIMITS=(5 5 10 25 50 100)
# CHANNEL_LIMIT=(1 2 3 5 10 20)
# MIN_TPS_TO_GROUP=(1 1 1 1 1 1)
# PLANE=(0 1 2)
# USE_ONLY_SUPERNOVA=0
TICK_LIMITS=(5)
CHANNEL_LIMIT=(2)
MIN_TPS_TO_GROUP=(4)
PLANE=(2)
SUPERNOVA_OPTION=(0) # 0 =  both, 1 = supernova, 2 = no supernova
MAIN_TRACK_OPTION=(0) # 0 = both, 1 = main track, 2 = all but main track
MAX_EVENTS_PER_FILENAME=150 # if 0, no limit

# compile the code

cd ../app/grouper_to_root
make clean
make
# cycle over the different tick limits and channel limits
# for (( k=0; k<10; k++ ));
# do    
    # for (( j=0; j<${#PLANE[@]}; j++ ));
    # do
    #     for (( i=0; i<${#TICK_LIMITS[@]}; i++ ));
    #     do
    #         # ./grouper_to_root.exe $FILENAME $OUTFOLDER ${TICK_LIMITS[$i]} ${CHANNEL_LIMIT[$i]} ${MIN_TPS_TO_GROUP[$i]} ${PLANE[$j]} $USE_ONLY_SUPERNOVA
    #         ./grouper_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/app/input-files/cc-bkg.txt /eos/user/d/dapullia/tp_dataset/emaprod/cc-bkg/ ${TICK_LIMITS[$i]} ${CHANNEL_LIMIT[$i]} ${MIN_TPS_TO_GROUP[$i]} ${PLANE[$j]} 0
    #         ./grouper_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/app/input-files/es-bkg.txt /eos/user/d/dapullia/tp_dataset/emaprod/es-bkg/ ${TICK_LIMITS[$i]} ${CHANNEL_LIMIT[$i]} ${MIN_TPS_TO_GROUP[$i]} ${PLANE[$j]} 0
    #         ./grouper_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/app/input-files/cc.txt /eos/user/d/dapullia/tp_dataset/emaprod/cc/ ${TICK_LIMITS[$i]} ${CHANNEL_LIMIT[$i]} ${MIN_TPS_TO_GROUP[$i]} ${PLANE[$j]} 1
    #         ./grouper_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/app/input-files/es.txt /eos/user/d/dapullia/tp_dataset/emaprod/es/ ${TICK_LIMITS[$i]} ${CHANNEL_LIMIT[$i]} ${MIN_TPS_TO_GROUP[$i]} ${PLANE[$j]} 1

    #     done
    # done
# done
# Usage: grouper_to_root <filename> <outfolder> <ticks_limit> <channel_limit> <min_tps_to_group> <plane> <supernova_option> <main_track_option>

# ./grouper_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/app/input-files/cc-all_dir.txt /eos/user/d/dapullia/tp_dataset/emaprod/all_es_dir_list_2/ 3 1 1 2 1 1 0 0
./grouper_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/app/input-files/all_es_dir_list_2.txt /eos/user/d/dapullia/tp_dataset/emaprod/all_es_dir_list_2/main-tracks/ 3 1 1 2 1 1 0 0


cd ../../scripts

