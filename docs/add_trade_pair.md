# Contract Initialization and Deployment Guide

## 1. Preparation

### Preparation Checklist

| Item                | Value                 | Description                                |
| ------------------- | --------------------- | ------------------------------------------ |
| bot_admin           | flonian               | Bot admin account, exists account          |
| fund_account        | flonian               | Fund account, must have assets(FLON, USDT) |
| fee_payer           | flonian               | Fee payer account, exists account          |
| trade_updater       | flonian               | trade updater, who can update target price |
| trade_pair          | eth.usdt              | new trade pair, name type                  |
| market_fund_account | ethusdtfund1          | market fund account, name type             |
| target_price        | 4500.0                | target price, double type                  |
| min_trade_amount    | 0.00010000 ETH        | min trade amount, asset type               |
| max_trade_amount    | 0.00100000 ETH        | max trade amount, asset type               |
| deposit_flon        | 0.01000000 ETH        | total flon to deposit, asset type          |
| deposit_usdt        | 45.000000 USDT        | total usdt to deposit, asset type          |
| url                 | https://m.flonscan.io | node RPC url                               |
| dex_contract        | flon.swap             | dex contract                               |
| trade_pair          | flon.usdt             | trade pair name                            |


### ENV

```bash
# Prepare environment variables
bot_admin="flonian"
fund_account="flonian"
trade_updater="flonian"
target_price=4500.0
min_trade_amount="0.00010000 ETH"
max_trade_amount="0.00100000 ETH"
deposit_flon="0.01000000 ETH"
deposit_usdt="45.000000 USDT"
buylowsellhi_contract="buylowsellhi"
bot_mm_contract="bot.mm"
tokenx_mm_contract="tokenx.mm"
trade_pair="eth.usdt"
bot_users=("ethusdtusr11" "ethusdtusr12" "ethusdtusr13")
market_fund_account="ethusdtfund1"
url="https://m.flonscan.io"
dex_contract="flon.swap"
left_contract="flon.mtoken"
right_contract="flon.mtoken"
```

## 2. Create market fund account
```bash
fucli -u $url system newaccount $bot_admin $market_fund_account "$bot_admin@active" "$tokenx_mm_contract@active" --fund-account "1.0 FLON" -p $bot_admin@active
```

### Create Bot Accounts
```bash
for bot in "${bot_users[@]}"; do
    fucli -u $url system newaccount $bot_admin "$bot" "$bot_admin@active" "$tokenx_mm_contract@active" --fund-account "1.0 FLON" -p $bot_admin@active
done
```

## 3. Configure bot group
```bash
bots_json=$(printf '"%s",' "${bot_users[@]}" | sed 's/,$//')
fucli -u $url push action $bot_mm_contract setgroup '{"group_name":"'"$trade_pair"'","desc":"Default bot group for '"$trade_pair"'","bots":['"$bots_json"']}' -p $bot_admin@active
```

## 4. Configure trade market of `buylowsellhi`
```bash
data='["'$trade_pair'","'0'","'$target_price'","'"$min_trade_amount"'","'"$max_trade_amount"'","",["'"$trade_updater"'"]]'
fucli -u "$url" push action "$buylowsellhi_contract" settrademkt "$data" -p "$buylowsellhi_contract@active"
```

## 5. Create trade permission for bot accounts

```bash
for bot in "${bot_users[@]}"; do
    authority='{"threshold":1,"keys":[],"accounts":[{"permission":{"actor":"'"$bot_admin"'","permission":"trade"},"weight":1}],"waits":[]'
    fucli -u $url set account permission $bot trade "$authority" active -p $bot@active
done
```

## 6. Link trade permission of bot accounts to `trade` Action of `tokenx.mm`
```bash
for user in "${bot_users[@]}"; do
    fucli -u "$url" set action permission "$user" "$tokenx_mm_contract" trade trade -p "$user@active"
done
```

## 7. Set bot market in `tokenx.mm` contract

```bash
fucli -u "$url" push action "$tokenx_mm_contract" setmarket '["'$trade_pair'", "'$market_fund_account'", "'$trade_pair'"]' -p "$bot_admin@active"
```

## 8. Fund Preparation and Deposit

```bash
fucli -u $url transfer $fund_account $tokenx_mm_contract "$deposit_flon" "addfund:$trade_pair" --contract $flon_token_contract
fucli -u $url transfer $fund_account $tokenx_mm_contract "$deposit_usdt" "addfund:$trade_pair" --contract $usdt_token_contract
```

## 9. Refresh market fund info

```bash
fucli -u "$url" push action "$tokenx_mm_contract" refreshfund '["'$trade_pair'"]' -p "$tokenx_mm_contract@active"
```

## 10. Check and Verify
```bash
# Check trade market of buylowsellhi
fucli -u "$url" get table "$buylowsellhi_contract" "$buylowsellhi_contract" trademarkets -L "$trade_pair" -l 1
# Check trade market of tokenx.mm
fucli -u "$url" get table "$tokenx_mm_contract" "$tokenx_mm_contract" botmarkets -L "$trade_pair" -l 1

# Verify bots trade
for user in "${bot_users[@]}"; do
    memo=$(od -An -N4 -tu4 /dev/urandom | tr -d ' \n')
    fucli -u "$url" push action "$tokenx_mm_contract" trade '["'"$user"'","'"$trade_pair"'","'$memo'"]' -p "$fee_payer@trade" -p "$user@trade"
done
```
## 11. Add trade pair Complete
If you need to upgrade the bot service, please refer to the [Bot Setup Guide](https://github.com/tychefi/pydexbot/blob/main/docs/add.trade_pair.bot.md)