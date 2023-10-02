INPUT_FILE=tpstream_run020638_0000_tpwriter_tpswriter_20230314T222757.txt
INPUT_PATH=/eos/user/d/dapullia/tp_dataset/
CHANMAP=vdcbce_chanmap_v4.txt
SHOW=false
SAVE_IMG=false
SAVE_DS=true
WRITE=true
SAVE_PATH=/eos/user/d/dapullia/tp_dataset/tpstream-cb/
IMG_SAVE_FOLDER=images/
IMG_SAVE_NAME=image
N_EVENTS=1000000
MIN_TPS_TO_CLUSTER=6

# if show is true then add --show
if [ "$SHOW" = true ] ; then
    SHOW_FLAG="--show"
else
    SHOW_FLAG=""
fi

# if save_img is true then add --save_img
if [ "$SAVE_IMG" = true ] ; then
    SAVE_IMG_FLAG="--save_img"
else
    SAVE_IMG_FLAG=""
fi

# if save_ds is true then add --save_ds
if [ "$SAVE_DS" = true ] ; then
    SAVE_DS_FLAG="--save_ds"
else
    SAVE_DS_FLAG=""
fi

# if write is true then add --write
if [ "$WRITE" = true ] ; then
    WRITE_FLAG="--write"
else
    WRITE_FLAG=""
fi

python main_tp_to_image.py --input_file $INPUT_FILE --input_path $INPUT_PATH --chanmap $CHANMAP --save_path $SAVE_PATH --img_save_folder $IMG_SAVE_FOLDER --img_save_name $IMG_SAVE_NAME --n_events $N_EVENTS --min_tps_to_cluster $MIN_TPS_TO_CLUSTER $SHOW_FLAG $SAVE_IMG_FLAG $SAVE_DS_FLAG $WRITE_FLAG
