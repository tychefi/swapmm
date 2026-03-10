# Update trading parameters

## 1. Preparation

### ENV

```bash
# Prepare environment variables
buylowsellhi_contract="buylowsellhi"
trade_pair="sing.usdt"
url="https://m.flonscan.io"
```


## 2. Configure trade market of `buylowsellhi`
```bash
min_trade_amount="50.00000000 SING"
max_trade_amount="500.00000000 SING"
target_price=0.1
data='["'$trade_pair'","'0'","'$target_price'","'"$min_trade_amount"'","'"$max_trade_amount"'","",["'"$trade_updater"'"]]'
fucli -u "$url" push action "$buylowsellhi_contract" settrademkt "$data" -p "$buylowsellhi_contract@active"
```

## 3. Get current trade market config
```bash
fucli -u "$url" get table $buylowsellhi_contract $buylowsellhi_contract trademarkets
```

