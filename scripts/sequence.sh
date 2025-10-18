#!/bin/bash

export SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${SCRIPTS_DIR}/init.sh


print_help(){
    echo "Usage: $0 -d <dataset> [-bt] [-ab] [-mc] [-ac] [--no-compile] [--clean-compile]"; 
    echo "Options:";
    echo "  --no-compile               Do not recompile the code"
    echo "  --clean-compile           Clean and recompile the code"
    echo "  -bt               Run backtrack step"
    echo "  -at               Run analzye_tps"
    echo "  -ab               Run add backgrounds step"
    echo "  --clean           Run cluster without backgrounds"
    echo "  -mc               Run make clusters step"
    echo "  -ac               Run analyze step"
    echo "  --all                  Run all steps (default if no flags provided)"
    echo "  -o|--override    Force reprocessing even if output already exists (useful for debugging)"
    echo "  -h|--help        Print this help message."
    exit 0;
}

# Parse command line options
dataset="" # cc_valid, es_valid, cc_bkg_myproc, es_bkg_myproc, cc_myproc, bkg_myprod, es_myproc
run_backtrack=false
run_add_backgrounds=false
run_make_clusters=false
run_analyze=false
noCompile=false
cleanCompile=false
clean_clusters=false
bg_suffix="_bg"
run_analyze_tps=false
override=false
all_steps=false



while [[ $# -gt 0 ]]; do
        case $1 in
                -d)
                        dataset="$2"
                        shift 2
                        ;;
                --no-compile)
                        noCompile=true
                        shift
                        ;;
                --clean-compile)
                        cleanCompile=true
                        shift
                        ;;
                -h|--help)
                        print_help
                        ;;
                -bt)
                        run_backtrack=true
                        shift
                        ;;
                -at)
                        run_analyze_tps=true
                        shift
                        ;;
                -ab)
                        run_add_backgrounds=true
                        shift
                        ;;
                --clean)
                        clean_clusters=true
                        bg_suffix=""
                        shift
                        ;;
                -mc)
                        run_make_clusters=true
                        shift
                        ;;
                -ac)
                        run_analyze=true
                        shift
                        ;;
                -o|--override)
                        override=true
                        shift
                        ;;
                --all|-a)
                        all_steps="true"
                        shift
                        ;;
                *)
                        echo "Invalid option: $1"
                        exit 1
                        ;;
        esac
done



# # Debug: print all_steps and run flags after parsing
# echo "[DEBUG] all_steps: $all_steps"
# echo "[DEBUG] Before --all block:"
# echo "  run_backtrack: $run_backtrack"
# echo "  run_analyze_tps: $run_analyze_tps"
# echo "  run_add_backgrounds: $run_add_backgrounds"
# echo "  run_make_clusters: $run_make_clusters"
# echo "  run_analyze: $run_analyze"


# Ensure --all always sets all run flags, regardless of order, after parsing and before any output
if [ "$all_steps" = "true" ]; then
        run_backtrack=true
        # run_analyze_tps=true
        run_add_backgrounds=true
        run_make_clusters=true
        run_analyze=true
fi

# echo "[DEBUG] After --all block:"
# echo "  run_backtrack: $run_backtrack"
# echo "  run_analyze_tps: $run_analyze_tps"
# echo "  run_add_backgrounds: $run_add_backgrounds"
# echo "  run_make_clusters: $run_make_clusters"
# echo "  run_analyze: $run_analyze"

echo "Selected options:"
echo " Run backtrack: $run_backtrack"
echo " Run analyze_tps: $run_analyze_tps"
echo " Run add backgrounds: $run_add_backgrounds"
echo " Run make clusters: $run_make_clusters"
echo " Run analyze: $run_analyze"
echo " Clean clusters (no backgrounds): $clean_clusters"
echo " No compile: $noCompile"
echo " Clean compile: $cleanCompile"
echo " Override: $override"

echo "Dataset selected: $dataset. Verify it makes sense, waiting 5 seconds..."
sleep 5

################################################
# Compile the code once only
compile_command="$SCRIPTS_DIR/compile.sh -p $HOME_DIR --no-compile $noCompile --clean-compile $cleanCompile"
echo "Compiling the code with the following command:"
echo $compile_command
. $compile_command
if [ $? -ne 0 ]; then
        echo "Error: Compilation failed."
        exit 1
fi

################################################

cd $HOME_DIR

backtrack_command="./scripts/backtrack.sh -j $JSON_DIR/backtrack/${dataset}.json --no-compile --override $override"
if [ "$run_backtrack" = true ]; then
        echo "Running backtrack step..."
        $backtrack_command
        if [ $? -ne 0 ]; then
                echo "Error: Backtrack step failed."
                exit 1
        fi
fi

analyze_tps_command="./scripts/analyze_tps.sh -j $JSON_DIR/analyze_tps/${dataset}.json --no-compile --override $override"
if [ "$run_analyze_tps" = true ]; then
        echo "Running analyze_tps step..."
        $analyze_tps_command
        if [ $? -ne 0 ]; then
                echo "Error: Analyze TPS step failed."
                exit 1
        fi
fi

add_backgrounds_command="./scripts/add_backgrounds.sh -j $JSON_DIR/add_backgrounds/${dataset}.json --no-compile --override $override"
if [ "$run_add_backgrounds" = true ]; then
        echo "Running add backgrounds step..."
        $add_backgrounds_command
        if [ $? -ne 0 ]; then
                echo "Error: Add backgrounds step failed."
                exit 1
        fi
fi


make_clusters_json="$JSON_DIR/make_clusters/${dataset}${bg_suffix}.json"
make_clusters_command="./scripts/make_clusters.sh -j $make_clusters_json --no-compile true --override $override"
if [ "$run_make_clusters" = true ]; then
        echo "Running make clusters step..."
        $make_clusters_command
        if [ $? -ne 0 ]; then
                echo "Error: Make clusters step failed."
                exit 1
        fi
fi

echo "Constructing analyze command..."


analyze_clusters_json="$JSON_DIR/analyze_clusters/${dataset}${bg_suffix}.json"
analyze_command="./scripts/analyze_clusters.sh -j $analyze_clusters_json --no-compile true --override $override"
if [ "$run_analyze" = true ]; then
        echo "Running analyze step with command: $analyze_command"
        $analyze_command
        if [ $? -ne 0 ]; then
                echo "Error: Analyze step failed."
                exit 1
        fi
fi

echo "All requested steps completed."