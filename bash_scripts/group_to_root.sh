FILENAME=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/text_files/es-cc-bkg-truth.txt
OUTFOLDER=/eos/user/d/dapullia/root_group_files/es-cc-bkg-truth/bkg
TICK_LIMITS=3
CHANNEL_LIMIT=1
MIN_TPS_TO_GROUP=1
PLANE=2
SUPERNOVA_OPTION=2 # 0 =  both, 1 = supernova, 2 = no supernova
MAIN_TRACK_OPTION=0 # 0 = both, 1 = main track, 2 = all but main track
MAX_EVENTS_PER_FILENAME=1000 # if 0, no limit
ADC_INTEGRAL_CUT=0 # if 0, no cut
# compile the code

cd ../app/group_to_root
# make clean
make
./group_to_root.exe $FILENAME $OUTFOLDER $TICK_LIMITS $CHANNEL_LIMIT $MIN_TPS_TO_GROUP $PLANE $SUPERNOVA_OPTION $MAIN_TRACK_OPTION $MAX_EVENTS_PER_FILENAME $ADC_INTEGRAL_CUT

cd ../../bash_scripts

