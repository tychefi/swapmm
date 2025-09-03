#include <tokenx.mm/tokenx.mm.hpp>
#include <flon/token.protocol.hpp>

#include <flon/utils.hpp>
#include <contract_version.hpp>

static constexpr eosio::name active_permission{"active"_n};

namespace flon {
   using namespace std;

   // scope: buylowsellhi contract
   struct trade_market_t {
      name            trade_market_name;           // trading market name, PK
      bool            paused              = true;  // is this market paused
      double          target_price        = 0.0;   // target price
      string          memo;

    uint64_t primary_key()const { return trade_market_name.value; }

    typedef eosio::multi_index< "trademarkets"_n,  trade_market_t> idx_t;
   };

   // scope: bots contract
   struct bot_group_t {
      name                        group_name;     //PK
      string                      desc;
      vector<name>                bots;

      uint64_t primary_key()const { return group_name.value; }

      typedef eosio::multi_index< "botgroups"_n,  bot_group_t> idx_t;

      EOSLIB_SERIALIZE( bot_group_t, (group_name)(desc)(bots) )
   };


   //scope: swap contract
   TBL swap_market_t {
      name           tpcode;                             // PK, Full name liquidity symbol: left_pool_quant.right_pool_quant
      symbol_code    liquidity_symbol;                   // Short Name liquidity symbol: L left_pool_quant[0:3] right_pool_quant[0:3]
                                                         // admin 可以指定liquidity_symbol
      extended_asset left_pool_quant;
      extended_asset right_pool_quant;

      uint64_t primary_key() const { return tpcode.value; }

      typedef eosio::multi_index<"markets"_n, swap_market_t>idx_t;
   };

   DEFINE_VERSION_CONTRACT_CLASS("tokenx.mm", tokenx_mm)

   static uint32_t get_random() {
    // Use transaction ID and current time for pseudo-randomness
    // Use tapos block prefix and current time, then XOR to form a seed
    uint32_t tapos = tapos_block_prefix();
    uint32_t timestamp = current_time_point().sec_since_epoch();
    uint64_t seed = uint64_t(tapos) ^ uint64_t(timestamp);

    uint32_t adata_size = action_data_size();

    // pack into a buffer: seed, account, action data size
    char buf[sizeof(seed) + sizeof(adata_size)];
    size_t offset = 0;
    std::memcpy(buf + offset, &seed, sizeof(seed)); offset += sizeof(seed);
    std::memcpy(buf + offset, &adata_size, sizeof(adata_size)); offset += sizeof(adata_size);

    checksum256 h = sha256(buf, offset);
    auto arr = h.extract_as_byte_array();
    uint32_t v = (uint32_t(arr[0]) << 24) | (uint32_t(arr[1]) << 16) | (uint32_t(arr[2]) << 8) | uint32_t(arr[3]);
   //  uint32_t r = (v % range) + 1;
    return v;
}

   void tokenx_mm::require_admin_auth() const {
      CHECKC(has_auth(_self) || (_gstate.admin.value != 0 && has_auth(_gstate.admin)),
         err::NO_AUTH, "miss self or admin authorization");
   }

   void tokenx_mm::setadmin( const name& admin ) {
      require_admin_auth();
      _gstate.admin = admin;
   }


    void tokenx_mm::cfgbots( const name& bots_contract ) {
      require_admin_auth();
      _gstate.bots_contract = bots_contract;
    }

   // void tokenx_mm::init(const name& bot_group_name, const name& price_mode_admin,
   //                      const double& fluct_ratio, const double& init_token_price ) {

   //    CHECKC( has_auth( _self ) || has_auth( _gstate.admin ), err::NO_AUTH, "neither self nor admin" )
   //    CHECKC( is_account( price_mode_admin ), err::ACCOUNT_INVALID, "account invalid: " + price_mode_admin.to_string() )

   //    _gstate.bot_group_name     = bot_group_name;
   //    _gstate.price_mode_admin   = price_mode_admin;
   //    _gstate.fluctuation_ratio  = fluct_ratio;
   //    _gstate.initial_token_price= init_token_price;

   // }

   // void tokenx_mm::pause() {
   //    CHECKC( has_auth( _self ) || has_auth( _gstate.admin ), err::NO_AUTH, "neither self nor admin" )

   //    CHECKC( _gstate.trade_status != TradeStatus::PENDING, err::STATUS_ERROR, "already paused" )

   //    _gstate.trade_status = TradeStatus::PENDING;
   // }

   // void tokenx_mm::resume() {
   //    CHECKC( has_auth( _self ) || has_auth( _gstate.admin ), err::NO_AUTH, "neither self nor admin" )

   //    CHECKC( _gstate.trade_status != TradeStatus::RUNNING, err::STATUS_ERROR, "already running" )

   //    _gstate.trade_status = TradeStatus::RUNNING;
   // }

   /// Perm: price_mode_admin
   // void tokenx_mm::setpricemode( const name& submitter, const name& price_mode) {
   //    require_auth( submitter );

   //    CHECKC( _gstate.price_mode_admin == submitter, err::NO_AUTH, "not price_mode_admin: " + submitter.to_string() )
   //    CHECKC( _gstate.price_mode != price_mode, err::PARAM_ERROR, "price mode unchanged" )

   //    _gstate.price_mode        = price_mode;

   // }

   //TODO
   ///Perm: bot accounts
   void tokenx_mm::exectrade( const string& memo ) {
      require_admin_auth();

      // contract name: buylowsellhi = 4520798682350377696
      auto markets = trade_market_t::idx_t( name(4520798682350377696), 4520798682350377696 );
      auto market_itr = markets.find(_gstate.trade_pair_name.value);
      CHECKC( market_itr != markets.end(), err::RECORD_NOT_FOUND, "market not existed: " + _gstate.trade_pair_name.to_string() )
      CHECKC( market_itr->target_price > 0, err::STATUS_ERROR, "invalid market target price" )

      auto bot_groups = bot_group_t::idx_t( _gstate.bots_contract, get_self().value );
      auto bot_group_itr = bot_groups.find(_gstate.bot_group_name.value);
      CHECKC( bot_group_itr != bot_groups.end(), err::RECORD_NOT_FOUND, "bot group not existed: " + _gstate.bot_group_name.to_string() )
      CHECKC( bot_group_itr->bots.size() > 0, err::RECORD_NOT_FOUND, "bot group has no bot: " + _gstate.bot_group_name.to_string() )

      auto swap_markets = swap_market_t::idx_t( get_self(), get_self().value );
      auto swap_market_itr = swap_markets.find( _gstate.trade_pair_name.value );
      CHECKC( swap_market_itr != swap_markets.end(), err::RECORD_NOT_FOUND, "swap market not existed: " + _gstate.trade_pair_name.to_string() )
      CHECKC( swap_market_itr->left_pool_quant.contract == _gstate.left_contract, err::PARAM_ERROR, "left pool token contract mismatch" )
      CHECKC( swap_market_itr->left_pool_quant.quantity.symbol == _gstate.left_balance.symbol, err::PARAM_ERROR, "left pool symbol mismatch" )
      CHECKC( swap_market_itr->right_pool_quant.contract == _gstate.right_contract, err::PARAM_ERROR, "right pool token contract mismatch" )
      CHECKC( swap_market_itr->right_pool_quant.quantity.symbol == _gstate.right_balance.symbol, err::PARAM_ERROR, "right pool symbol mismatch" )

      double price = swap_market_itr->right_pool_quant.quantity.amount > 0 ?
         swap_market_itr->left_pool_quant.quantity.amount / swap_market_itr->right_pool_quant.quantity.amount : 0;

      CHECKC( price > 0, err::STATUS_ERROR, "invalid market actual price" )

      auto rand = get_random();
      // rand_num
      // auto chain_modeget_chain_mode()
      CHECKC( _gstate.fluctuation_ratio >= 0 && _gstate.fluctuation_ratio <= 1, err::PARAM_ERROR, "invalid fluctuation ratio" )
      double min_sideways_price = market_itr->target_price * (1 - _gstate.fluctuation_ratio );
      double max_sideways_price = market_itr->target_price * (1 + _gstate.fluctuation_ratio );
      constexpr double left_ratio_upward     = 0.9; // 90% chance to buy when price is low
      constexpr double left_ratio_downward   = 0.1; // 10% chance to sell when price is high
      constexpr double left_ratio_sideways   = 0.5; // 50% chance to hold when price is stable
      double left_ratio = 0;
      if (price < min_sideways_price)
         left_ratio = left_ratio_upward;
      else if (price > max_sideways_price)
         left_ratio = left_ratio_downward;
      else
         left_ratio = left_ratio_sideways;
      bool is_left_side = uint32_t(left_ratio * 10000'0000) % 10000'0000 < rand % 10000'0000;

      name bot = bot_group_itr->bots[rand % bot_group_itr->bots.size()];

      asset input_quantity;
      name input_contract;
      if ( is_left_side ) {
         auto amount_diff = _gstate.max_trade_amount.amount - _gstate.min_trade_amount.amount;
         input_quantity = asset( _gstate.min_trade_amount.amount + rand % amount_diff , _gstate.left_balance.symbol );
         CHECKC( input_quantity.amount > 0, err::PARAM_ERROR, "The calculated input quantity is invalid" )
         CHECKC( input_quantity <= _gstate.left_total_quant, err::PARAM_ERROR, "The calculated input quantity exceed the left total quantity" )
         ASSERT( _gstate.left_balance <= _gstate.left_total_quant )

         flon_token::accounts accounts( input_contract, bot.value );
         asset bot_balance_before = asset(0, _gstate.left_balance.symbol);
         auto bot_acct_itr = accounts.find( bot.value );
         if ( bot_acct_itr != accounts.end() ) {
            const auto& ac = accounts.get( _gstate.left_balance.symbol.code().raw() );
            bot_balance_before = bot_acct_itr->balance;
         }


         if ( bot_balance_before < input_quantity ) {
            CHECKC( _gstate.left_total_quant.amount / 4 >= input_quantity.amount, err::PARAM_ERROR,
               "Left pool total quantity is too small" )
            int64_t max_bot_balance_amount = min(_gstate.left_total_quant.amount / 4, _gstate.left_balance.amount);
            CHECKC( bot_balance_before.amount + max_bot_balance_amount >= input_quantity.amount, err::PARAM_ERROR,
               "Left pool balance insufficient" )
            // TODO: transfer out to bot， check in ontransfer
            asset transfer_quant = asset(max_bot_balance_amount, _gstate.left_balance.symbol);
            _gstate.left_balance       -= transfer_quant;
            // _gstate.left_total_quant   -= transfer_quant;
            // TODO: transfer_out memo: out:{nonce}
            TRANSFER_OUT(input_contract, bot, transfer_quant, "")
         }

         // input_quantity 将被交易成另一侧的资产，所以需要做：1. 从左侧总量中扣除；2. 在交易完成后，向右侧总量和右侧余额增加交易收到的右侧资产
         _gstate.left_total_quant -= input_quantity;
         asset min_received = asset((double)input_quantity.amount * price * (1 - _gstate.max_slippage), _gstate.right_balance.symbol);

         // swap
         std::string swap_memo = "swap:" + min_received.to_string() + ":" + _gstate.trade_pair_name.to_string();
         TRANSFER(input_contract, bot, _gstate.dex_contract, input_quantity, swap_memo)

         // TODO: after swap: add received_asset to _gstate.right_total_quant and _gstate.right_balance

      } else { // right_side
      }

      // do swap
   }


   void tokenx_mm::on_transfer(const name& from, const name& to, const asset& quant, const string& memo) {
      if (from == get_self() || to != get_self()) return;

      // CHECKC( from != to, err::ACCOUNT_INVALID, "cannot transfer to self" );
      CHECKC( quant.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )

      if (quant.symbol == _gstate.left_balance.symbol) {
         _gstate.left_balance += quant;
         _gstate.left_total_quant += quant;
      } else if (quant.symbol == _gstate.right_balance.symbol) {
         _gstate.right_balance += quant;
         _gstate.right_total_quant += quant;
      }
   }

   // void tokenx_mm::_check_bot( const name& bot_account ) {

   // }

   // void tokenx_mm::_process_plan_investment( const asset& quant ) {
   //    if( quant.symbol == USDT ) {
   //       _gstate.invested_usdt_balance.amount   += quant.amount;
   //       _gstate.usdt_balance.amount            += quant.amount;

   //    } else {
   //       CHECKC( _gstate.token_symbol == quant.symbol, err::PARAM_ERROR, "quant symobl invalid" )

   //       _gstate.invested_token_balance.amount  += quant.amount;
   //       _gstate.token_balance.amount           += quant.amount;
   //    }
   // }

   // void tokenx_mm::_process_trade_settlement( const asset& quant ) {
   //    if( quant.symbol == USDT ) {
   //        _gstate.usdt_balance.amount           += quant.amount;

   //    } else {
   //       CHECKC( _gstate.token_symbol == quant.symbol, err::PARAM_ERROR, "quant symobl invalid" )

   //       _gstate.token_balance.amount           += quant.amount;
   //    }
   // }

   // //generate a random boolean with even chance (50% probability)
   // bool tokenx_mm::_even_odds_buy() {
   //    // Use transaction ID and current time for pseudo-randomness
   //    auto tapos = eosio::tapos_block_prefix();
   //    auto timestamp = current_time_point().sec_since_epoch();

   //    // Combine tapos and timestamp for a seed
   //    uint64_t seed = tapos ^ timestamp; //bitwise XOR

   //    // Generate a pseudo-random number and check if even/odd for 50% probability
   //    return( (seed % 2) == 0 );
   // }
   // double tokenx_mm::_get_token_price() {
   //    return 0; //FIXME
   // }

   // void tokenx_mm::_process_buy() {
   //    double price = _get_token_price();
   //    auto target_price = price * (1 + _gstate.fluctuation_ratio );
   //    auto target_quant = asset( 100, USDT ); //FIXME
   //    TRANSFER( _gstate.usdt_contract, _gstate.dex_contract, target_quant, "" )
   // }

   // void tokenx_mm::_process_sell() {

   // }
}
