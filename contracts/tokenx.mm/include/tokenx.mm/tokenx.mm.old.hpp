#pragma once

#include <tokenx.mm/tokenx.mm.db.hpp>


namespace flon {

// using namespace std;
// using namespace eosio;

// static constexpr symbol FLON                                = symbol(symbol_code("FLON"), 8);
// static constexpr symbol USDT                                = symbol(symbol_code("USDT"), 6);

// #define TBL struct [[eosio::table, eosio::contract("tokenx.mm")]]
// #define NTBL(name) struct [[eosio::table(name), eosio::contract("tokenx.mm")]]

struct dex_pool_side_old_t {
    extended_asset     balance;
    asset              total_quantity;

    EOSLIB_SERIALIZE( dex_pool_side_old_t, (balance)(total_quantity) )
};

static const dex_pool_side_old_t LEFT_SIDE_POOL = {
    .balance = extended_asset(asset(0, FLON), "flon.token"_n),
    .total_quantity = asset(0, FLON)
};

static const dex_pool_side_old_t RIGHT_SIDE_POOL = {
    .balance = extended_asset(asset(0, USDT), "flon.mtoken"_n),
    .total_quantity = asset(0, USDT)
};

// scope: self
// For upgrade from old version
struct global_old_t {
    name            admin               = "flonian"_n;      // Administrator account name
    name            bot_mgr_contract    = "bot.mm"_n;       // bot manager contract name
    name            dex_contract        = "flon.swap"_n;    // DEX contract name for trading
    name            trade_pair_name     = "flon.usdt"_n;    // Trading pair name
    name            bot_group_name      = "flon.usdt"_n;    // Bot group name for managing trading bots
    dex_pool_side_t left_pool           = LEFT_SIDE_POOL;   // Left side pool info
    dex_pool_side_t right_pool          = RIGHT_SIDE_POOL;  // Right side pool info
    double          max_slippage        = 0.1;              // Maximum allowed slippage (default: 10%)
    double          fluctuation_ratio   = 0.1;              // Price fluctuation ratio for sideways market (default: 10%)
    uint32_t        min_trade_seconds   = 10;               // Minimum interval between trades in seconds (default: 10)
    uint32_t        max_trade_seconds   = 30;               // Maximum interval between trades in seconds (default: 30)


    EOSLIB_SERIALIZE( global_old_t, (admin)(bot_mgr_contract)(dex_contract)(trade_pair_name)(bot_group_name)
                                (left_pool)(right_pool)(max_slippage)(fluctuation_ratio)(min_trade_seconds)
                                (max_trade_seconds))
};
typedef eosio::singleton< "global"_n, global_old_t > global_singleton_old;

} //namespace flon
