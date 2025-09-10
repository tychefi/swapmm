#!/bin/bash
# Get token pair price from swap contract markets table
# Usage: bash get.price.sh [token_pair]
# Example: bash get.price.sh flon.usdt

TRADING_PAIR=${1:-flon.usdt}
BUYLOWSELLHI_CONTRACT=${2:-"buylowsellhi"}
UPDATER=${3:-"botuser11111"}
NODE_URL="http://hk-t3.vm.nestar.vip:18888"

# Query markets table for the token pair
# sym_pair_id=$(fucli -u $NODE_URL convert encode_name "$TRADING_PAIR")

fucli -u $NODE_URL get table $BUYLOWSELLHI_CONTRACT $BUYLOWSELLHI_CONTRACT trademarkets -l 1 -L $TRADING_PAIR -U $TRADING_PAIR

paused=true
fucli -u $NODE_URL push action $BUYLOWSELLHI_CONTRACT pause '["'$UPDATER'", "'$TRADING_PAIR'", '$paused']' -p $UPDATER@active

# check rows size > 0
rows_count=$(echo "$resp" | jq '.rows | length')
if [[ "$rows_count" -eq 0 ]]; then
	echo "Error: No rows found for $TRADING_PAIR"
	exit 1
fi

# Parse price
left_pool=$(echo "$resp" | jq -r '.rows[0].left_pool_quant.quantity')
right_pool=$(echo "$resp" | jq -r '.rows[0].right_pool_quant.quantity')

left_amount=$(echo "$left_pool" | awk '{print $1}')
right_amount=$(echo "$right_pool" | awk '{print $1}')

if [[ -z "$left_amount" || -z "$right_amount" ]]; then
	echo "Error: Pool info not found for $PAIR"
	exit 1
fi

price=$(awk "BEGIN {if ($left_amount > 0) print $right_amount / $left_amount; else print 0}")

left_symbol=$(echo "$left_pool" | awk '{print $2}')
right_symbol=$(echo "$right_pool" | awk '{print $2}')

# Output result
cat <<EOF
Token Pair: $PAIR
Price: 1 $left_symbol = $price $right_symbol
Left Pool: $left_pool
Right Pool: $right_pool
EOF
