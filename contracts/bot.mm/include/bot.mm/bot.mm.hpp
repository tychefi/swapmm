#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>
#include <bot.mm/bot.mm.db.hpp>

namespace flon {

using std::string;

using namespace eosio;

/**
 * The `bot_mm` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for flon based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `bot_mm` contract instead of developing their own.
 *
 * The `bot_mm` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `bot_mm` contract manages the set of tokens, bots and their corresponding balances, by using two internal multi-index structures: the `bots` and `stats`. The `bots` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `bots` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `bots` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("bot.mm")]] bot_mm : public contract {
   public:
      using contract::contract;

      bot_mm(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
            _global(get_self(), get_self().value)
      {
         _gstate = _global.exists() ? _global.get() : global_t{};
      }
      ~bot_mm() { _global.set( _gstate, get_self() ); }

   /**
    * @brief Set the admin account
    * @param admin The account name to be set as admin
    */
   ACTION setadmin( const name& admin );

   /**
    * @brief Create or update a bot group
    * @param group_name The name of the group
    * @param desc Description of the group
    * @param bots Set of bot account names
    */
   ACTION setgroup( const name& group_name, const string& desc, const set<name>& bots );

   /**
    * @brief Add bot accounts to a group
    * @param group_name The name of the group
    * @param bots Set of bot account names to add
    */
   ACTION addtogroup( const name& group_name, const set<name>& bots );

   /**
    * @brief Remove bot accounts from a group
    * @param group_name The name of the group
    * @param bots Set of bot account names to remove
    */
   ACTION rmfromgroup( const name& group_name, const set<name>& bots );

   /**
    * @brief Set group description
    * @param group_name The name of the group
    * @param desc New description for the group
    */
   ACTION setgroupdesc( const name group_name, const string& desc );

   /**
    * @brief Delete a bot group
    * @param group_name The name of the group
    */
   ACTION deletegroup( const name& group_name );

   private:
      global_singleton    _global;
      global_t            _gstate;

      const name& require_admin_auth() const;

};


} //namespace flon
