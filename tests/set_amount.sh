#!/bin/bash
# Get token pair price from swap contract markets table
# Usage: bash get.price.sh [token_pair]
# Example: bash get.price.sh flon.usdt

TRADING_PAIR=${1:-flon.usdt}
BUYLOWSELLHI_CONTRACT=${2:-"buylowsellhi"}
UPDATER=${3:-"botuser11111"}
NODE_URL="http://t.flonscan.io"

# Query markets table for the token pair
# sym_pair_id=$(fucli -u $NODE_URL convert encode_name "$TRADING_PAIR")

fucli -u $NODE_URL get table $BUYLOWSELLHI_CONTRACT $BUYLOWSELLHI_CONTRACT trademarkets -l 1 -L $TRADING_PAIR -U $TRADING_PAIR

min_trade_amount="1.00000000 FLON"
max_trade_amount="300.00000000 FLON"
fucli -u $NODE_URL push action $BUYLOWSELLHI_CONTRACT settradeamt '["'$TRADING_PAIR'", "'"$min_trade_amount"'", "'"$max_trade_amount"'"]' -p $UPDATER@active
