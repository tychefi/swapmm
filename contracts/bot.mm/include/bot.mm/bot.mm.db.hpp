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


#define TBL struct [[eosio::table, eosio::contract("bot.mm")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("bot.mm")]]

NTBL("global") global_t {
    name                        admin = "flonian"_n;

    EOSLIB_SERIALIZE( global_t, (admin) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

// scope: self
TBL bot_group_t {
    name                        group_name;     //PK
    string                      desc;
    set<name>                   bots;

    uint64_t primary_key()const { return group_name.value; }

    typedef eosio::multi_index< "botgroups"_n,  bot_group_t> idx_t;

    EOSLIB_SERIALIZE( bot_group_t, (group_name)(desc)(bots) )
};

} //namespace flon
