# Set bot admin trade Permission and Link to `trade` action of tokenx.mm contract

## 1. Preparation

### Preparation Checklist

| Item             | Value                 | Description                                |
| ---------------- | --------------------- | ------------------------------------------ |
| bot_admin        | flonian               | Bot admin account, exists account          |
| fund_account     | flonian               | Fund account, must have assets(FLON, USDT) |
| trade_updater    | flonian               | trade updater, who can update target price |
| trade_pubkey     | {YOUR_TRADE_PUBKEY}   | bot_trade_pubkey                           |
| trade_privkey    | {YOUR_TRADE_PRIVKEY}  | bot_trade_privkey                          |
| target_price     | 0.1                   | target price, double type                  |
| min_trade_amount | 10.00000000 FLON      | min trade amount, asset type               |
| max_trade_amount | 30.00000000 FLON      | max trade amount, asset type               |
| deposit_flon     | 5000.000000 FLON      | total flon to deposit, asset type          |
| deposit_usdt     | 500.000000 USDT       | total usdt to deposit, asset type          |
| url              | https://m.flonscan.io | node RPC url                               |
| dex_contract     | flon.swap             | dex contract                               |
| trade_pair       | flon.usdt             | trade pair name                            |


## 2. Set ENV in your shell

```bash
export bot_admin="flonian"
export trade_pubkey="${YOUR_TRADE_PUBKEY}"
export tokenx_mm_contract="tokenx.mm"
export url="https://m.flonscan.io"

```

## 3. [optional] Create trade Permission for Admin Account if not exists

```bash
authority='{"threshold":1,"keys":[{"key":"'"$trade_pubkey"'","weight":1}],"accounts":[],"waits":[]}'
fucli -u $url set account permission $bot_admin trade "$authority" active -p $bot_admin@active
```

## 4. Link trade Permission of bot_admin to `trade` Action of `tokenx.mm`
```bash
fucli -u $url set action permission $bot_admin $tokenx_mm_contract trade trade -p $bot_admin@active
```

## 5. Get account info to verify

```bash
fucli -u $url get account $bot_admin
```
You should see the `trade` permission and linked action in the output.