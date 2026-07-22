#!/bin/bash
# run.bot.sh - Automatically loop to call tokenx.mm contract trade, and record trade details
# set -e
# Configurations
tokenx_mm_contract="tokenxmm1111"
node_url="http://hk-t3.vm.nestar.vip:18888"
bot="botuser11111"
trade_pair="flon.usdt"
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
    result=$(fucli -u "$node_url" push action "$tokenx_mm_contract" trade '{"bot":"'$bot'","trade_pair_name":"'$trade_pair'","memo":"'$memo'"}' -p "$bot@trade")
    if [ $? -ne 0 ]; then
        retry_sleep=$(( RANDOM % (interval_max - interval_min + 1) + interval_min ))
        log "ERROR: trade failed and wait for ${retry_sleep} seconds for another try"
        sleep "$retry_sleep"
        continue
    fi

    # Record log
    echo "-----------------------------"
    log "trade status: $status"
    log "$result"
    sleep_time=$(( RANDOM % (interval_max - interval_min + 1) + interval_min ))
    log "wait for: ${sleep_time}s"
    sleep "$sleep_time"
done
