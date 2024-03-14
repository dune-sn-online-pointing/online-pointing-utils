FILENAME=/afs/cern.ch/work/d/dapullia/public/dune/data-selection-pipeline/output/2_test/clustering/cc-lab/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root
OUTPUT_FOLDER=/eos/user/d/dapullia/root_cluster_files/eliminami/
# OUTPUT_FOLDER=/eos/user/e/evilla/dune/
PREDICTIONS=/afs/cern.ch/work/d/dapullia/public/dune/data-selection-pipeline/output/2_test/mt_id/cc-lab/predictions.txt
RADIUS=50 # mention units somewhere TODO
THRESHOLD=0.5

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
./app/aggregate_clusters_within_volume -f ${FILENAME} -o ${OUTPUT_FOLDER} -r ${RADIUS} -p ${PREDICTIONS} -t ${THRESHOLD}
cd ${REPO_HOME}/scripts/
