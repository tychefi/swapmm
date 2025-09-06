# Contract Initialization and Deployment Guide

## 1. Preparation

### Preparation Checklist

| Item             | Value                | Description                                |
| ---------------- | -------------------- | ------------------------------------------ |
| bot_admin        | flonian              | Bot admin account, exists account          |
| fund_account     | flonian              | Fund account, must have assets(FLON, USDT) |
| trade_updater    | flonian              | trade updater, who can update target price |
| trade_pubkey     | {YOUR_TRADE_PUBKEY}  | bot_trade_pubkey                           |
| trade_privkey    | {YOUR_TRADE_PRIVKEY} | bot_trade_privkey                          |
| target_price     | 0.1                  | target price, double type                  |
| min_trade_amount | 10.00000000 FLON     | min trade amount, asset type               |
| max_trade_amount | 30.00000000 FLON     | max trade amount, asset type               |
| deposit_flon     | 5000.00000000 FLON   | total flon to deposit, asset type          |
| deposit_usdt     | 500.00000000 USDT    | total usdt to deposit, asset type          |
| url              | flonian              | node RPC url                               |
| dex_contract     | flon.swap            | dex contract                               |
| trade_pair       | flon.usdt            | trade pair name                            |

### ENV

```bash
# Prepare environment variables
export bot_admin="flonian"
export fund_account="flonian"
export trade_updater="flonian"
export trade_pubkey="{YOUR_TRADE_PUBKEY}"
export trade_privkey="{YOUR_TRADE_PRIVKEY}"
export target_price=0.1
export min_trade_amount="10.00000000 FLON"
export max_trade_amount="30.00000000 FLON"
export buylowsellhi_contract="buylowsellhi"
export bot_mm_contract="bot.mm"
export tokenx_mm_contract="tokenx.mm"
export trade_pair="flon.usdt"
export bot_users=("botuser11111" "botuser11112" "botuser11113")
export url="https://flonscan.io"
export dex_contract="flon.swap"
export trade_pair="flon.usdt"
export flon_token_contract="flon.token"
export usdt_token_contract="flon.mtoken"
```

## 2. Clone Repository and Build Contracts

```bash
git clone https://github.com/tychefi/swapmm.git
cd swapmm
bash build.sh
```

**重要提示：请确保工作目录在`swapmm`目录下，否则后续脚本和命令可能无法正确执行。**

## 2. Create Accounts**
### Create Contract Accounts
```bash
fucli -u $url system newaccount $bot_admin $bot_mm_contract "$bot_admin@active" --fund-account "1.0 FLON" -p $bot_admin@active
fucli -u $url system newaccount $bot_admin $buylowsellhi_contract "$bot_admin@active" --fund-account "1.0 FLON" -p $bot_admin@active
fucli -u $url system newaccount $bot_admin $tokenx_mm_contract "$bot_admin@active" --fund-account "1.0 FLON" -p $bot_admin@active
   ```

### Create Bot Accounts
```bash
for bot in "${bot_users[@]}"; do
    fucli -u $url system newaccount $bot_admin "$bot" "$bot_admin@active" --fund-account "1.0 FLON" -p $bot_admin@active
done
```

## 3. Deploy Contracts

```bash
# bot.mm contract
fucli -u $url set contract $bot_mm_contract build/contracts/bot.mm -p $bot_mm_contract@active
# buylowsellhi contract
fucli -u $url set contract $buylowsellhi_contract build/contracts/buylowsellhi -p $buylowsellhi_contract@active
# tokenx.mm contract
fucli -u $url set contract $tokenx_mm_contract build/contracts/tokenx.mm -p $tokenx_mm_contract@active
```

## 4. Configure admin of Contracts
```bash
# Set contract admin
fucli -u $url push action $bot_mm_contract setadmin '{"admin":"$bot_admin"}' -p $bot_mm_contract@active
fucli -u $url push action $buylowsellhi_contract setadmin '{"admin":"$bot_admin"}' -p $buylowsellhi_contract@active
fucli -u $url push action $tokenx_mm_contract setadmin '{"admin":"$bot_admin"}' -p $tokenx_mm_contract@active
```

## 5. Configure bot group
```bash
bots_json=$(printf '"%s",' "${bot_users[@]}" | sed 's/,$//')
fucli -u $url push action $bot_mm_contract setgroup '{"group_name":"'"$trade_pair"'","desc":"Default bot group for '"$trade_pair"'","bots":['"$bots_json"']}' -p $bot_mm_contract@active
```

## 6. Configure trade market
```bash
data='["'$trade_pair'","'$paused'","'$target_price'","'"$min_trade_amount"'","'"$max_trade_amount"'","",['$trade_updater']]}'
fucli -u "$url" push action "$buylowsellhi_contract" settrademkt "$data" -p "$buylowsellhi_contract@active"
```

## 7. [optional] configure bot manager contract of `tokenx.mm` if needed
```bash
fucli -u $url push action $tokenx_mm_contract cfgbotmgr '["'$bot_mm_contract'"]' -p $tokenx_mm_contract@active
```

## 8. Create trade Permission for Admin Account

```bash
authority='{"threshold":1,"keys":[{"key":"'"$trade_key"'","weight":1}],"accounts":[],"waits":[]}'
fucli -u $url set account permission $bot_admin trade "$authority" active -p $bot_admin@active
```
## 9. Link trade Permission of bot_admin to `exectrade` Action of `tokenx.mm`
```bash
fucli -u $url set action permission $bot_admin $tokenx_mm_contract exectrade trade -p $bot_admin@active
```


## 10. Fund Preparation and Deposit

```bash
fucli -u $url transfer flonian botmm1111111 "$deposit_flon" "init fund" --contract $flon_token_contract
fucli -u $url transfer flonian botuser11111 "$deposit_usdt" "init bot fund" --contract $usdt_token_contract
```

## 11. Check and Verify
```bash
memo=$(od -An -N4 -tu4 /dev/urandom | tr -d ' \n')
fucli -u "$url" push action "$tokenx_mm_contract" exectrade '{"'"$memo"'"}'
```

## 12. Setup pydexbot in server


**Login to server first, then clone pydexbot repo and setup config file**


```bash
# prepare env
export data_dir="/opt/data/pydexbot"
export privkey="${YOUR_TRADE_PRIVKEY}"
# clone pydexbot repo
git clone --recurse-submodules https://github.com/tychefi/pydexbot.git
cd pydexbot
# build docker image
bash build.docker.image.sh
# setup config file
bash setup.config.sh
# run pydexbot
cd $data_dir && run.sh
```
