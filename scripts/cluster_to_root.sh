# FILENAME=/afs/cern.ch/work/d/dapullia/public/dune/online-pointing-utils/lists/text_files/es-cc-bkg-truth.txt
# OUTFOLDER=/eos/user/d/dapullia/root_cluster_files/es-cc-bkg-truth/bkg
TICK_LIMITS=3
CHANNEL_LIMIT=1
MIN_TPS_TO_CLUSTER=1
PLANE=2
SUPERNOVA_OPTION=1 # 0 =  both, 1 = supernova, 2 = no supernova   - this is what we accept: 1 means we only accept TPs with label 1(SN)
MAIN_TRACK_OPTION=2 # 0 = both, 1 = main track, 2 = all but main track: blips and bkg  -- this is what we save
MAX_EVENTS_PER_FILENAME=1000 #
ADC_INTEGRAL_CUT=0 # if 0, no cut
# compile the code

REPO_HOME=$(git rev-parse --show-toplevel)
echo "REPO_HOME: ${REPO_HOME}"

cd ${REPO_HOME}/build
cmake ..
make -j $(nproc)
# check if the compilation was successful or stop
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

# TODO handle all these trough json

#MAIN TRACK
#input_file=/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/es-cc_lab.txt
#output_folder=/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc_lab/main_tracks/
#SUPERNOVA_OPTION=1
#MAIN_TRACK_OPTION=1
# already in build
# execution_command="./app/cluster_to_root -f $input_file -o $output_folder --ticks-limit $TICK_LIMITS --channel-limit $CHANNEL_LIMIT --min-tps-to-cluster $MIN_TPS_TO_CLUSTER --plane $PLANE --supernova-option $SUPERNOVA_OPTION --main-track-option $MAIN_TRACK_OPTION --max-events-per-filename $MAX_EVENTS_PER_FILENAME --adc-integral-cut $ADC_INTEGRAL_CUT"
# echo "Now running: ${execution_command}"
# eval ${execution_command}
# # ///

#BLIPS
#input_file=/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/es-cc_lab.txt
#output_folder=/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc_lab/blips/
#SUPERNOVA_OPTION=1
#MAIN_TRACK_OPTION=2
#This option means we accept the TPs with the SN label, and save everything but the main track. So its a supernova event but only saving the blips 

# # already in build
# execution_command="./app/cluster_to_root -f $input_file -o $output_folder --ticks-limit $TICK_LIMITS --channel-limit $CHANNEL_LIMIT --min-tps-to-cluster $MIN_TPS_TO_CLUSTER --plane $PLANE --supernova-option $SUPERNOVA_OPTION --main-track-option $MAIN_TRACK_OPTION --max-events-per-filename $MAX_EVENTS_PER_FILENAME --adc-integral-cut $ADC_INTEGRAL_CUT"
# echo "Now running: ${execution_command}"
# eval ${execution_command}
# # ///

#BACKGROUND
input_file=/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/es-cc-bkg-truth.txt
output_folder=/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth/X/bkg/
SUPERNOVA_OPTION=2
MAIN_TRACK_OPTION=0
#Basically saving only backgrounds and random blips(not from supernova)

# # already in build
# execution_command="./app/cluster_to_root -f $input_file -o $output_folder --ticks-limit $TICK_LIMITS --channel-limit $CHANNEL_LIMIT --min-tps-to-cluster $MIN_TPS_TO_CLUSTER --plane $PLANE --supernova-option $SUPERNOVA_OPTION --main-track-option $MAIN_TRACK_OPTION --max-events-per-filename $MAX_EVENTS_PER_FILENAME --adc-integral-cut $ADC_INTEGRAL_CUT"
# echo "Now running: ${execution_command}"
# eval ${execution_command}

#input_file=/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/es-cc-bkg-truth.txt 
#output_folder=/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth
#SUPERNOVA_OPTION=1
#MAIN_TRACK_OPTION=1

input_file=/afs/cern.ch/work/h/hakins/private/online-pointing-utils/lists/text_files/background_file_combined.txt
output_folder=/afs/cern.ch/work/h/hakins/private/root_cluster_files/es-cc-bkg-truth/
SUPERNOVA_OPTION=0
MAIN_TRACK_OPTION=0


# already in bui
execution_command="./app/cluster_to_root -f $input_file -o $output_folder --ticks-limit $TICK_LIMITS --channel-limit $CHANNEL_LIMIT --min-tps-to-cluster $MIN_TPS_TO_CLUSTER --plane $PLANE --supernova-option $SUPERNOVA_OPTION --main-track-option $MAIN_TRACK_OPTION --max-events-per-filename $MAX_EVENTS_PER_FILENAME --adc-integral-cut $ADC_INTEGRAL_CUT"
echo "Now running: ${execution_command}"
eval ${execution_command}


cd ${REPO_HOME}/scripts/