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


#define TBL struct [[eosio::table, eosio::contract("buylowsellhi")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("buylowsellhi")]]

NTBL("global") global_t {
    name            admin = "flonian"_n;

    EOSLIB_SERIALIZE( global_t, (admin) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

TBL trade_market_t {
    name            trade_market_name;       // trading market name, PK
    bool            paused           = true; // is this market paused
    double          dest_price       = 0.0;  // dest price
    string          memo;
    set<name>       updaters;
    time_point      created_at;
    time_point      updated_at;

    uint64_t primary_key()const { return trade_market_name.value; }

    typedef eosio::multi_index< "trademarket"_n,  trade_market_t> idx_t;

    EOSLIB_SERIALIZE( trade_market_t, (trade_market_name)(paused)(dest_price)(memo)(updaters)(created_at)(updated_at) )
};

} //namespace flon
