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

static constexpr symbol FLON                                = symbol(symbol_code("FLON"), 8);
static constexpr symbol USDT                                = symbol(symbol_code("USDT"), 6);

#define TBL struct [[eosio::table, eosio::contract("tokenx_mm")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("tokenx_mm")]]

NTBL("global") global_t {
    name     admin               = "flonian"_n;        // Administrator account name
    name     bots_contract       = "bot.mm"_n;         // bot manager contract name
    name     dex_contract        = "flon.swap"_n;      // DEX contract name for trading
    name     trade_pair_name     = "flon.usdt"_n;      // Trading pair name
    name     left_contract       = "flon.token"_n;     // Contract name for the left-side asset
    name     right_contract      = "flon.mtoken"_n;    // Contract name for the right-side asset
    name     bot_group_name      = "flon.usdt"_n;        // Bot group name for managing trading bots
    asset    left_balance       = asset(0, FLON);      // Current balance of the left-side asset
    asset    right_balance      = asset(0, USDT);      // Current balance of the right-side asset
    asset    left_total_quant   = asset(0, FLON);      // Total quantity of the left-side asset
    asset    right_total_quant  = asset(0, USDT);      // Total quantity of the right-side asset
    double   max_slippage        = 0.1;                // Maximum allowed slippage (default: 10%)
    double   fluctuation_ratio   = 0.1;                // Price fluctuation ratio for sideways market (default: 10%)
    uint32_t min_trade_seconds   = 10;                 // Minimum interval between trades in seconds (default: 10)
    uint32_t max_trade_seconds   = 30;                 // Maximum interval between trades in seconds (default: 30)
    asset    min_trade_amount   = asset(0, FLON);      // Minimum amount per trade
    asset    max_trade_amount   = asset(0, FLON);      // Maximum amount per trade


    EOSLIB_SERIALIZE( global_t, (admin)(dex_contract)(trade_pair_name)(left_contract)(right_contract)(bot_group_name)
                                (left_balance)(right_balance)(left_total_quant)(right_total_quant)(max_slippage)(fluctuation_ratio)
                                (min_trade_seconds)(max_trade_seconds)(min_trade_amount)(max_trade_amount) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

} //namespace flon
