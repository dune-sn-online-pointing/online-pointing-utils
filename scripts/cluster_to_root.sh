# FILENAME=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/text_files/es-cc-bkg-truth.txt
# OUTFOLDER=/eos/user/d/dapullia/root_cluster_files/es-cc-bkg-truth/bkg
TICK_LIMITS=3
CHANNEL_LIMIT=1
MIN_TPS_TO_CLUSTER=1
PLANE=2
SUPERNOVA_OPTION=1 # 0 =  both, 1 = supernova, 2 = no supernova
MAIN_TRACK_OPTION=0 # 0 = both, 1 = main track, 2 = all but main track
MAX_EVENTS_PER_FILENAME=1000 #
ADC_INTEGRAL_CUT=0 # if 0, no cut
# compile the code

cd ../app/cluster_to_root
make clean
make

./cluster_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/text_files/cc_lab.txt /eos/user/d/dapullia/root_cluster_files/eliminami/ $TICK_LIMITS $CHANNEL_LIMIT $MIN_TPS_TO_CLUSTER $PLANE $SUPERNOVA_OPTION $MAIN_TRACK_OPTION $MAX_EVENTS_PER_FILENAME $ADC_INTEGRAL_CUT
# ./cluster_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/text_files/es_lab.txt /eos/user/d/dapullia/root_cluster_files/es-lab/ $TICK_LIMITS $CHANNEL_LIMIT $MIN_TPS_TO_CLUSTER $PLANE $SUPERNOVA_OPTION $MAIN_TRACK_OPTION $MAX_EVENTS_PER_FILENAME $ADC_INTEGRAL_CUT
# ./cluster_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/text_files/es-cc-bkg-truth.txt /eos/user/d/dapullia/root_cluster_files/es-cc-bkg-truth/bkg/ $TICK_LIMITS $CHANNEL_LIMIT $MIN_TPS_TO_CLUSTER $PLANE $SUPERNOVA_OPTION $MAIN_TRACK_OPTION $MAX_EVENTS_PER_FILENAME $ADC_INTEGRAL_CUT


# ./cluster_to_root.exe /afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/text_files/all_es_dir_list_2.txt /eos/user/d/dapullia/root_cluster_files/all_es_dir_list_2/ $TICK_LIMITS $CHANNEL_LIMIT $MIN_TPS_TO_CLUSTER $PLANE $SUPERNOVA_OPTION $MAIN_TRACK_OPTION $MAX_EVENTS_PER_FILENAME $ADC_INTEGRAL_CUT

cd ../../bash_scripts

