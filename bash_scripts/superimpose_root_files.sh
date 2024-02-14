FILENAME_SIG=/eos/user/d/dapullia/root_group_files/es-lab/X/groups_tick_limits_3_channel_limits_1_min_tps_to_group_1.root
FILENAME_BKG=/eos/user/d/dapullia/root_group_files/es-cc-bkg-truth/bkg/X/groups_tick_limits_3_channel_limits_1_min_tps_to_group_1.root
OUTFOLDER=/eos/user/d/dapullia/root_group_files/superimposed_files/es-lab-bkg/
RADIUS=1


# compile the code

cd ../app/superimpose_root_files
# make clean
make
./superimpose_root_files.exe $FILENAME_SIG $FILENAME_BKG $OUTFOLDER $RADIUS

cd ../../bash_scripts

