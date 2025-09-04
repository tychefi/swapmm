
node_url="https://t1.flonscan.io"

bot_admin="flonian"
bot_users=("botuser11111" "botuser11112" "botuser11113")
tokenx_mm_contract="tokenxmm1111"

export node_url

updateauth_acct() {
    local user=$1
    local perm=$2
    local parent=$3
    local auth_actor=$4
    local auth_perm=$5

    echo "Updating permissions for user: $user, perm: $perm, parent: $parent, auth_actor: $auth_actor, auth_perm: $auth_perm"

    fucli -u "$node_url" push action flon updateauth '{
        "account": "'$user'",
        "permission": "'$perm'",
        "parent": "'$parent'",
        "auth": {
            "threshold": 1,
            "keys": [],
            "waits": [],
            "accounts": [
                {
                    "weight": 1,
                    "permission": {
                        "actor": "'$auth_actor'",
                        "permission": "'$auth_perm'"
                    }
                }
            ]
        }
    }' -p $user@$perm
}
for user in "${bot_users[@]}"; do
    updateauth_acct "$user" "active" "owner" "$tokenx_mm_contract" "active"
    updateauth_acct "$user" "owner" "" "$bot_admin" "active"
done