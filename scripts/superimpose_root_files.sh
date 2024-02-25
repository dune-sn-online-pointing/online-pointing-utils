FILENAME_SIG_CC=/eos/user/d/dapullia/root_groups_files/cc-lab/X/groups_tick_limits_3_channel_limits_1_min_tps_to_group_1.root
FILENAME_SIG_ES='/eos/user/d/dapullia/root_groups_files/es-lab/X/groups_tick_limits_3_channel_limits_1_min_tps_to_group_1.root'
FILENAME_BKG=/eos/user/d/dapullia/root_group_files/es-cc-bkg-truth/bkg/X/groups_tick_limits_3_channel_limits_1_min_tps_to_group_1.root
# OUTPUT_FOLDER=/eos/user/d/dapullia/root_cluster_files/superimposed_files/es-lab-bkg/
OUTPUT_FOLDER=/eos/user/e/evilla/dune/

RADIUS=50 # mention units somewhere TODO

REPO_HOME=$(git rev-parse --show-toplevel)
echo "REPO_HOME: ${REPO_HOME}"

# compile
echo "Compiling..."
cd ${REPO_HOME}/build/
cmake ..
make -j $(nproc)
# if successful, run the app
if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

# Run the app
# CC events
# this is a bit dangerous, can it be avoided?
# rm -rf $OUTPUT_FOLDER
./app/superimpose_root_files -s $FILENAME_SIG_CC -b $FILENAME_BKG -o $OUTPUT_FOLDER -r $RADIUS

# ES events
# this is a bit dangerous, can it be avoided?
# rm -rf $OUTPUT_FOLDER
#./superimpose_root_files -s $FILENAME_SIG_ES -b $FILENAME_BKG -o $OUTPUT_FOLDER -r $RADIUS

cd ${REPO_HOME}/scripts/
