#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>
#include <bot.mm/bot.mm.db.hpp>
#include <wasm_db.hpp>
#include <utils.hpp>

namespace flon {

using std::string;

using namespace wasm::db;
using namespace eosio;

enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   MEMO_FORMAT_ERROR    = 6,
   PAUSED               = 7,
   NO_AUTH              = 8,
   NOT_POSITIVE         = 9,
   NOT_STARTED          = 10,
   OVERSIZED            = 11,
   TIME_EXPIRED         = 12,
   NOTIFY_UNRELATED     = 13,
   ACTION_REDUNDANT     = 14,
   ACCOUNT_INVALID      = 15,
   FEE_INSUFFICIENT     = 16,
   FIRST_CREATOR        = 17,
   STATUS_ERROR         = 18,
   SCORE_NOT_ENOUGH     = 19,
   NEED_MANUAL_CHECK    = 20
};

/**
 * The `bot_mm` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for flon based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `bot_mm` contract instead of developing their own.
 *
 * The `bot_mm` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `bot_mm` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("bot.mm")]] bot_mm : public contract {
   private:
      dbc                 _db;

   public:
      using contract::contract;
  
   bot_mm(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
         _db(get_self()),
         _global(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }
    ~bot_mm() { _global.set( _gstate, get_self() ); }

   // ACTION init( const extended_asset& fee_info, const name& fee_collector );

   /**
    * @brief 
    * 
    * @param account 
    * @param actions 
    * @return ACTION 
    */
   ACTION creategroup( const name& group_name, const string& group_title, const vector<name>& accounts );
   ACTION addtogroup( const name& group_name, const vector<name>& accounts );
   ACTION rmfromgroup( const name& group_name, const vector<name>& accounts );
   ACTION setgroupdesc( const name group_name, const string& group_desc );
   ACTION deletegroup( const name& group_name );

   private:
      global_singleton    _global;
      global_t            _gstate;

   // private:

};


} //namespace flon
