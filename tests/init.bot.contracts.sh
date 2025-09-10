#!/bin/bash
set -e
# Configuration
# CURR_DIR=.
CURR_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$CURR_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

bot_mm_contract="botmm1111111"
buylowsellhi_contract="buylowsellhi"
tokenx_mm_contract="tokenxmm1111"
bot_admin="flonian"
node_url="https://t1.flonscan.io"
tp_code="flon.usdt"
bot_users=("botuser11111" "botuser11112" "botuser11113")
trade_key="FU8UEDT3816bHVG3wZyezaynxEowvbQx4aD7E6rZXte22NkQGUsu"

export node_url
 # each contract name corresponds to its account
contracts=($bot_mm_contract $buylowsellhi_contract $tokenx_mm_contract)
contract_names=(bot.mm buylowsellhi tokenx.mm)

# Create account if it does not exist
create_account() {
    local account_name="$1"
    local creator="$2"
    local fund_amount="$3"
    local new_acct_key="$4"
    # Check if account exists
    if ! fucli -u "$node_url" get account "$account_name" &>/dev/null; then
        echo "Creating account: $account_name"
        fucli -u "$node_url" system newaccount "$creator" "$account_name" "$new_acct_key" --fund-account "$fund_amount" -p "$creator@active"
    else
        echo "Account $account_name already exists, skipping creation."
    fi
}

deploy_contract() {
	local contract_account="$1"
	local contract_name="$2"
	local wasm_file="$BUILD_DIR/contracts/$contract_name/$contract_name.wasm"
	local abi_file="$BUILD_DIR/contracts/$contract_name/$contract_name.abi"
	echo "Deploying contract '$contract_name' code to account $contract_account..."
	fucli -u "$node_url" set code "$contract_account" "$wasm_file" -p "$contract_account@active"
	echo "Deploying contract '$contract_name' ABI to $contract_account..."
	fucli -u "$node_url" set abi "$contract_account" "$abi_file" -p "$contract_account@active"
	echo "Success to deploy Contract '$contract_name' to account $contract_account."
}

#1. Create accounts.
#1.1 Create contract accounts
echo "Creating contract accounts ..."
for a in "${contracts[@]}"; do
    create_account "$a" "$bot_admin" "10.0 FLON" "$bot_admin@active"
    # set code auth to contract account itself
    fucli -u "$node_url" set account permission "$a" active --add-code -p "$a@active"
done
#1.2 Create bot user accounts
echo "Creating bot user accounts ..."
for a in "${bot_users[@]}"; do
    create_account "$a" "$bot_admin" "1.0 FLON" "$tokenx_mm_contract@active"
done

#2. Deploy contracts
echo "Deploying contracts ..."
for i in ${!contracts[@]}; do
	deploy_contract "${contracts[$i]}" "${contract_names[$i]}"
done

#3. init bot.mm contract
echo "Initializing bot.mm contract ..."
#3.1 set bot.mm admin
echo "Setting admin for bot.mm contract to $bot_admin ..."
fucli -u "$node_url" push action "$bot_mm_contract" setadmin '{"admin":"'$bot_admin'"}' -p "$bot_mm_contract@active"

#3.2 set bot group
echo "Setting bot group for bot.mm contract ..."
group_name="$tp_code"
group_desc="Default bot group for $tp_code"
bots_json=$(printf '"%s",' "${bot_users[@]}" | sed 's/,$//')
setgroup_data='{"group_name":"'$group_name'","desc":"'$group_desc'","bots":['$bots_json']}'
fucli -u "$node_url" push action "$bot_mm_contract" setgroup "$setgroup_data" -p "$bot_mm_contract@active"


#4. init buylowsellhi contract
echo "Initializing buylowsellhi contract ..."
#4.1 set buylowsellhi admin
echo "Setting admin for buylowsellhi contract to $bot_admin ..."
fucli -u "$node_url" push action "$buylowsellhi_contract" setadmin '{"admin":"'$bot_admin'"}' -p "$buylowsellhi_contract@active"

#4.2 set trade market
echo "Setting trade market ${tp_code} for buylowsellhi contract ..."
trade_market_name="$tp_code"
paused=false
target_price=0.01
min_trade_amount="1.00000000 FLON"
max_trade_amount="100.00000000 FLON"
memo="$tp_code market"
updaters_json=$(printf '"%s",' "${bot_users[@]}" | sed 's/,$//')
settrademkt_data='{
    "trade_market_name":"'$trade_market_name'",
    "paused":'$paused',
    "target_price":'$target_price',
    "min_trade_amount":"'"$min_trade_amount"'",
    "max_trade_amount":"'"$max_trade_amount"'",
    "memo":"'$memo'",
    "updaters":['$updaters_json']
}'

fucli -u "$node_url" push action "$buylowsellhi_contract" settrademkt "$settrademkt_data" -p "$buylowsellhi_contract@active"

#5. init tokenx.mm contract
echo "Initializing tokenx.mm contract ..."
#5.1 set tokenx.mm admin
echo "Setting admin for tokenx.mm contract to $bot_admin ..."
fucli -u "$node_url" push action "$tokenx_mm_contract" setadmin '{"admin":"'$bot_admin'"}' -p "$tokenx_mm_contract@active"

#5.2 (optional) configure bot manager contract for tokenx.mm if needed
echo "Configuring bot manager contract to $bot_mm_contract for tokenx.mm ..."
fucli -u "$node_url" push action "$tokenx_mm_contract" cfgbotmgr '{"bot_mgr_contract":"'$bot_mm_contract'"}' -p "$tokenx_mm_contract@active"



#6. add executing trade permission to bot_admin and link to trade of tokenx.mm contract
echo "Adding executing trade permission for $bot_admin ..."

#6.1 create new permission "trade" to bot_admin if not exist
echo "Checking if 'trade' permission exists for $bot_admin ..."
if ! fucli -u "$node_url" get account "$bot_admin" | grep -q '"perm_name": "trade"'; then
    echo "Creating 'trade' permission for $bot_admin ..."
    authority='{"threshold":1,"keys":[{"key":"'$trade_key'","weight":1}],"accounts":[],"waits":[]}'
    fucli -u "$node_url" set account permission "$bot_admin" trade "$authority" active -p "$bot_admin@active"
else
    echo "'trade' permission already exists for $bot_admin, skipping creation."
fi

# link trade action to "trade" permission of bot_admin, skip if already set
echo "Checking if trade action link exists for $bot_admin ..."
if ! fucli -u "$node_url" get account "$bot_admin" | grep -q '"code": "'$tokenx_mm_contract'", "type": "trade", "required_permission": "trade"'; then
    echo "Linking trade action to trade permission ..."
    fucli -u "$node_url" set action permission "$bot_admin" "$tokenx_mm_contract" trade trade -p "$bot_admin@active"
else
    echo "trade action already linked to trade permission, skipping."
fi