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
node_url="http://hk-t3.vm.nestar.vip:18888"

export node_url
 # each contract name corresponds to its account
contracts=($bot_mm_contract $buylowsellhi_contract $tokenx_mm_contract)
contract_names=(bot.mm buylowsellhi tokenx.mm)

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

#Deploy contracts
echo "Deploying contracts ..."
for i in ${!contracts[@]}; do
	deploy_contract "${contracts[$i]}" "${contract_names[$i]}"
done
