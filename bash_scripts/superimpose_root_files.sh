# FILENAME_SIG=/eos/user/d/dapullia/root_cluster_files/es-lab/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root
FILENAME_BKG=/eos/user/d/dapullia/root_cluster_files/es-cc-bkg-truth/bkg/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root
# OUTFOLDER=/eos/user/d/dapullia/root_cluster_files/superimposed_files/es-lab-bkg/
RADIUS=50


# compile the code

cd ../app/superimpose_root_files
# make clean
make
# ./superimpose_root_files.exe $FILENAME_SIG $FILENAME_BKG $OUTFOLDER $RADIUS
rm -rf /eos/user/d/dapullia/root_cluster_files/superimposed_files/cc-lab-bkg/
./superimpose_root_files.exe /eos/user/d/dapullia/root_cluster_files/cc-lab/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root $FILENAME_BKG /eos/user/d/dapullia/root_cluster_files/superimposed_files/cc-lab-bkg/ $RADIUS
rm -rf /eos/user/d/dapullia/root_cluster_files/superimposed_files/es-lab-bkg/
./superimpose_root_files.exe /eos/user/d/dapullia/root_cluster_files/es-lab/X/clusters_tick_limits_3_channel_limits_1_min_tps_to_cluster_1.root $FILENAME_BKG /eos/user/d/dapullia/root_cluster_files/superimposed_files/es-lab-bkg/ $RADIUS

cd ../../bash_scripts

