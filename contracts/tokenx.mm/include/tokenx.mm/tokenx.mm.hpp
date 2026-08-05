#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>
#include <tokenx.mm/tokenx.mm.db.hpp>
// #include <wasm_db.hpp>
// #include <utils.hpp>

namespace flon {

using std::string;

// using namespace wasm::db;
using namespace eosio;


/**
 * The `tokenx_mm` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for flon based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `tokenx_mm` contract instead of developing their own.
 *
 * The `tokenx_mm` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `tokenx_mm` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("tokenx.mm")]] tokenx_mm : public contract {

   public:
      using contract::contract;

   tokenx_mm(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
         _global(get_self(), get_self().value)
   {
      _gstate = _global.exists() ? _global.get() : global_t{};
   }
   ~tokenx_mm() { _global.set( _gstate, get_self() ); }

   ACTION setadmin( const name& admin );

   ACTION cfgbotmgr( const name& bot_mgr_contract );


   ACTION setmarket( const name& trade_pair_name, const name& fund_account, const name& bot_group_name );

   ACTION trade( const name& bot, const name& trade_pair_name, const string& memo );
   ACTION buy( const name& bot, const name& trade_pair_name, const string& memo );
   ACTION sell( const name& bot, const name& trade_pair_name, const string& memo );

   [[eosio::on_notify("*::transfer")]]
   void on_transfer(const name& from, const name& to, const asset& quant, const string& memo);


   ACTION afterswap(const name& bot, const name& trade_pair_name, const name& side, const asset& input_quantity, const asset& bot_received_before);

   ACTION updatefund(const name& trade_pair_name, const asset& left_pool_balance, const asset& left_pool_total,
                     const asset& right_pool_balance, const asset& right_pool_total);

   ACTION refreshfund(const name& trade_pair_name);

   ACTION withdrawfund(const name& trade_pair_name, const extended_asset& quantity);

   using afterswap_action = eosio::action_wrapper<"afterswap"_n, &tokenx_mm::afterswap>;

   private:
      global_singleton    _global;
      global_t            _gstate;

   private:
      const name& require_admin_auth() const;
      void check_paused() const;
      void execute_trade( const name& bot, const name& trade_pair_name, const string& memo, const name& forced_side );
      void do_trade(const name& side, const bot_market_t& bot_market, dex_pool_side_t& input_pool, dex_pool_side_t& output_pool, double min_price, int64_t input_amount, const name& bot, size_t bot_size);

};

} //namespace flon
