#!/bin/bash
# Completeness check for the matchfix rebuild of the ES production ED pool.
# Compares every product folder against the tpstream basenames that drive the chain
# and prints the missing basenames (which map 1:1 onto --skip-files/--max-files slices).
#
# Usage: check_prodes_matchfix_complete.sh [--list-missing]
set -u
SRC=/eos/project-e/ep-nu/evilla/sn-online-pointing/prod_es
B=/eos/project-e/ep-nu/evilla/sn-online-pointing/prod_es_matchfix
COND=tick3_ch2_min2_tot3_e2p0

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

ls "$SRC"/tpstreams | grep '_tpstream\.root$' | sed 's/_tpstream\.root$//' | sort > "$TMP/base"
n_base=$(wc -l < "$TMP/base")
echo "tpstream basenames: $n_base"

check () {  # $1 label, $2 dir, $3 sed expression stripping the suffix
    if [[ ! -d "$2" ]]; then echo "  $1: MISSING DIR $2"; return; fi
    ls "$2" | sed "$3" | sort -u > "$TMP/have"
    n=$(wc -l < "$TMP/have")
    comm -23 "$TMP/base" "$TMP/have" > "$TMP/miss"
    m=$(wc -l < "$TMP/miss")
    echo "  $1: $n present, $m missing"
    if [[ "${1:-}" != "" && "$m" -gt 0 && "${LIST:-0}" == "1" ]]; then
        echo "    first missing: $(head -3 "$TMP/miss" | tr '\n' ' ')"
        while read -r b; do grep -n "^$b$" "$TMP/base" | cut -d: -f1; done < "$TMP/miss" \
            | sort -n | awk '{print $1-1}' | tr '\n' ' ' | fold -w 200 | sed 's/^/    0-based indices: /'
        echo
    fi
}

[[ "${1:-}" == "--list-missing" ]] && LIST=1 || LIST=0

check "tps_bg                 " "$B/tps_bg"                                  's/_bg_tps\.root$//'
check "clusters               " "$B/es_production_clusters_$COND"            "s/_bg_clusters\.root$//;s/_bg\.root$//;s/_clusters\.root$//"
check "matched (matchfix)     " "$B/es_production_matched_clusters_${COND}_matchfix"   's/_bg_matched\.root$//'
check "matched (legacy rule)  " "$B/es_production_matched_clusters_${COND}_legacyrule" 's/_bg_matched\.root$//'
for p in U V X; do
    check "images $p              " "$B/es_production_cluster_images_${COND}_matchfix/$p" "s/_bg_matched_plane${p}\.npz\$//"
done
echo
echo "inodes free on /eos/project-e/ep-nu: $(getfattr --absolute-names --only-values -n eos.quota /eos/project-e/ep-nu 2>/dev/null | awk '/^project-e/ {print $5}')"
