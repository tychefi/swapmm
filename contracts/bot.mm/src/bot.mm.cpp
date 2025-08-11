#include <bot.mm/bot.mm.hpp>
#include <flon.token/flon.token.hpp>

static constexpr eosio::name active_permission{"active"_n};

namespace flon {
   using namespace std;

   #define CHECKC(exp, code, msg) \
      { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }

   // void bot_mm::init( const extended_asset& fee_info, const name& fee_collector) {
   //    CHECKC(has_auth(_self),  err::NO_AUTH, "no auth for operate");
      
   //    _gstate.fee_info           = fee_info;
   //    _gstate.fee_collector      = fee_collector;
   // }

   void bot_mm::creategroup(const name& group_name, const string& group_desc, const vector<name>& accounts ) {
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )

      auto group           = bot_group_t( group_name );
      CHECKC( !_db.get( group ), err::RECORD_EXISTING, "group already existing: " + group_name.to_string() )

      group.group_desc     = group_desc;

      for( auto& account : accounts ) {
         group.bot_accounts.insert( account );
      }

      _db.set( group );

   }

   void bot_mm::addtogroup( const name& group_name, const vector<name>& accounts ) {
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )

      auto group           = bot_group_t( group_name );
      CHECKC( _db.get( group ), err::RECORD_NOT_FOUND, "group not existing: " + group_name.to_string() )

      auto added           = false;
      for( const auto& account : accounts ) {
         if( group.bot_accounts.find( account ) != group.bot_accounts.end() ) continue;

         group.bot_accounts.insert( account );
         if( !added ) 
            added          = true;
      }

      CHECKC( added, err::RECORD_EXISTING, "must add non-existing accounts only" )

      _db.set( group );

   }

   void bot_mm::rmfromgroup( const name& group_name, const vector<name>& accounts ) {
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )

      auto group           = bot_group_t( group_name );
      CHECKC( _db.get( group ), err::RECORD_NOT_FOUND, "group not existing: " + group_name.to_string() )
      
      auto removed           = false;
      for( const auto& account : accounts ) {
         if( group.bot_accounts.find( account ) == group.bot_accounts.end() ) continue;

         group.bot_accounts.erase( account );

         if( !removed ) 
            removed          = true;
      }

      CHECKC( removed, err::RECORD_NOT_FOUND, "must remove existing accounts only" )

      _db.set( group );

   }

   void bot_mm::setgroupdesc( const name group_name, const string& group_desc ) {
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )

      auto group           = bot_group_t( group_name );
      CHECKC( _db.get( group ), err::RECORD_NOT_FOUND, "group not existing: " + group_name.to_string() )

      group.group_desc     = group_desc;
      _db.set( group );

   }

   void bot_mm::deletegroup( const name& group_name ) {
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )

      auto group           = bot_group_t( group_name );
      CHECKC( _db.get( group ), err::RECORD_NOT_FOUND, "group not existing: " + group_name.to_string() )

      _db.del( group );

   }
}
