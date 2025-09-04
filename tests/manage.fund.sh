#!/bin/bash

# Configuration
bot_mm_contract="botmm1111111"
buylowsellhi_contract="buylowsellhi"
tokenx_mm_contract="tokenxmm1111"
bot_admin="flonian"
node_url="https://t1.flonscan.io"
tp_code="flon.usdt"
bot_users=("botuser11111" "botuser11112" "botuser11113")
swap_contract="flon.swap"

export node_url swap_contract

get_account_assets() {
    local account="$1"
    local title="$2"
    echo "--- $title Asset Info: $account ---"
    echo "FLON Balance:"
    fucli -u "$node_url" get currency balance flon.token "$account" "FLON"
    echo "$tp_code Balance:"
    fucli -u "$node_url" get currency balance flon.mtoken "$account" "USDT"
    echo "-------------------------"
}

get_swap_market_info() {
    local trading_pair=${1}
    resp=$(fucli -u $node_url get table $swap_contract $swap_contract markets -l 1 -L $trading_pair -U $trading_pair)
    left_pool=$(echo "$resp" | jq -r '.rows[0].left_pool_quant.quantity')
    right_pool=$(echo "$resp" | jq -r '.rows[0].right_pool_quant.quantity')
    left_amount=$(echo "$left_pool" | awk '{print $1}')
    right_amount=$(echo "$right_pool" | awk '{print $1}')
    if [[ -n "$left_amount" && -n "$right_amount" ]]; then
        price=$(awk "BEGIN {if ($left_amount > 0) print $right_amount / $left_amount; else print 0}")
    fi
    left_symbol=$(echo "$left_pool" | awk '{print $2}')
    right_symbol=$(echo "$right_pool" | awk '{print $2}')

    echo "--- swap market info ---"
    echo "Token Pair: $trading_pair"
    echo "Price: 1 $left_symbol = $price $right_symbol"
    echo "Left Pool: $left_pool"
    echo "Right Pool: $right_pool"
    echo "-------------------------"
}

#1. Get accounts fund info
#1.1 get tokenx_mm_contract fund info
get_account_assets "$tokenx_mm_contract" "tokenx_mm_contract"
#1.2 get bot users fund info
for user in "${bot_users[@]}"; do
    get_account_assets "$user" "Bot User"
done

#1.3 get bot_admin fund info
get_account_assets "$bot_admin" "bot_admin"

#1.3 get tokenx_mm_contract global info
fucli -u "$node_url" get table "$tokenx_mm_contract" "$tokenx_mm_contract" global

# 1.4 get price from swap contract
get_swap_market_info "$tp_code"

#2. deposit fund to tokenx_mm_contract
#2.1 deposit 10000 FLON
fucli -u "$node_url" transfer "$bot_admin" "$tokenx_mm_contract" "10000.0 FLON" -p "$bot_admin@active"

#2.2 deposit 100 USDT
fucli -u "$node_url" transfer "$bot_admin" "$tokenx_mm_contract" "100.0 USDT" --contract "flon.mtoken" -p "$bot_admin@active"