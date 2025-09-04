

bot_mm_contract="botmm1111111"
buylowsellhi_contract="buylowsellhi"
tokenx_mm_contract="tokenxmm1111"
bot_admin="flonian"
node_url="https://t1.flonscan.io"
tp_code="flon.usdt"
bot_users=("botuser11111" "botuser11112" "botuser11113")

alias tcli="fucli -u $node_url"

tcli get table $buylowsellhi_contract $buylowsellhi_contract trademarkets


tcli get table $bot_mm_contract $bot_mm_contract botgroups

tcli get table $tokenx_mm_contract $tokenx_mm_contract global