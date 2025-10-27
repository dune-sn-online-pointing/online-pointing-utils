#!/bin/bash

export SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${SCRIPTS_DIR}/init.sh


print_help(){
    echo "Usage: $0 -s <sample> [-bt] [-ab] [-mc] [-ac] [-gi] [--no-compile] [--clean-compile]"; 
    echo "Options:";
    echo "  --no-compile                Do not recompile the code"
    echo "  --clean-compile             Clean and recompile the code"
    echo "  -j|--json-settings <json>   Global json file including all steps. Overrides individual jsons if parsed"
    echo "  -bt               Run backtrack step"
    echo "  -at               Run analzye_tps"
    echo "  -ab               Run add backgrounds step"
    echo "  --clean           Run cluster without backgrounds"
    echo "  -mc               Run make clusters step"
    echo "  -ac               Run analyze step"
    echo "  -gi               Generate cluster images (16x128 numpy arrays for NN)"
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
run_analyze=false
run_generate_images=false
noCompile=false
cleanCompile=false
clean_clusters=false
bg_suffix="_bg"
run_analyze_tps=false
override=false
all_steps=false
debug=false
verbose=false

while [[ $# -gt 0 ]]; do
        case $1 in
                -s|--sample) sample="$2"; shift 2 ;;
                -j|--json-settings) settingsFile="$2"; shift 2 ;;
                --no-compile) noCompile=true; shift ;;
                --clean-compile) cleanCompile=true; shift ;;
                -h|--help) print_help ;;
                -bt) run_backtrack=true; shift ;;
                -at) run_analyze_tps=true; shift ;;
                -ab) run_add_backgrounds=true; shift ;;
                --clean) clean_clusters=true; bg_suffix=""; shift ;;
                -mc) run_make_clusters=true; shift ;;
                -ac) run_analyze=true; shift ;;
                -gi) run_generate_images=true; shift ;;
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
        run_analyze=true
        run_generate_images=true
fi

echo "**************************"
echo "Selected options:"
echo -e "Run backtrack:\t\t$run_backtrack"
echo -e "Run analyze_tps:\t$run_analyze_tps"
echo -e "Run add backgrounds:\t$run_add_backgrounds"
echo -e "Run make clusters:\t$run_make_clusters"
echo -e "Run analyze:\t\t$run_analyze"
echo -e "Run generate images:\t$run_generate_images"
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

if [ ! -z "$settingsFile" ]; then
        backtrack_json="$settingsFile"
else
        backtrack_json="$JSON_DIR/backtrack/${sample}.json"
fi
backtrack_command="./scripts/backtrack.sh -j $backtrack_json $common_options"
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
analyze_tps_command="./scripts/analyze_tps.sh -j $analyze_tps_json $common_options"
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
add_backgrounds_command="./scripts/add_backgrounds.sh -j $add_backgrounds_json $common_options"
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
make_clusters_command="./scripts/make_clusters.sh -j $make_clusters_json $common_options"
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

if [ ! -z "$settingsFile" ]; then
        analyze_clusters_json="$settingsFile"
else
        analyze_clusters_json="$JSON_DIR/analyze_clusters/${sample}${bg_suffix}.json"
fi
analyze_command="./scripts/analyze_clusters.sh -j $analyze_clusters_json $common_options"
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
        # Determine cluster folder from make_clusters json
        if [ ! -z "$settingsFile" ]; then
                clusters_json="$settingsFile"
        else
                clusters_json="$JSON_DIR/make_clusters/${sample}${bg_suffix}.json"
        fi
        
        # Extract output directory from json
        cluster_folder=$(python3 -c "import json; f=open('${clusters_json}'); d=json.load(f); print(d.get('output_directory', ''))")
        
        if [ -z "$cluster_folder" ] || [ ! -d "$cluster_folder" ]; then
                echo "Warning: Could not determine cluster folder from ${clusters_json}"
                echo "Skipping image generation step."
        else
                generate_images_command="$SCRIPTS_DIR/generate_cluster_images.sh $cluster_folder"
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

################################################

echo "All requested steps completed, hopefully successfully."