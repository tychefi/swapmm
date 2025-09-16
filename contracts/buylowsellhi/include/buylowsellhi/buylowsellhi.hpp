#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>
#include <buylowsellhi/buylowsellhi.db.hpp>

namespace flon {

using std::string;

using namespace eosio;

/**
 * The `buylowsellhi` contract provides structures and actions for creating, managing, and operating trade markets and bots on flon-based blockchains. It enables users to create trade markets, set market parameters, manage updaters, and control bot groups for automated trading.
 *
 * Key features:
 * - Create, update, and delete trade markets with custom parameters and updaters.
 * - Set market pause status and target price via authorized updater accounts.
 * - Manage bot groups and assign updaters for flexible market operations.
 * - Query market and bot information using multi-index tables for efficient access.
 *
 * Data structures:
 * - Trade markets and bots are managed using internal multi-index tables, allowing fast lookup and flexible management.
 * - The contract supports querying market status, bot balances, and market parameters.
 *
 * This contract is suitable for projects requiring automated trading, market management, and flexible permission control on flon-based blockchains.
 */
class [[eosio::contract("buylowsellhi")]] buylowsellhi : public contract {
   public:
      using contract::contract;

      buylowsellhi(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
            _global(get_self(), get_self().value)
      {
         _gstate = _global.exists() ? _global.get() : global_t{};
      }
      ~buylowsellhi() { _global.set( _gstate, get_self() ); }

   /**
    * @brief Set the admin account
    * @param admin The account name to be set as admin
    */
   ACTION setadmin( const name& admin );

   /**
    * @brief Create or update a trade market.
    * @param trade_market_name The name of the trade market.
    * @param paused Whether the market is paused.
    * @param target_price The target price for the market.
    * @param min_trade_amount The minimum trade amount allowed in each trade.
    * @param max_trade_amount The maximum trade amount allowed in each trade.
    * @param memo Description or memo for the market.
    * @param updaters Set of updater account names.
    */
   ACTION settrademkt( name         trade_market_name,
                       bool         paused,
                       double       target_price,
                       asset        min_trade_amount,
                       asset        max_trade_amount,
                       string       memo,
                       set<name>    updaters );

   /**
    * @brief Set the pause status of a trade market.
    * @param updater The account requesting the update.
    * @param trade_market_name The name of the trade market.
    * @param paused Whether the market should be paused.
    */
   ACTION pause( const name& updater, const name& trade_market_name, bool paused );

   /**
    * @brief Set the target price of a trade market.
    * @param updater The account requesting the update.
    * @param trade_market_name The name of the trade market.
    * @param target_price The new target price.
    */
   ACTION setprice( const name& updater, const name& trade_market_name, double target_price );


   /**
    * @brief Set the minimum and maximum trade amount for a trade market.
    * @details Allows an authorized updater to set the minimum and maximum trade amounts for a specific trade market. This action does not affect the target price, only the allowed trade amount range.
    * @param updater The account requesting the update (must be authorized updater for the market).
    * @param trade_market_name The name of the trade market to update.
    * @param min_trade_amount The new minimum trade amount, it must be left side.
    * @param max_trade_amount The new maximum trade amount, it must be left side
    */
   ACTION setamount( const name& updater, const name& trade_market_name, const asset& min_trade_amount, const asset& max_trade_amount );

   /**
    * @brief Set the updaters for a trade market.
    * @param group_name The name of the trade market.
    * @param updaters Set of updater account names.
    */
   ACTION setupdaters( const name group_name, const set<name>& updaters );

   /**
    * @brief Delete a trade market.
    * @param group_name The name of the trade market to delete.
    */
   ACTION deltrademkt( const name& group_name );

   ACTION upgtrademkt( const name& trade_market_name);

   private:
      global_singleton    _global;
      global_t            _gstate;

      const name& require_admin_auth() const;

};


} //namespace flon
