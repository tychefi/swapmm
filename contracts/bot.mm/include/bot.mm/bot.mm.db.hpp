#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>


#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>


namespace flon {

using namespace std;
using namespace eosio;


#define TBL struct [[eosio::table, eosio::contract("bot_mm")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("bot_mm")]]

NTBL("global") global_t {
    name                        superadmin = "flonian"_n;
    
    EOSLIB_SERIALIZE( global_t, (superadmin) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

TBL bot_group_t {
    name                        group_name;     //PK
    string                      group_desc;
    set<name>                   bot_accounts;

    bot_group_t() {}
    bot_group_t(const name& n): group_name(n) {}

    uint64_t primary_key()const { return group_name.value; }

    typedef eosio::multi_index< "botgroups"_n,  bot_group_t> idx_t;

    EOSLIB_SERIALIZE( bot_group_t, (group_name)(group_desc)(bot_accounts) )
};

} //namespace flon
