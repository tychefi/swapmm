# Update trading parameters

## 1. Preparation

### ENV

```bash
# Prepare environment variables
buylowsellhi_contract="buylowsellhi"
trade_updater="flonian"
url="https://m.flonscan.io"
```


## 2. Configure trade markets of `buylowsellhi`
```bash
trade_pair="flon.usdt"
min_trade_amount="0.20000000 FLON"
max_trade_amount="0.80000000 FLON"
target_price=0.00323
data='["'$trade_pair'","'$target_price'","'"$min_trade_amount"'","'"$max_trade_amount"'","",["'"$trade_updater"'"]]'
fucli -u "$url" push action "$buylowsellhi_contract" settrademkt "$data" -p "$buylowsellhi_contract@active"

trade_pair="sing.usdt"
min_trade_amount="1.00000000 SING"
max_trade_amount="3.00000000 SING"
target_price=0.1
data='["'$trade_pair'","'$target_price'","'"$min_trade_amount"'","'"$max_trade_amount"'","",["'"$trade_updater"'"]]'
fucli -u "$url" push action "$buylowsellhi_contract" settrademkt "$data" -p "$buylowsellhi_contract@active"
```

## 3. Tune slippage and rhythm

```bash
trade_pair="flon.usdt"
fucli -u "$url" push action "$buylowsellhi_contract" setslippage '["'$trade_updater'","'$trade_pair'",0.006,0.003]' -p "$trade_updater@active"

trade_pair="sing.usdt"
fucli -u "$url" push action "$buylowsellhi_contract" setslippage '["'$trade_updater'","'$trade_pair'",0.006,0.003]' -p "$trade_updater@active"
```

After deploying a `buylowsellhi` build that contains `settradeint`, spread the contract cadence:

```bash
trade_pair="flon.usdt"
fucli -u "$url" push action "$buylowsellhi_contract" settradeint '["'$trade_pair'",90,240]' -p "$buylowsellhi_contract@active"

trade_pair="sing.usdt"
fucli -u "$url" push action "$buylowsellhi_contract" settradeint '["'$trade_pair'",330,600]' -p "$buylowsellhi_contract@active"
```

## 4. Get current trade market config
```bash
fucli -u "$url" get table $buylowsellhi_contract $buylowsellhi_contract trademarkets -L flon.usdt -U flon.usdt -l 1
fucli -u "$url" get table $buylowsellhi_contract $buylowsellhi_contract trademarkets -L sing.usdt -U sing.usdt -l 1
```
