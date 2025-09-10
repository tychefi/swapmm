#include <buylowsellhi/buylowsellhi.hpp>
#include <flon/utils.hpp>
#include <contract_version.hpp>

static constexpr eosio::name active_permission{"active"_n};

namespace flon {

   using namespace std;

   DEFINE_VERSION_CONTRACT_CLASS("buylowsellhi", buylowsellhi)

   const name& buylowsellhi::require_admin_auth() const {
      if (_gstate.admin.value != 0 && has_auth(_gstate.admin)) {
         return _gstate.admin;
      } else if (has_auth(_self)) {
         return _self;
      } else {
         CHECKC(false, err::NO_AUTH, "miss self or admin authorization");
      }
   }

   void buylowsellhi::setadmin( const name& admin ) {
      require_admin_auth();

      _gstate.admin = admin;
   }

   void buylowsellhi::settrademkt(  name trade_market_name,
                                    bool paused,
                                    double target_price,
                                    asset min_trade_amount,
                                    asset max_trade_amount,
                                    string memo,
                                    set<name> updaters ) {

      auto auth_admin = require_admin_auth();

      auto markets = trade_market_t::idx_t( get_self(), get_self().value );
      auto itr = markets.find( trade_market_name.value );
      if ( itr == markets.end() ) {
         markets.emplace( auth_admin, [&] (auto& row) {
            row.trade_market_name   = trade_market_name;
            row.paused              = paused;
            row.target_price        = target_price;
            row.min_trade_amount    = min_trade_amount;
            row.max_trade_amount    = max_trade_amount;
            row.memo                = memo;
            row.updaters            = updaters;
            row.created_at          = current_time_point();
            row.updated_at          = row.created_at;
         } );
      } else {
         markets.modify( itr, auth_admin, [&] (auto& row) {
            row.paused              = paused;
            row.target_price        = target_price;
            row.min_trade_amount    = min_trade_amount;
            row.max_trade_amount    = max_trade_amount;
            row.memo                = memo;
            row.updaters            = updaters;
            row.updated_at          = current_time_point();
         } );
      }
   }

   void buylowsellhi::setpause( const name& updater, const name& trade_market_name, bool paused ) {
      require_auth( updater );

      auto markets   = trade_market_t::idx_t( get_self(), get_self().value );
      auto itr       = markets.find( trade_market_name.value );
      CHECKC( itr    != markets.end(), err::RECORD_NOT_FOUND, "market not existing: " + trade_market_name.to_string() );
      CHECKC( updater == get_self() || updater == _gstate.admin || itr->updaters.count( updater ), err::NO_AUTH, "updater no permission:" + updater.to_string() );
      CHECKC( itr->paused != paused, err::PARAM_ERROR, "market already in desired state" )

      markets.modify( itr, same_payer, [&] (auto& row) {
         row.paused              = paused;
         row.updated_at          = current_time_point();
      } );
   }


   void buylowsellhi::setprice( const name& updater, const name& trade_market_name, double target_price ) {
      require_auth( updater );

      auto markets   = trade_market_t::idx_t( get_self(), get_self().value );
      auto itr       = markets.find( trade_market_name.value );
      CHECKC( itr    != markets.end(), err::RECORD_NOT_FOUND, "market not exising: " + trade_market_name.to_string() )

      CHECKC( updater == get_self() || updater == _gstate.admin || itr->updaters.count( updater ), err::NO_AUTH, "updater no permission:" + updater.to_string() );

      CHECKC( itr->target_price != target_price, err::PARAM_ERROR, "market price no change" )

      markets.modify( itr, same_payer, [&] (auto& row) {
         row.target_price        = target_price;
         row.updated_at          = current_time_point();
      } );
   }


   void buylowsellhi::setupdaters( const name group_name, const set<name>& updaters ) {
      require_admin_auth();

      auto markets    = trade_market_t::idx_t( get_self(), get_self().value );
      auto itr       = markets.find( group_name.value );
      CHECKC( itr != markets.end(), err::RECORD_NOT_FOUND, "market not existing: " + group_name.to_string() )

      markets.modify( itr, same_payer, [&] (auto& row) {
         row.updaters            = updaters;
         row.updated_at          = current_time_point();
      } );
   }

   void buylowsellhi::deltrademkt( const name& trade_market_name ) {
      require_admin_auth();

      auto markets   = trade_market_t::idx_t( get_self(), get_self().value );
      auto itr       = markets.find( trade_market_name.value );
      CHECKC( itr    != markets.end(), err::RECORD_NOT_FOUND, "market not existing: " + trade_market_name.to_string() )

      markets.erase( itr );
   }
}
