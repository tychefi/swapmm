#include <tokenx.mm/tokenx.mm.hpp>
#include <flon.token/flon.token.hpp>

static constexpr eosio::name active_permission{"active"_n};

namespace flon {
   using namespace std;

   #define CHECKC(exp, code, msg) \
      { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }

   void tokenx_mm::init(const name& bot_group_name, const name& price_mode_admin, 
                        const double& fluct_ratio, const double& init_token_price ) {
      
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )
      CHECKC( is_account( price_mode_admin ), err::ACCOUNT_INVALID, "account invalid: " + price_mode_admin.to_string() )

      _gstate.bot_group_name     = bot_group_name;
      _gstate.price_mode_admin   = price_mode_admin;
      _gstate.fluctuation_ratio  = fluct_ratio;
      _gstate.initial_token_price= init_token_price;

   }

   void tokenx_mm::pause() {
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )

      CHECKC( _gstate.trade_status != TradeStatus::PENDING, err::STATUS_ERROR, "already paused" )

      _gstate.trade_status = TradeStatus::PENDING;
   }

   void tokenx_mm::resume() {
      CHECKC( has_auth( _self ) || has_auth( _gstate.superadmin ), err::NO_AUTH, "neither self nor superadmin" )

      CHECKC( _gstate.trade_status != TradeStatus::RUNNING, err::STATUS_ERROR, "already running" )

      _gstate.trade_status = TradeStatus::RUNNING;
   }

   /// Perm: price_mode_admin
   void tokenx_mm::setpricemode( const name& submitter, const name& price_mode) {
      require_auth( submitter );

      CHECKC( _gstate.price_mode_admin == submitter, err::NO_AUTH, "not price_mode_admin: " + submitter.to_string() )
      CHECKC( _gstate.price_mode != price_mode, err::PARAM_ERROR, "price mode unchanged" )
      
      _gstate.price_mode        = price_mode;

   }

   //TODO
   ///Perm: bot accounts
   void tokenx_mm::exectrade( const name& bot_account ) {
      require_auth( bot_account );

      CHECKC( _gstate.trade_status == TradeStatus::RUNNING, err::NOT_STARTED, "system paused" )

      _check_bot( bot_account );

      switch( _gstate.price_mode.value ) {
         case PriceMode::UPWARD.value: _process_buy(); break;
         case PriceMode::DOWNWARD.value: _process_sell(); break;
         case PriceMode::SIDEWAYS.value: if( _even_odds_buy() ) _process_buy(); else _process_sell(); break;
         default: CHECKC( false, err::PARAM_ERROR, "Invalid Price Mode" ) 
      }
   }

   void tokenx_mm::on_transfer(const name& from, const name& to, const asset& quant, const string& memo) {
      if (from == get_self() || to != get_self()) return;

      CHECKC( from != to, err::ACCOUNT_INVALID, "cannot transfer to self" );
      CHECKC( quant.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )

      if( memo == "refuel" ) {
         _process_plan_investment( quant );

      } else if ( memo == "" ) {
         _process_trade_settlement( quant );
      }

      // CHECKC( false, err::MEMO_FORMAT_ERROR, "invalid memo" )
   }

   void tokenx_mm::_check_bot( const name& bot_account ) {

   }

   void tokenx_mm::_process_plan_investment( const asset& quant ) {
      if( quant.symbol == USDT ) {
         _gstate.invested_usdt_balance.amount   += quant.amount; 
         _gstate.usdt_balance.amount            += quant.amount;

      } else {
         CHECKC( _gstate.token_symbol == quant.symbol, err::PARAM_ERROR, "quant symobl invalid" )

         _gstate.invested_token_balance.amount  += quant.amount;
         _gstate.token_balance.amount           += quant.amount;
      }
   }

   void tokenx_mm::_process_trade_settlement( const asset& quant ) {
      if( quant.symbol == USDT ) {
          _gstate.usdt_balance.amount           += quant.amount;

      } else {
         CHECKC( _gstate.token_symbol == quant.symbol, err::PARAM_ERROR, "quant symobl invalid" )

         _gstate.token_balance.amount           += quant.amount;
      }
   }

   //generate a random boolean with even chance (50% probability)
   bool tokenx_mm::_even_odds_buy() {
      // Use transaction ID and current time for pseudo-randomness
      auto tapos = eosio::tapos_block_prefix();
      auto timestamp = current_time_point().sec_since_epoch();

      // Combine tapos and timestamp for a seed
      uint64_t seed = tapos ^ timestamp; //bitwise XOR

      // Generate a pseudo-random number and check if even/odd for 50% probability
      return( (seed % 2) == 0 );
   }

   void tokenx_mm::_process_buy() {

   }

   void tokenx_mm::_process_sell() {

   }
}
