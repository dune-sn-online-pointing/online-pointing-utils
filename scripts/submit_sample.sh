#!/bin/bash

# Initialize env variables
SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export SCRIPTS_DIR
export HOME_DIR=$(dirname $SCRIPTS_DIR)
source $HOME_DIR/scripts/init.sh


# parser
print_help() {
    echo "Usage: $0 -w <which-interaction> -f <first-job> -l <last-job> [OPTIONS]"
    echo " -j, --json-config          Path to the JSON configuration file"
    echo "  -w, --which                 Which interaction (es or cc)"
    echo "  --tot-files                 Total number of files to process (default: 5000)"
    echo "  --max-files                 Maximum number of files to process per job (default: 100)"
    echo "  -d, --delete-submit-files   Delete submit files after submission (default: false)"
    echo "  --delete-root               Delete root files (default: true)"
    echo "  -p, --print-only            Dry run: print the submit file without submitting"
    echo "  -h, --help                  Display this help message"
    exit 1
}

# init
delete_submit_files=false
delete_root_files=true
tot_files=4500
max_files=10
skip=0

which_sims="--all"

# parse
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -j|--json-config) JSON_SETTINGS="$2"; shift ;;
        --tot-files) tot_files="$2"; shift ;;
        --max-files) max_files="$2"; shift ;;
        -d|--delete-submit-files) delete_submit_files="$2"; shift ;;
        --delete-root) delete_root_files="$2"; shift ;;
        -p|--print-only) print_only=true ;;
        -h|--help) print_help ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done


max_jobs=$(( (tot_files + max_files - 1) / max_files ))

# Validate required parameters
if [ -z "$JSON_SETTINGS" ]; then
    echo "Error: Missing required parameters!"
    echo "  -j/--json-config is required"
    print_help
fi


echo "Looking for settings file $JSON_SETTINGS. If execution stops, it means that the file was not found."
findSettings_command="$SCRIPTS_DIR/findSettings.sh -j $JSON_SETTINGS"
echo "Using command: $findSettings_command"
# last line of the output of findSettings.sh is the full path of the settings file
JSON_SETTINGS=$( $findSettings_command | tail -n 1)
echo -e "Settings file found, full path is: $JSON_SETTINGS \n"


username=$(whoami | cut -d' ' -f1)
user_email="$username@cern.ch"

output_folder="$HOME_DIR/condor/${username}/"

JOB_OUTPUT_DIR=${HOME_DIR}/job_output/
echo "Job output folder: $JOB_OUTPUT_DIR"
mkdir -p $JOB_OUTPUT_DIR

mkdir -p $output_folder
echo "Output folder: $output_folder"

list_of_jobs="${output_folder}list_of_jobs.txt"

# generate list of arguments and put them in a file, but delete the file first to avoid issues
rm -f $list_of_jobs
touch $list_of_jobs
for i in $(seq 1 $max_jobs); do
    skip=$(( (i - 1) * max_files ))
    echo "Adding job $i with skip $skip"
    echo "-j ${JSON_SETTINGS} --home-dir ${HOME_DIR} --no-compile $which_sims --skip-files $skip --max-files $max_files" >> $list_of_jobs
done

echo "List of jobs:"
cat $list_of_jobs


# generate a submit file for condor 
jobname=$(basename "$JSON_SETTINGS" | sed 's/\.[^.]*$//')
submit_file="${output_folder}submit_processing_${jobname}.sub"
echo "Creating submit file: $submit_file"

cat <<EOF > $submit_file
# script to submit jobs to the grid for sn simulation

notify_user         = ${user_email}
notification        = Error

JOBNAME             = ${jobname}
executable          = ${HOME_DIR}/scripts/sequence.sh
# using the arguments from below, not this line
output              = ${JOB_OUTPUT_DIR}job.\$(ClusterId).\$(ProcId).\$(JOBNAME).out
error               = ${JOB_OUTPUT_DIR}job.\$(ClusterId).\$(ProcId).\$(JOBNAME).err
log                 = ${JOB_OUTPUT_DIR}job.\$(ClusterId).\$(JOBNAME).log
# request_cpus        = 1
# request_memory      = 2000

+JobFlavour         = "workday"

queue arguments from ${list_of_jobs}
EOF

# submit the job to the grid
if [ "$print_only" = true ]; then
    echo "Print only mode enabled. Submit file content:"
    cat $submit_file
    echo -e "\nList of jobs:"
    cat $list_of_jobs
    exit 0
else 
    echo "Submitting all jobs from ${skip} to $((skip + max_files)) for ${JSON_SETTINGS}"
    condor_submit $submit_file
    if [ "$delete_submit_files" = true ]; then
        rm $submit_file
        rm $list_of_jobs
    fi
    echo "Submission done."
fi