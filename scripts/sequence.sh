#!/bin/bash

export SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
# Use absolute path to init.sh from AFS when running on worker nodes
if [[ -f "${SCRIPTS_DIR}/init.sh" ]]; then
    source ${SCRIPTS_DIR}/init.sh
else
    # Fallback: reconstruct path from script location in AFS
    REPO_DIR="$(dirname "${SCRIPTS_DIR}")"
    source ${REPO_DIR}/scripts/init.sh
fi


print_help(){
    echo "Usage: $0 -s <sample> [-bt] [-ab] [-mc] [-ac] [-gi] [--no-compile] [--clean-compile]";
    echo "Options:";
    echo "  --no-compile                Do not recompile the code"
    echo "  --clean-compile             Clean and recompile the code"
    echo "  -j|--json-settings <json>   Global json file including all steps. Overrides individual jsons if parsed"
    echo "  --home-dir <path>          Path to the home directory (overrides default)"
    echo "  --max-files <n>             Maximum number of files to process (overrides JSON)"
    echo "  --skip-files <n>            Number of files to skip at start (overrides JSON)"
    echo "  -bt               Run backtrack step"
    echo "  -at               Run analzye_tps"
    echo "  -ab               Run add backgrounds step"
    echo "  --clean           Run cluster without backgrounds"
    echo "  -mc               Run make clusters step"
    echo "  -mm               Run match clusters step"
    echo "  -ac               Run analyze step"
    echo "  -gi               Generate cluster images (16x128 numpy arrays for NN)"
    echo "  -ai               Run analyze images step"
    echo "  -gv               Generate volume images"
    echo "  -av               Run analyze volumes step"
    echo "  --all                  Run all steps (default if no flags provided)"
    echo "  -f|--override    Force reprocessing even if output already exists (useful for debugging)"
    echo "  -d|--debug       Enable debug mode."
    echo "  -v|--verbose     Enable verbose mode."
    echo "  -h|--help        Print this help message."
    exit 0;
}

# Parse command line options
sample="" # cc_valid, es_valid, cc_bkg_myproc, es_bkg_myproc, cc_myproc, bkg_myprod, es_myproc
run_backtrack=false
run_add_backgrounds=false
run_make_clusters=false
run_match_clusters=false
run_analyze=false
run_generate_images=false
# run_analyze_images=false
run_generate_volumes=false
run_analyze_volumes=false
noCompile=false
cleanCompile=false
clean_clusters=false
bg_suffix="_bg"
run_analyze_tps=false
override=false
all_steps=false
debug=false
verbose=false
max_files=""
skip_files=""

while [[ $# -gt 0 ]]; do
        case $1 in
                -s|--sample) sample="$2"; shift 2 ;;
                -j|--json-settings) settingsFile="$2"; shift 2 ;;
                --home-dir) export HOME_DIR="$2"; echo "Home directory set from CLI to: $HOME_DIR"; source ${HOME_DIR}/scripts/init.sh; shift 2 ;;
                --max-files) max_files="$2"; shift 2 ;;
                --skip-files) skip_files="$2"; shift 2 ;;
                --no-compile) noCompile=true; shift ;;
                --clean-compile) cleanCompile=true; shift ;;
                -h|--help) print_help ;;
                -bt) run_backtrack=true; shift ;;
                -at) run_analyze_tps=true; shift ;;
                -ab) run_add_backgrounds=true; shift ;;
                --clean) clean_clusters=true; bg_suffix=""; shift ;;
                -mc) run_make_clusters=true; shift ;;
                -mm) run_match_clusters=true; shift ;;
                -ac) run_analyze=true; shift ;;
                -gi) run_generate_images=true; shift ;;
                # -ai) run_analyze_images=true; shift ;;
                -gv) run_generate_volumes=true; shift ;;
                -av) run_analyze_volumes=true; shift ;;
                -f|--override) override=true; shift ;;
                -d|--debug)
                        if [[ $2 == "true" || $2 == "false" ]]; then
                                debug=$2
                                shift 2
                        else
                                debug=true
                                shift
                        fi
                ;;
                -v|--verbose)
                        if [[ $2 == "true" || $2 == "false" ]]; then
                                verbose=$2
                                shift 2
                        else
                                verbose=true
                                shift
                        fi
                ;;
                --all|-a) all_steps="true"; shift ;;
                *) echo "Invalid option: $1"; exit 1 ;;
        esac
done

# Ensure --all always sets all run flags, regardless of order, after parsing and before any output
if [ "$all_steps" = "true" ]; then
        run_backtrack=true
        # run_analyze_tps=true
        run_add_backgrounds=true
        run_make_clusters=true
        run_match_clusters=true
        run_analyze=true
        run_generate_images=true
        # run_analyze_images=true
        run_generate_volumes=true
        run_analyze_volumes=true
fi

echo "**************************"
echo "Selected options:"
echo -e "Run backtrack:\t\t$run_backtrack"
echo -e "Run analyze_tps:\t$run_analyze_tps"
echo -e "Run add backgrounds:\t$run_add_backgrounds"
echo -e "Run make clusters:\t$run_make_clusters"
echo -e "Run match clusters:\t$run_match_clusters"
echo -e "Run analyze:\t\t$run_analyze"
echo -e "Run generate images:\t$run_generate_images"
# echo -e "Run analyze images:\t$run_analyze_images"
echo -e "Run generate volumes:\t$run_generate_volumes"
echo -e "Run analyze volumes:\t$run_analyze_volumes"
echo -e "Clean clusters (no backgrounds):\t$clean_clusters"
echo -e "No compile:\t\t$noCompile"
echo -e "Clean compile:\t\t$cleanCompile"
echo -e "Override:\t\t$override"
echo "**************************"
echo ""
echo "**************************"
echo "Sample selected: $sample."
if [ ! -z "$settingsFile" ]; then
        echo "Global json settings file provided: $settingsFile"
else
        echo "The list of jsons to be used is:"
        echo " Backtrack: $JSON_DIR/backtrack/${sample}.json"
        echo " Analyze TPS: $JSON_DIR/analyze_tps/${sample}.json"
        echo " Add backgrounds: $JSON_DIR/add_backgrounds/${sample}.json"
        echo " Make clusters: $JSON_DIR/make_clusters/${sample}${bg_suffix}.json"
        echo " Analyze: $JSON_DIR/analyze_clusters/${sample}${bg_suffix}.json"
        echo " Check it makes sense, waiting 5 seconds..."
fi
echo "**************************"
sleep 5

################################################
# If global json file was given, use that for all steps
if [ ! -z "$settingsFile" ]; then
        # check it exists
        find_settings_command="$SCRIPTS_DIR/findSettings.sh -j $settingsFile"
        echo " Looking for json settings using command: $find_settings_command"
        settingsFile=$($find_settings_command | tail -n 1)
        echo -e "Global settings file found, full path is: $settingsFile \n"
        echo "Using this for all steps, overriding individual jsons."   
        # JSON_DIR/backtrack/${sample}.json="$settingsFile"
        # JSON_DIR/analyze_tps/${sample}.json="$settingsFile"
        # JSON_DIR/add_backgrounds/${sample}.json="$settingsFile"
        # JSON_DIR/make_clusters/${sample}.json="$settingsFile"
        # JSON_DIR/analyze_clusters/${sample}.json="$settingsFile"
fi

################################################
# Compile the code once only
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command || exit

################################################

cd $HOME_DIR

common_options="--no-compile -f $override -v $verbose -d $debug"
if [ ! -z "$max_files" ]; then
        common_options="$common_options --max-files $max_files"
fi
if [ ! -z "$skip_files" ]; then
        common_options="$common_options --skip-files $skip_files"
fi

if [ ! -z "$settingsFile" ]; then
        backtrack_json="$settingsFile"
else
        backtrack_json="$JSON_DIR/backtrack/${sample}.json"
fi
backtrack_command="${HOME_DIR}/scripts/backtrack.sh -j $backtrack_json $common_options"
if [ "$run_backtrack" = true ]; then
        echo "Running backtrack step with command:"
        echo $backtrack_command
        $backtrack_command
        if [ $? -ne 0 ]; then
                echo "Error: Backtrack step failed."
                exit 1
        fi
        echo ""
fi

####################

if [ ! -z "$settingsFile" ]; then
        analyze_tps_json="$settingsFile"
else
        analyze_tps_json="$JSON_DIR/analyze_tps/${sample}.json"
fi
analyze_tps_command="${HOME_DIR}/scripts/analyze_tps.sh -j $analyze_tps_json $common_options"
if [ "$run_analyze_tps" = true ]; then
        echo "Running analyze_tps step with command:"
        echo $analyze_tps_command
        $analyze_tps_command
        if [ $? -ne 0 ]; then
                echo "Error: Analyze TPS step failed."
                exit 1
        fi
        echo ""
fi

####################

if [ ! -z "$settingsFile" ]; then
        add_backgrounds_json="$settingsFile"
else
        add_backgrounds_json="$JSON_DIR/add_backgrounds/${sample}.json"
fi
add_backgrounds_command="${HOME_DIR}/scripts/add_backgrounds.sh -j $add_backgrounds_json $common_options"
if [ "$run_add_backgrounds" = true ] && [ "$clean_clusters" = false ]; then
        echo "Running add backgrounds step with command:"
        echo $add_backgrounds_command
        $add_backgrounds_command
        if [ $? -ne 0 ]; then
                echo "Error: Add backgrounds step failed."
                exit 1
        fi
        echo ""
fi

####################

if [ ! -z "$settingsFile" ]; then
        make_clusters_json="$settingsFile"
else
        make_clusters_json="$JSON_DIR/make_clusters/${sample}${bg_suffix}.json"
fi
make_clusters_command="${HOME_DIR}/scripts/make_clusters.sh -j $make_clusters_json $common_options"
if [ "$run_make_clusters" = true ]; then
        echo "Running make clusters step with command:"
        echo $make_clusters_command
        $make_clusters_command
        if [ $? -ne 0 ]; then
                echo "Error: Make clusters step failed."
                exit 1
        fi
        echo ""
fi

####################

# Match clusters step
if [ ! -z "$settingsFile" ]; then
        match_clusters_json="$settingsFile"
else
        match_clusters_json="$JSON_DIR/match_clusters/${sample}${bg_suffix}.json"
fi
match_clusters_command="./scripts/match_clusters.sh -j $match_clusters_json $common_options"
if [ "$run_match_clusters" = true ]; then
        echo "Running match clusters step with command:"
        echo $match_clusters_command
        $match_clusters_command
        if [ $? -ne 0 ]; then
                echo "Error: Match clusters step failed."
                exit 1
        fi
        echo ""
fi

####################

if [ ! -z "$settingsFile" ]; then
        analyze_clusters_json="$settingsFile"
else
        analyze_clusters_json="$JSON_DIR/analyze_clusters/${sample}${bg_suffix}.json"
fi
analyze_command="${HOME_DIR}/scripts/analyze_clusters.sh -j $analyze_clusters_json $common_options"
if [ "$run_analyze" = true ]; then
        echo "Running analyze step with command: $analyze_command"
        $analyze_command
        if [ $? -ne 0 ]; then
                echo "Error: Analyze step failed."
                exit 1
        fi
        echo ""
fi

####################

# Generate cluster images (16x128 numpy arrays)
if [ "$run_generate_images" = true ]; then
        # Use the same JSON file as other steps
        if [ ! -z "$settingsFile" ]; then
                images_json="$settingsFile"
        else
                images_json="$JSON_DIR/${sample}${bg_suffix}.json"
        fi
        
        if [ ! -f "$images_json" ]; then
                echo "Warning: JSON file not found: ${images_json}"
                echo "Skipping image generation step."
        else
                generate_images_command="$SCRIPTS_DIR/generate_cluster_images.sh -j $images_json"
                if [ "$verbose" = true ]; then
                        generate_images_command+=" -v"
                fi
                
                echo "Running generate cluster images step with command:"
                echo $generate_images_command
                $generate_images_command
                if [ $? -ne 0 ]; then
                        echo "Error: Generate images step failed."
                        exit 1
                fi
                echo ""
        fi
fi

####################

# Analyze images
# if [ "$run_analyze_images" = true ]; then
#         if [ ! -z "$settingsFile" ]; then
#                 analyze_images_json="$settingsFile"
#         else
#                 analyze_images_json="$JSON_DIR/${sample}${bg_suffix}.json"
#         fi
        
#         if [ ! -f "$analyze_images_json" ]; then
#                 echo "Warning: JSON file not found: ${analyze_images_json}"
#                 echo "Skipping analyze images step."
#         else
#                 # Look for a Python analyze_images script
#                 if [ -f "$HOME_DIR/python/ana/analyze_images.py" ]; then
#                         analyze_images_command="python3 $HOME_DIR/python/ana/analyze_images.py --json $analyze_images_json"
#                         if [ "$verbose" = true ]; then
#                                 analyze_images_command+=" --verbose"
#                         fi
                        
#                         echo "Running analyze images step with command:"
#                         echo $analyze_images_command
#                         $analyze_images_command
#                         if [ $? -ne 0 ]; then
#                                 echo "Error: Analyze images step failed."
#                                 exit 1
#                         fi
#                         echo ""
#                 else
#                         echo "Warning: analyze_images.py script not found in python/ana/"
#                         echo "Skipping analyze images step."
#                 fi
#         fi
# fi

####################

# Generate volume images
if [ "$run_generate_volumes" = true ]; then
        if [ ! -z "$settingsFile" ]; then
                volumes_json="$settingsFile"
        else
                volumes_json="$JSON_DIR/${sample}${bg_suffix}.json"
        fi
        
        if [ ! -f "$volumes_json" ]; then
                echo "Warning: JSON file not found: ${volumes_json}"
                echo "Skipping generate volumes step."
        else
                generate_volumes_command="$SCRIPTS_DIR/create_volumes.sh -j $volumes_json"
                if [ "$verbose" = true ]; then
                        generate_volumes_command+=" -v"
                fi
                
                echo "Running generate volumes step with command:"
                echo $generate_volumes_command
                $generate_volumes_command
                if [ $? -ne 0 ]; then
                        echo "Error: Generate volumes step failed."
                        exit 1
                fi
                echo ""
        fi
fi

####################

# Analyze volumes
if [ "$run_analyze_volumes" = true ]; then
        if [ ! -z "$settingsFile" ]; then
                analyze_volumes_json="$settingsFile"
        else
                analyze_volumes_json="$JSON_DIR/${sample}${bg_suffix}.json"
        fi
        
        if [ ! -f "$analyze_volumes_json" ]; then
                echo "Warning: JSON file not found: ${analyze_volumes_json}"
                echo "Skipping analyze volumes step."
        else
                # Look for a Python analyze_volumes script
                if [ -f "$HOME_DIR/python/ana/analyze_volumes.py" ]; then
                        analyze_volumes_command="python3 $HOME_DIR/python/ana/analyze_volumes.py --json $analyze_volumes_json"
                        if [ "$verbose" = true ]; then
                                analyze_volumes_command+=" --verbose"
                        fi
                        
                        echo "Running analyze volumes step with command:"
                        echo $analyze_volumes_command
                        $analyze_volumes_command
                        if [ $? -ne 0 ]; then
                                echo "Error: Analyze volumes step failed."
                                exit 1
                        fi
                        echo ""
                else
                        echo "Warning: analyze_volumes.py script not found in python/ana/"
                        echo "Skipping analyze volumes step."
                fi
        fi
fi

################################################

echo "All requested steps completed, hopefully successfully."