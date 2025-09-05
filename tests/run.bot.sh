#!/bin/bash
# run.bot.sh - Automatically loop to call tokenx.mm contract exectrade, and record trade details
# set -e
# Configurations
tokenx_mm_contract="tokenxmm1111"
node_url="http://hk-t3.vm.nestar.vip:18888"
interval_min=10      # Minimum interval in seconds
interval_max=60      # Maximum interval in seconds

log() {
    local message="$1"
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "[$timestamp] $message"
}

# mkdir -p "$logs_dir"
echo "-----------------------------------------------------------------"
log "Starting automated trading bot..."
while true; do

    # Execute trade
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    # random nonce
    memo=$RANDOM
    result=$(fucli -u "$node_url" push action "$tokenx_mm_contract" exectrade '{"memo":"'$memo'"}' -p "$tokenx_mm_contract@active")
    [ $? -ne 0 ] && log "ERROR: exectrade failed and wait for 3 seconds for another try" && sleep 3 && continue

    # Record log
    echo "-----------------------------"
    log "exectrade status: $status"
    log "$result"
    sleep_time=$(( RANDOM % (interval_max - interval_min + 1) + interval_min ))
    log "wait for: ${sleep_time}s"
    sleep "$sleep_time"
done
