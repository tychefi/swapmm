# Update trading parameters

## 1. Preparation

### ENV

```bash
# Prepare environment variables
buylowsellhi_contract="buylowsellhi"
trade_pair="sing.usdt"
trade_updater="flonian"
url="https://m.flonscan.io"
```


## 2. Configure trade market of `buylowsellhi`
```bash
min_trade_amount="10.00000000 SING"
max_trade_amount="30.00000000 SING"
target_price=0.1
data='["'$trade_pair'","'$target_price'","'"$min_trade_amount"'","'"$max_trade_amount"'","",["'"$trade_updater"'"]]'
fucli -u "$url" push action "$buylowsellhi_contract" settrademkt "$data" -p "$buylowsellhi_contract@active"
```

## 3. Tune slippage and rhythm

```bash
fucli -u "$url" push action "$buylowsellhi_contract" setslippage '["'$trade_updater'","'$trade_pair'",0.005,0.015]' -p "$trade_updater@active"
fucli -u "$url" push action "$buylowsellhi_contract" settradeint '["'$trade_pair'",20,75]' -p "$buylowsellhi_contract@active"
```

## 4. Get current trade market config
```bash
fucli -u "$url" get table $buylowsellhi_contract $buylowsellhi_contract trademarkets
```
