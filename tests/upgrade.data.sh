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
bot_fund="flonian"
fee_payer="flonian"
node_url="https://t.flonscan.io"
bot_users=("botuser11111" "botuser11112" "botuser11113")
trade_key="FU8UEDT3816bHVG3wZyezaynxEowvbQx4aD7E6rZXte22NkQGUsu"
trade_pairs=("flon.usdt")
market_fund_accounts=("botfundfusdt")

export node_url

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

#1. upgrade flon.usdt of buylowsellhi
# 1.1 deploy buylowsellhi contract
echo "Deploying buylowsellhi contract ..."
fucli -u "$node_url" set contract "$buylowsellhi_contract" "$BUILD_DIR/contracts/buylowsellhi" -p "$buylowsellhi_contract@active"
# 1.2 upgrade flon.usdt of buylowsellhi
echo "Upgrading flon.usdt of buylowsellhi ..."
fucli -u "$node_url" push action "$buylowsellhi_contract" upgtrademkt '["flon.usdt"]' -p "$bot_admin@active"

fucli -u "$node_url" get table "$buylowsellhi_contract" "$buylowsellhi_contract" trademarkets -L flon.usdt -l 1

#2. create bot fund account if not exist
create_account "botfundfusdt" "$bot_admin" "100.0 FLON" "$bot_admin@active"

#3. set bot market of flon.ust in tokenx.mm
# 3.1 deploy tokenx.mm contract
echo "Deploying tokenx.mm contract ..."
fucli -u "$node_url" set contract "$tokenx_mm_contract" "$BUILD_DIR/contracts/tokenx.mm" -p "$tokenx_mm_contract@active"
# 3.2 set bot market of flon.usdt in tokenx.mm
echo "Setting bot market of flon.usdt in tokenx.mm ..."
fucli -u "$node_url" push action "$tokenx_mm_contract" setmarket '["flon.usdt", "botfundfusdt", "flon.usdt"]' -p "$tokenx_mm_contract@active"
# 3.3 verify
fucli -u "$node_url" get table "$tokenx_mm_contract" "$tokenx_mm_contract" botmarkets -L flon.usdt -l 1

#4. add fund to bot market fund account
#4.1 transfer FLON to bot market fund account
fucli -u "$node_url" transfer "$bot_fund" $tokenx_mm_contract "1000.0 FLON" "addfund:flon.usdt" -p "$bot_fund@active"
#4.1 transfer USDT to bot market fund account
fucli -u "$node_url" transfer "$bot_fund" $tokenx_mm_contract "1000.0 USDT" "addfund:flon.usdt" -c flon.mtoken -p "$bot_fund@active"
#4.2 refresh fund info in tokenx.mm
fucli -u "$node_url" push action "$tokenx_mm_contract" refreshfund '["flon.usdt"]' -p "$tokenx_mm_contract@active"
#4.3 verify fund info
fucli -u "$node_url" get table "$tokenx_mm_contract" "$tokenx_mm_contract" botmarkets -L flon.usdt -l 1

# 5. setup permission
#5.1 create new permission "trade" to bot_admin if not exist
create_trade_permission() {
    local user="$1"
    local admin="$2"
    echo "Checking if 'trade' permission exists for $user ..."
    if ! fucli -u "$node_url" get account "$user" | grep -q '"perm_name": "trade"'; then
        echo "Creating 'trade' permission for $user ..."
        authority='{"threshold":1,"keys":[],"accounts":[{"permission":{"actor":"'"$admin"'","permission":"trade"},"weight":1}],"waits":[]}'
        fucli -u "$node_url" set account permission "$user" trade "$authority" active -p "$user@active"
    else
        echo "'trade' permission already exists for $user, skipping creation."
    fi
}

for u in "${bot_users[@]}"; do
    create_trade_permission "$u" "$bot_admin"
done

#5.2 link trade action to "trade" permission of bot_admin, skip if already set
link_trade_action_permission() {
    local user="$1"
    local contract="$2"
    echo "Checking if trade action link exists for $user ..."
    if ! fucli -u "$node_url" get account "$user" | grep -q "${contract}::trade"; then
        echo "Linking trade action to trade permission ..."
        fucli -u "$node_url" set action permission "$user" "$contract" trade trade -p "$user@active"
    else
        echo "trade action already linked to trade permission, skipping."
    fi
}

for u in "${bot_users[@]}"; do
    link_trade_action_permission "$u" "$tokenx_mm_contract"
done

#6. execute trade by bot user
fucli -u "$node_url" push action "$tokenx_mm_contract" trade '["botuser11111", "flon.usdt", "123"]' -p "$bot_fund@trade" -p "botuser11111@trade"

#7. verify trade public key of signature by trade transaction
tid=37203292e71e58fa7b9ae346b99bcdf3996088a52df59c3c92af881d1342c8cb
trx=$(tcli get transaction $tid | jq '.trx.trx')
tcli validate signatures "${trx}"
