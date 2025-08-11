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

namespace Status {
    static constexpr eosio::name    PENDING   { "pending"_n     };
    static constexpr eosio::name    RUNNING   { "running"_n     };
}

static constexpr uint64_t seconds_per_day                   = 24 * 3600;
static constexpr uint64_t order_expiry_duration             = seconds_per_day;
static constexpr uint64_t manual_order_expiry_duration      = 3 * seconds_per_day;

static constexpr symbol USDT                                = symbol(symbol_code("USDT"), 6);

#define TBL struct [[eosio::table, eosio::contract("tokenx_mm")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("tokenx_mm")]]

NTBL("global") global_t {
    name                        dex_contract        = "flon.swap"_n;
    name                        token_contract      = "flon.mtoken"_n;
    name                        usdt_contract       = "flon.mtoken"_n;
    name                        superadmin          = "flonian"_n;
    name                        bot_group_name;
    name                        price_mode_admin;
    symbol                      token_symbol;
    double                      fluctuation_ratio   = 0.1;    //10%
    double                      initial_token_price;        // in usdt
    asset                       invested_token_balance;     //transfer memo: refuel
    asset                       invested_usdt_balance;      //transfer memo: refuel
    asset                       token_balance;
    asset                       usdt_balance;
    name                        price_mode          = PriceMode::SIDEWAYS; //UPWARD | DOWNWARD | SIDEWAYS
    name                        status              = Status::PENDING;

    EOSLIB_SERIALIZE( global_t, (dex_contract)(token_contract)(usdt_contract)(superadmin)(bot_group_name)(price_mode_admin)(token_symbol)
                                (fluctuation_ratio)(initial_token_price)(invested_token_balance)(invested_usdt_balance)
                                (token_balance)(usdt_balance)(price_mode)(status) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

} //namespace flon
