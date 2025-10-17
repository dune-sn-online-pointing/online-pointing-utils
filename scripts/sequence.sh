#!/bin/bash

export SCRIPTS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source ${SCRIPTS_DIR}/init.sh

dataset="" # cc_valid, es_valid, cc_bkg_myproc, es_bkg_myproc, cc_myproc, bkg_myprod, es_myproc
override=false

print_help(){
    echo "Usage: $0 <dataset> [-bt] [-ab] [-mc] [-ac] [--no-compile] [--clean-compile]"; 
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
run_backtrack=false
run_add_backgrounds=false
run_make_clusters=false
run_analyze=false
noCompile=false
cleanCompile=false
clean_clusters=false

run_analyze_tps=false

if [ $# -eq 0 ]; then
        print_help
fi

dataset=$1
shift

for arg in "$@"; do
        case $arg in
                --no-compile) noCompile=true ;;
                --clean-compile) cleanCompile=true ;;
                -h|--help) print_help ;;
                -bt) run_backtrack=true ;;
                -at) run_analyze_tps=true ;;
                -ab) run_add_backgrounds=true ;;
                --clean) clean_clusters=true ;;
                -mc) run_make_clusters=true ;;
                -ac) run_analyze=true ;;
                -o|--override) override=true ;;
                --all)
                        run_backtrack=true
                        run_analyze_tps=true
                        run_add_backgrounds=true
                        run_make_clusters=true
                        run_analyze=true
                        ;;
                *) echo "Invalid option: $arg"; exit 1 ;;
        esac
done

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

backtrack_command="$SCRIPTS_DIR/backtrack.sh -j $JSON_DIR/backtrack/${dataset}.json --no-compile --override $override"
if [ "$run_backtrack" = true ]; then
        echo "Running backtrack step..."
        . $backtrack_command
        if [ $? -ne 0 ]; then
                echo "Error: Backtrack step failed."
                exit 1
        fi
fi

analyze_tps_command="$SCRIPTS_DIR/analyze_tps.sh -j $JSON_DIR/analyze_tps/${dataset}.json --no-compile --override $override"
if [ "$run_analyze_tps" = true ]; then
        echo "Running analyze_tps step..."
        . $analyze_tps_command
        if [ $? -ne 0 ]; then
                echo "Error: Analyze TPS step failed."
                exit 1
        fi
fi

add_backgrounds_command="$SCRIPTS_DIR/add_backgrounds.sh -j $JSON_DIR/add_backgrounds/${dataset}.json --no-compile --override $override"
if [ "$run_add_backgrounds" = true ]; then
        echo "Running add backgrounds step..."
        . $add_backgrounds_command
        if [ $? -ne 0 ]; then
                echo "Error: Add backgrounds step failed."
                exit 1
        fi
fi


if [ "$clean_clusters" = true ]; then
        make_clusters_json="$JSON_DIR/make_clusters/${dataset}.json"
else
        make_clusters_json="$JSON_DIR/make_clusters/${dataset}_bg.json"
fi


make_clusters_command="$SCRIPTS_DIR/make_clusters.sh -j $make_clusters_json --no-compile true --override $override"
if [ "$run_make_clusters" = true ]; then
        echo "Running make clusters step..."
        . $make_clusters_command
        if [ $? -ne 0 ]; then
                echo "Error: Make clusters step failed."
                exit 1
        fi
fi

echo "Constructing analyze command..."


if [ "$clean_clusters" = true ]; then
        analyze_clusters_json="$JSON_DIR/analyze_clusters/${dataset}.json"
else
        analyze_clusters_json="$JSON_DIR/analyze_clusters/${dataset}_bg.json"
fi

analyze_command="$SCRIPTS_DIR/analyze_clusters.sh -j $analyze_clusters_json --no-compile true --override $override"
if [ "$run_analyze" = true ]; then
        echo "Running analyze step with command: $analyze_command"
        . $analyze_command
        if [ $? -ne 0 ]; then
                echo "Error: Analyze step failed."
                exit 1
        fi
fi

echo "All requested steps completed."