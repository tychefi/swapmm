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

namespace PriceMode {   //price trend
    static constexpr eosio::name    UPWARD    { "upward"_n      };
    static constexpr eosio::name    DOWNWARD  { "downward"_n    };
    static constexpr eosio::name    SIDEWAYS  { "sideways"_n    }; //or range-bound
}

namespace TradeStatus {
    static constexpr eosio::name    PENDING   { "pending"_n     };
    static constexpr eosio::name    RUNNING   { "running"_n     };
}

static constexpr uint64_t seconds_per_day                   = 24 * 3600;
static constexpr uint64_t order_expiry_duration             = seconds_per_day;
static constexpr uint64_t manual_order_expiry_duration      = 3 * seconds_per_day;

static constexpr name SYSTEM_TOKEN                          = name("flon.token");
static constexpr symbol FLON                                = symbol(symbol_code("FLON"), 8);
static constexpr symbol USDT                                = symbol(symbol_code("USDT"), 6);

#define TBL struct [[eosio::table, eosio::contract("tokenx.mm")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("tokenx.mm")]]

struct dex_pool_side_t {
    extended_asset     balance;
    asset              total_quantity;

    EOSLIB_SERIALIZE( dex_pool_side_t, (balance)(total_quantity) )
};

static const dex_pool_side_t LEFT_SIDE_POOL = {
    .balance = extended_asset(asset(0, FLON), "flon.token"_n),
    .total_quantity = asset(0, FLON)
};

static const dex_pool_side_t RIGHT_SIDE_POOL = {
    .balance = extended_asset(asset(0, USDT), "flon.mtoken"_n),
    .total_quantity = asset(0, USDT)
};

NTBL("global") global_t {
    name            admin               = "flonian"_n;      // Administrator account name
    name            bot_mgr_contract    = "bot.mm"_n;       // bot manager contract name
    name            dex_contract        = "flon.swap"_n;    // DEX contract name for trading

    EOSLIB_SERIALIZE( global_t, (admin)(bot_mgr_contract)(dex_contract))
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

NTBL("botmarkets") bot_market_t {
    name            trade_pair_name;                // Market name
    name            fund_account;                   // bot manager contract name
    name            bot_group_name;                 // Bot group name for managing trading bots
    dex_pool_side_t left_pool;                      // Left side pool info
    dex_pool_side_t right_pool;                     // Right side pool info
    time_point      last_traded_at;


    uint64_t primary_key() const { return trade_pair_name.value; }

    typedef eosio::multi_index< "botmarkets"_n,  bot_market_t> idx_t;

    EOSLIB_SERIALIZE( bot_market_t, (trade_pair_name)(fund_account)(bot_group_name)
                                (left_pool)(right_pool)(last_traded_at))
};


} //namespace flon
