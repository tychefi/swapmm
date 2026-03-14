#include <bot.mm/bot.mm.hpp>
#include <flon/utils.hpp>
#include <contract_version.hpp>

static constexpr eosio::name active_permission{"active"_n};

namespace flon {
   using namespace std;

   DEFINE_VERSION_CONTRACT_CLASS("bot.mm", bot_mm)

   const name& bot_mm::require_admin_auth() const {
      if (_gstate.admin.value != 0 && has_auth(_gstate.admin)) {
         return _gstate.admin;
      } else if (has_auth(_self)) {
         return _self;
      } else {
         CHECKC(false, err::NO_AUTH, "miss self or admin authorization");
         __builtin_unreachable();
      }
   }

   void bot_mm::setadmin( const name& admin ) {
      require_admin_auth();

      _gstate.admin = admin;
   }

   void bot_mm::setgroup(const name& group_name, const string& desc, const set<name>& bots ) {

      const auto& admin = require_admin_auth();

      auto groups           = bot_group_t::idx_t( get_self(), get_self().value );
      auto itr = groups.find( group_name.value );
      if ( itr == groups.end() ) {
         groups.emplace( admin, [&] (auto& row) {
            row.group_name = group_name;
            row.desc = desc;
            row.bots   = bots;
         } );
      } else {
         groups.modify( itr, admin, [&] (auto& row) {
            row.desc = desc;
            row.bots = bots;
         } );
      }
   }

   void bot_mm::addtogroup( const name& group_name, const set<name>& bots ) {
      require_admin_auth();

      auto groups    = bot_group_t::idx_t( get_self(), get_self().value );
      auto itr       = groups.find( group_name.value );
      CHECKC( itr == groups.end(), err::RECORD_NOT_FOUND, "group not existed: " + group_name.to_string() )

      bool added = false;
      groups.modify( itr, same_payer, [&] (auto& row) {
         for (const auto& a : bots) {
            auto ret = row.bots.insert( a );
            if (ret.second) {
               added = true;
            }
         }
      } );

      CHECKC( added, err::RECORD_EXISTING, "No new account added." )

   }

   void bot_mm::rmfromgroup( const name& group_name, const set<name>& bots ) {
      require_admin_auth();

      auto groups    = bot_group_t::idx_t( get_self(), get_self().value );
      auto itr       = groups.find( group_name.value );
      CHECKC( itr == groups.end(), err::RECORD_NOT_FOUND, "group not existed: " + group_name.to_string() )

      bool removed = false;
      groups.modify( itr, same_payer, [&] (auto& row) {
         for (const auto& a : bots) {
            auto size = row.bots.erase( a );
            if (size > 0) {
               removed = true;
            }
         }
      } );

      CHECKC( removed, err::RECORD_NOT_FOUND, "No account removed." )
   }

   void bot_mm::setgroupdesc( const name group_name, const string& desc ) {
      require_admin_auth();

      auto groups    = bot_group_t::idx_t( get_self(), get_self().value );
      auto itr       = groups.find( group_name.value );
      CHECKC( itr == groups.end(), err::RECORD_NOT_FOUND, "group not existed: " + group_name.to_string() )

      groups.modify( itr, same_payer, [&] (auto& row) {
         row.desc = desc;
      } );
   }

   void bot_mm::deletegroup( const name& group_name ) {
      require_admin_auth();

      auto groups    = bot_group_t::idx_t( get_self(), get_self().value );
      auto itr       = groups.find( group_name.value );
      CHECKC( itr == groups.end(), err::RECORD_NOT_FOUND, "group not existed: " + group_name.to_string() )

      groups.erase( itr );
   }
}
