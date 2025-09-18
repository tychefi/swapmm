# Contract Initialization and Deployment Guide

## 1. Preparation

### Preparation Checklist

| Item          | Value                 | Description                                | is new |
| ------------- | --------------------- | ------------------------------------------ | ------ |
| fee_payer     | flonian               | Fee payer account, exists account          | 1      |
| bot_admin     | flonian               | Bot admin account, exists account          |        |
| fund_account  | flonian               | Fund account, must have assets(FLON, USDT) |        |
| trade_pubkey  | {YOUR_TRADE_PUBKEY}   | bot_trade_pubkey                           |        |
| trade_privkey | {YOUR_TRADE_PRIVKEY}  | bot_trade_privkey                          |        |
| deposit_flon  | 5000.000000 FLON      | total flon to deposit, asset type          |        |
| deposit_usdt  | 500.000000 USDT       | total usdt to deposit, asset type          |        |
| url           | https://m.flonscan.io | node RPC url                               |        |
| trade_pair    | flon.usdt             | trade pair name                            |        |

### ENV

```bash
# Prepare environment variables
export bot_admin="flonian"
export fund_account="flonian"
export trade_pubkey="${YOUR_TRADE_PUBKEY}"
export trade_privkey="${YOUR_TRADE_PRIVKEY}"
export deposit_flon="5000.00000000 FLON"
export deposit_usdt="500.000000 USDT"
export buylowsellhi_contract="buylowsellhi"
export bot_mm_contract="bot.mm"
export tokenx_mm_contract="tokenx.mm"
export trade_pair="flon.usdt"
export bot_users=("botuser11111" "botuser11112" "botuser11113")
export market_fund_account="fundflonusdt"
export fee_payer="flonian"
export url="https://m.flonscan.io"
export flon_token_contract="flon.token"
export usdt_token_contract="flon.mtoken"
```

## 2. Clone Repository and Build Contracts

```bash
git clone https://github.com/tychefi/swapmm.git
cd swapmm
bash build.sh
```

**Important: Please make sure your working directory is `swapmm`, otherwise subsequent scripts and commands may not work correctly.**

## 2. Create Accounts**
### Create Contract Accounts
```bash
# create bot_mm_contract account
fucli -u $url system newaccount $bot_admin $bot_mm_contract "$bot_admin@active" --fund-account "1.0 FLON" -p $bot_admin@active
fucli -u "$url" set account permission "$bot_mm_contract" active --add-code -p "$bot_mm_contract@active"
# create buylowsellhi account
fucli -u $url system newaccount $bot_admin $buylowsellhi_contract "$bot_admin@active" --fund-account "1.0 FLON" -p $bot_admin@active
fucli -u "$url" set account permission "$buylowsellhi_contract" active --add-code -p "$buylowsellhi_contract@active"
# create tokenx_mm_contract account
fucli -u $url system newaccount $bot_admin $tokenx_mm_contract "$bot_admin@active" --fund-account "1.0 FLON" -p $bot_admin@active
fucli -u "$url" set account permission "$tokenx_mm_contract" active --add-code -p "$tokenx_mm_contract@active"
```

### Create market fund account
```bash
fucli -u $url system newaccount $bot_admin "$market_fund_account" "$bot_admin@active" "$tokenx_mm_contract@active" --fund-account "1.0 FLON" -p $bot_admin@active
```

## 3. Upgrade Contracts

```bash
# bot.mm contract
# fucli -u $url set contract $bot_mm_contract build/contracts/bot.mm -p $bot_mm_contract@active
# buylowsellhi contract
fucli -u $url set contract $buylowsellhi_contract build/contracts/buylowsellhi -p $buylowsellhi_contract@active
# tokenx.mm contract
fucli -u $url set contract $tokenx_mm_contract build/contracts/tokenx.mm -p $tokenx_mm_contract@active
```

## 4. Upgrade data of trade market in `buylowsellhi` contract
```bash
# Note: The upgrade can only be performed once. If you have already executed this command, please skip this step.
fucli -u $url push action $buylowsellhi_contract upgtrademkt '["'$trade_pair'"]' -p $buylowsellhi_contract@active
```


## 8. Create trade Permission for bot accounts

```bash
for user in "${bot_users[@]}"; do
    authority='{"threshold":1,"keys":[],"accounts":[{"permission":{"actor":"'"$bot_admin"'","permission":"trade"},"weight":1}],"waits":[]}'
    fucli -u "$url" set account permission "$user" trade "$authority" active -p "$user@active"
done

```
## 9. Link trade Permission of bot accounts to `trade` Action of `tokenx.mm`
```bash
for user in "${bot_users[@]}"; do
    fucli -u "$url" set action permission "$user" "$tokenx_mm_contract" trade trade -p "$user@active"
done
```

## 10. Fund Preparation and Deposit

```bash
fucli -u $url transfer $fund_account $tokenx_mm_contract "$deposit_flon" "addfund:$trade_pair" --contract $flon_token_contract
fucli -u $url transfer $fund_account $tokenx_mm_contract "$deposit_usdt" "addfund:$trade_pair" --contract $usdt_token_contract
```

## 11. Check and Verify
```bash
# Check trade market of buylowsellhi
fucli -u "$url" get table "$buylowsellhi_contract" "$buylowsellhi_contract" trademarkets -L "$trade_pair" -l 1
# Check trade market of tokenx.mm
fucli -u "$url" get table "$tokenx_mm_contract" "$tokenx_mm_contract" botmarkets -L "$trade_pair" -l 1


for user in "${bot_users[@]}"; do
    memo=$(od -An -N4 -tu4 /dev/urandom | tr -d ' \n')
    fucli -u "$url" push action "$tokenx_mm_contract" trade '["'"$user"'","'"$trade_pair"'","'$memo'"]' -p "$fee_payer@trade" -p "$user@trade"
done
```

## Upgrade Complete
If you need to upgrade the bot service, please refer to the [Bot Setup Guide](https://github.com/tychefi/pydexbot/blob/main/docs/setup.pydexbot.md)
