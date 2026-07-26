#include <tokenx.mm/tokenx.mm.hpp>
#include <flon/token.protocol.hpp>

#include <flon/utils.hpp>
#include <contract_version.hpp>
#include <tokenx.mm/tokenx.mm.old.hpp>

static constexpr eosio::name active_permission{"active"_n};

namespace flon {
   using namespace std;

   static const name LEFT_SIDE = "left"_n;
   static const name RIGHT_SIDE = "right"_n;
   static constexpr uint32_t SIDE_SEGMENT_SECONDS = 1800;
   static constexpr double EDGE_REVERSION_THRESHOLD = 0.92;

   // scope: buylowsellhi contract
   struct trade_market_t {
      name            trade_market_name;           // trading market name, PK
      bool            paused              = true;  // is this market paused
      double          target_price        = 0.0;   // target price
      asset           min_trade_amount;            // Minimum amount allowed in each trade, it must be left side
      asset           max_trade_amount;            // Maximum amount allowed in each trade, it must be left side
      string          memo;
      set<name>       updaters;
      double          max_slippage            = 0.1;   // Maximum allowed slippage (default: 10%)
      double          fluctuation_ratio       = 0.1;   // Price fluctuation ratio for sideways market (default: 10%)
      uint32_t        min_trade_seconds       = 10;    // Minimum interval between trades in seconds (default: 10)
      uint32_t        max_trade_seconds       = 30;    // Maximum interval between trades in seconds (default: 30)

      uint64_t primary_key()const { return trade_market_name.value; }

      typedef eosio::multi_index< "trademarkets"_n,  trade_market_t> idx_t;

      EOSLIB_SERIALIZE( trade_market_t, (trade_market_name)(paused)(target_price)(min_trade_amount)
                                        (max_trade_amount)(memo)(updaters)(max_slippage)(fluctuation_ratio)
                                        (min_trade_seconds)(max_trade_seconds) )
   };

   // scope: bots contract
   struct bot_group_t {
      name                        group_name;     //PK
      string                      desc;
      set<name>                   bots;

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

   static int64_t get_random_range(int64_t min, int64_t max, int64_t rand) {
      ASSERT(max >= min);
      ASSERT(rand >= 0);
      if (max == min)
         return max;
      return min + rand % (max - min + 1);
   }

   static double clamp_double(double value, double min_value, double max_value) {
      if (value < min_value) return min_value;
      if (value > max_value) return max_value;
      return value;
   }

   static int64_t get_small_biased_random_range(int64_t min, int64_t max, uint32_t rand) {
      ASSERT(max >= min);
      if (max == min) return max;

      int64_t span = max - min;
      int64_t first = rand % (span + 1);
      int64_t second = ((rand >> 16) ^ (rand * 1103515245u)) % (span + 1);
      return min + std::min(first, second);
   }

   static uint32_t mix32(uint32_t value) {
      value ^= value >> 16;
      value *= 0x7feb352du;
      value ^= value >> 15;
      value *= 0x846ca68bu;
      value ^= value >> 16;
      return value;
   }

   static uint32_t calc_trade_wait_seconds(uint32_t min_seconds, uint32_t max_seconds, uint32_t rand) {
      CHECK( min_seconds > 0, "min_trade_seconds must be greater than 0" )
      CHECK( max_seconds >= min_seconds, "max_trade_seconds can not be less than min_trade_seconds" )
      return get_random_range(min_seconds, max_seconds, (rand >> 8) ^ (rand * 2654435761u));
   }

   struct side_bias_t {
      bool     primary_left;
   };

   static uint32_t trade_pair_seed(const name& trade_pair_name) {
      return uint32_t(trade_pair_name.value) ^ uint32_t(trade_pair_name.value >> 32);
   }

   static double calc_normalized_price_offset(double left_price, double target_price, double fluctuation_ratio) {
      if (target_price <= 0 || fluctuation_ratio <= 0) {
         return 0.0;
      }

      double band_width = target_price * fluctuation_ratio;
      if (band_width <= 0) {
         return 0.0;
      }

      return clamp_double((left_price - target_price) / band_width, -1.0, 1.0);
   }

   static side_bias_t calc_sideways_side_bias(double left_price, double target_price, double fluctuation_ratio,
                                              const name& trade_pair_name, uint32_t now_seconds) {
      if (target_price <= 0 || fluctuation_ratio <= 0) {
         return side_bias_t{ true };
      }

      double band_width = target_price * fluctuation_ratio;
      if (band_width <= 0) {
         return side_bias_t{ true };
      }

      double normalized = calc_normalized_price_offset(left_price, target_price, fluctuation_ratio);

      if (normalized >= EDGE_REVERSION_THRESHOLD) {
         return side_bias_t{ true };
      }
      if (normalized <= -EDGE_REVERSION_THRESHOLD) {
         return side_bias_t{ false };
      }

      // Keep direction stable across several 5-minute candles so bid/ask fee spread does not draw clipped up/down bars.
      uint32_t segment = now_seconds / SIDE_SEGMENT_SECONDS;
      uint32_t segment_rand = mix32(trade_pair_seed(trade_pair_name) ^ (segment * 2246822519u));
      bool primary_left = (segment_rand & 1u) == 0;
      return side_bias_t{ primary_left };
   }

   static int64_t calc_depth_limited_input_amount(const asset& input_reserve, double fluctuation_ratio) {
      CHECK( input_reserve.amount > 0, "invalid dex input reserve" )

      // Limit a single bot trade to a small fraction of pool depth. Smaller configured bands imply smaller per-trade impact.
      double max_reserve_ratio = clamp_double(fluctuation_ratio * 0.05, 0.0002, 0.002);
      int64_t depth_limited_amount = (int64_t)((double)input_reserve.amount * max_reserve_ratio);
      return std::max<int64_t>(1, depth_limited_amount);
   }

   static int64_t calc_trade_left_amount(int64_t min_amount, int64_t max_amount, uint32_t rand,
                                         const name& trade_pair_name, uint32_t now_seconds, bool counter_primary_side) {
      int64_t amount = get_small_biased_random_range(min_amount, max_amount, rand);
      if (max_amount <= min_amount) return amount;

      int64_t span_amount = amount - min_amount;
      if (counter_primary_side) {
         return min_amount + span_amount * 35 / 100;
      }

      uint32_t segment = now_seconds / 900;
      uint32_t rhythm_rand = mix32(trade_pair_seed(trade_pair_name) ^ (segment * 3266489917u) ^ (rand >> 7));
      uint32_t rhythm_bps = 7000 + (rhythm_rand % 6001);
      int64_t adjusted_span = span_amount * rhythm_bps / 10000;
      return min(max_amount, min_amount + adjusted_span);
   }

   static uint32_t calc_left_inventory_value_bps(const bot_market_t& bot_market, double left_price) {
      int64_t left_boost = power10(bot_market.left_pool.total_quantity.symbol.precision());
      int64_t right_boost = power10(bot_market.right_pool.total_quantity.symbol.precision());
      double left_amount = (double)bot_market.left_pool.total_quantity.amount / (double)left_boost;
      double right_amount = (double)bot_market.right_pool.total_quantity.amount / (double)right_boost;
      double left_value_in_right = left_amount * left_price;
      double total_value_in_right = left_value_in_right + right_amount;
      if (total_value_in_right <= 0) return 5000;
      return (uint32_t)clamp_double(left_value_in_right * 10000.0 / total_value_in_right, 0.0, 10000.0);
   }

   static int64_t apply_inventory_amount_limit(int64_t amount, bool is_left_side, uint32_t left_inventory_bps) {
      uint32_t scale_bps = 10000;
      if (is_left_side && left_inventory_bps < 3500) {
         scale_bps = 3500;
      } else if (!is_left_side && left_inventory_bps > 6500) {
         scale_bps = 3500;
      } else if (is_left_side && left_inventory_bps < 4500) {
         scale_bps = 6500;
      } else if (!is_left_side && left_inventory_bps > 5500) {
         scale_bps = 6500;
      }
      return std::max<int64_t>(1, amount * scale_bps / 10000);
   }

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

   const name& tokenx_mm::require_admin_auth() const {
      if (_gstate.admin.value != 0 && has_auth(_gstate.admin)) {
         return _gstate.admin;
      } else if (has_auth(_self)) {
         return _self;
      } else {
         CHECKC(false, err::NO_AUTH, "miss self or admin authorization");
         __builtin_unreachable();
      }
   }

   void tokenx_mm::setadmin( const name& admin ) {
      require_admin_auth();
      _gstate.admin = admin;
   }


    void tokenx_mm::cfgbotmgr( const name& bot_mgr_contract ) {
      require_admin_auth();
      _gstate.bot_mgr_contract = bot_mgr_contract;
    }

    static double calc_price(const asset& left_pool_quantity, const asset& right_pool_quantity) {
      int64_t left_amount = left_pool_quantity.amount;
      int64_t left_boost = power10(left_pool_quantity.symbol.precision());

      int64_t right_amount = right_pool_quantity.amount;
      int64_t right_boost = power10(right_pool_quantity.symbol.precision());

      return ((double)right_amount * left_boost) / ((double) left_amount * right_boost);
    }

   static double calc_trade_out(double price, int64_t input_amount, const symbol& input_symbol, const symbol& output_symbol) {
      int64_t in_boost = power10(input_symbol.precision());
      int64_t out_boost = power10(output_symbol.precision());
      return (double)input_amount * price * out_boost / in_boost;
   }

   static int64_t get_input_pool_available(const dex_pool_side_t& input_pool, const name& bot) {
      const auto& input_contract = input_pool.balance.contract;
      const auto& input_symbol = input_pool.balance.quantity.symbol;

      asset bot_balance = flon_token::get_balance(input_contract, bot, input_symbol, false);
      asset available_quantity = input_pool.balance.quantity + bot_balance;
      return min(input_pool.total_quantity.amount, available_quantity.amount);
   }

   void tokenx_mm::setmarket( const name& trade_pair_name, const name& fund_account, const name& bot_group_name ) {
      auto admin = require_admin_auth();

      CHECKC( trade_pair_name.value != 0, err::PARAM_ERROR, "invalid trade pair name" )
      CHECKC( is_account(fund_account), err::PARAM_ERROR, "fund account not existing" )
      CHECKC( bot_group_name.value != 0, err::PARAM_ERROR, "invalid bot group name" )

      auto bot_markets = bot_market_t::idx_t( get_self(), get_self().value );
      auto bot_market_itr = bot_markets.find(trade_pair_name.value);
      if (bot_market_itr == bot_markets.end()) {
         auto swap_markets = swap_market_t::idx_t( _gstate.dex_contract, _gstate.dex_contract.value );
         auto swap_market_itr = swap_markets.find( trade_pair_name.value );
         CHECKC( swap_market_itr != swap_markets.end(), err::RECORD_NOT_FOUND, "swap market not existing: " + trade_pair_name.to_string() )

         const auto& left_contract = swap_market_itr->left_pool_quant.contract;
         const auto& left_symbol = swap_market_itr->left_pool_quant.quantity.symbol;
         const auto& right_contract = swap_market_itr->right_pool_quant.contract;
         const auto& right_symbol = swap_market_itr->right_pool_quant.quantity.symbol;

         bot_markets.emplace( admin, [&] (auto& row) {
            row.trade_pair_name              = trade_pair_name;
            row.fund_account                 = fund_account;
            row.bot_group_name               = bot_group_name;
            row.left_pool.balance            = extended_asset(asset(0, left_symbol), left_contract);
            row.left_pool.total_quantity     = asset(0, left_symbol);
            row.right_pool.balance           = extended_asset(asset(0, right_symbol), right_contract);
            row.right_pool.total_quantity    = asset(0, right_symbol);
            // other fields use default values
         } );
      } else {
         bot_markets.modify( bot_market_itr, admin, [&] (auto& row) {
            row.fund_account        = fund_account;
            row.bot_group_name      = bot_group_name;
         } );
      }
   }

   void tokenx_mm::trade( const name& bot, const name& trade_pair_name, const string& memo ) {
      require_auth(bot);
      bot_market_t::idx_t bot_markets( get_self(), get_self().value );
      auto bot_market_itr = bot_markets.find(trade_pair_name.value);
      CHECKC( bot_market_itr != bot_markets.end(), err::RECORD_NOT_FOUND, "bot market not existing: " + trade_pair_name.to_string() )


      // contract name: buylowsellhi = 4520798682350377696
      auto markets = trade_market_t::idx_t( name(4520798682350377696), 4520798682350377696 );
      auto market_itr = markets.find(bot_market_itr->trade_pair_name.value);
      CHECKC( market_itr != markets.end(), err::RECORD_NOT_FOUND, "market not existing: " + bot_market_itr->trade_pair_name.to_string() )
      CHECKC( market_itr->min_trade_amount.symbol == bot_market_itr->left_pool.balance.quantity.symbol,
         err::STATUS_ERROR, "left pool symbol mismatch with min_trade_amount.symbol" )

      CHECKC( !market_itr->paused, err::STATUS_ERROR, "market is paused: " + bot_market_itr->trade_pair_name.to_string() )

      auto bot_groups = bot_group_t::idx_t( _gstate.bot_mgr_contract, _gstate.bot_mgr_contract.value );
      auto bot_group_itr = bot_groups.find(bot_market_itr->bot_group_name.value);
      CHECKC( bot_group_itr != bot_groups.end(), err::RECORD_NOT_FOUND, "bot group not existing: " + bot_market_itr->bot_group_name.to_string() )
      CHECKC( bot_group_itr->bots.size() > 0, err::STATUS_ERROR, "no bot in bot group: " + bot_market_itr->bot_group_name.to_string() )
      CHECKC( bot_group_itr->bots.count(bot) > 0, err::RECORD_NOT_FOUND, "bot not existing in group: " + bot_market_itr->bot_group_name.to_string() )

      auto swap_markets = swap_market_t::idx_t( _gstate.dex_contract, _gstate.dex_contract.value );
      auto swap_market_itr = swap_markets.find( bot_market_itr->trade_pair_name.value );
      CHECKC( swap_market_itr != swap_markets.end(), err::RECORD_NOT_FOUND, "swap market not existing: " + bot_market_itr->trade_pair_name.to_string() )
      CHECKC( swap_market_itr->left_pool_quant.contract == bot_market_itr->left_pool.balance.contract,
         err::PARAM_ERROR, "left pool token contract mismatch" )
      CHECKC( swap_market_itr->left_pool_quant.quantity.symbol == bot_market_itr->left_pool.balance.quantity.symbol,
         err::PARAM_ERROR, "left pool symbol mismatch" )
      CHECKC( swap_market_itr->right_pool_quant.contract == bot_market_itr->right_pool.balance.contract,
         err::PARAM_ERROR, "right pool token contract mismatch" )
      CHECKC( swap_market_itr->right_pool_quant.quantity.symbol == bot_market_itr->right_pool.balance.quantity.symbol,
         err::PARAM_ERROR, "right pool symbol mismatch" )

      CHECKC( swap_market_itr->left_pool_quant.quantity.amount > 0, err::STATUS_ERROR, "invalid dex market left pool amount" )
      CHECKC( swap_market_itr->right_pool_quant.quantity.amount > 0, err::STATUS_ERROR, "invalid dex market right pool amount" )
      // int64_t left_pool_amount = swap_market_itr->left_pool_quant.quantity.amount;
      // int64_t right_pool_amount = swap_market_itr->right_pool_quant.quantity.amount;
      // double right_to_left_ratio = (double)right_pool_amount / left_pool_amount;
      double left_price = calc_price(swap_market_itr->left_pool_quant.quantity, swap_market_itr->right_pool_quant.quantity);
      CHECKC( left_price > 0, err::STATUS_ERROR, "invalid market actual price" )

      auto rand = get_random();

      auto schedules = schedule_t::idx_t( get_self(), get_self().value );
      auto schedule_itr = schedules.find(bot_market_itr->trade_pair_name.value);
      uint32_t now_seconds = current_time_point().sec_since_epoch();
      if (schedule_itr != schedules.end()) {
         uint32_t last_traded_seconds = schedule_itr->last_traded_at.sec_since_epoch();
         uint64_t next_trade_seconds = uint64_t(last_traded_seconds) + schedule_itr->random_interval_seconds;
         CHECKC( last_traded_seconds == 0 || uint64_t(now_seconds) >= next_trade_seconds,
            err::STATUS_ERROR, "trade interval not reached, next trade at: " + std::to_string(next_trade_seconds) )
      }

      // rand_num
      // auto chain_modeget_chain_mode()
      CHECKC( market_itr->fluctuation_ratio >= 0 && market_itr->fluctuation_ratio <= 1, err::PARAM_ERROR, "invalid fluctuation ratio" )
      double min_sideways_price = market_itr->target_price * (1 - market_itr->fluctuation_ratio );
      double max_sideways_price = market_itr->target_price * (1 + market_itr->fluctuation_ratio );
      bool is_left_side = false;
      bool inside_sideways_band = left_price >= min_sideways_price && left_price <= max_sideways_price;
      double normalized_offset = calc_normalized_price_offset(left_price, market_itr->target_price, market_itr->fluctuation_ratio);
      uint32_t left_inventory_bps = calc_left_inventory_value_bps(*bot_market_itr, left_price);
      side_bias_t side_bias = calc_sideways_side_bias(left_price, market_itr->target_price, market_itr->fluctuation_ratio,
                                                      bot_market_itr->trade_pair_name, now_seconds);
      if (left_price < min_sideways_price) {
         // Price is below the target band: buy the left asset with right-side funds to lift price.
         is_left_side = false;
      } else if (left_price > max_sideways_price) {
         // Price is above the target band: sell the left asset to push price back down.
         is_left_side = true;
      } else if (normalized_offset >= EDGE_REVERSION_THRESHOLD) {
         // Near the upper edge, correct price before it prints a clipped high.
         is_left_side = true;
      } else if (normalized_offset <= -EDGE_REVERSION_THRESHOLD) {
         // Near the lower edge, correct price before it prints a clipped low.
         is_left_side = false;
      } else {
         // Inside the band, follow the segment direction instead of flipping every trade.
         // Opposite-side prints in the same candle expose swap fee spread as artificial high/low clipping.
         is_left_side = side_bias.primary_left;
         if (left_inventory_bps < 1000 && bot_market_itr->right_pool.total_quantity.amount > 0) {
            is_left_side = false;
         } else if (left_inventory_bps > 9000 && bot_market_itr->left_pool.total_quantity.amount > 0) {
            is_left_side = true;
         }
      }
      bool counter_primary_side = inside_sideways_band && (is_left_side != side_bias.primary_left);
      int64_t trading_left_amount = calc_trade_left_amount( market_itr->min_trade_amount.amount, market_itr->max_trade_amount.amount,
                                                            rand ^ 0x9e3779b9u, bot_market_itr->trade_pair_name,
                                                            now_seconds, counter_primary_side );
      trading_left_amount = apply_inventory_amount_limit(trading_left_amount, is_left_side, left_inventory_bps);

      const auto& side = is_left_side ? LEFT_SIDE : RIGHT_SIDE;

      bool trade_sent = false;
      bot_markets.modify( bot_market_itr, same_payer, [&] (auto& row) {
         if ( is_left_side ) {

            double min_price = left_price * (1 - market_itr->max_slippage);
            if (row.left_pool.balance.quantity > row.left_pool.total_quantity) {
               eosio::print("skip trade: ", side, " side fund snapshot invalid, refreshfund required\n");
               return;
            }
            int64_t max_input_amount = get_input_pool_available(row.left_pool, bot);
            int64_t depth_limited_amount = calc_depth_limited_input_amount(swap_market_itr->left_pool_quant.quantity, market_itr->fluctuation_ratio);
            int64_t input_amount = min(min(trading_left_amount, depth_limited_amount), max_input_amount);
            if (input_amount <= 0) {
               eosio::print("skip trade: ", side, " side total quantity is zero\n");
               return;
            }
            do_trade(side, row, row.left_pool, row.right_pool, min_price, input_amount, bot, bot_group_itr->bots.size());
            trade_sent = true;
         } else { // right_side
            double price = 1 / left_price;

            int64_t input_amount = calc_trade_out(left_price, trading_left_amount, row.left_pool.balance.quantity.symbol,
                        row.right_pool.balance.quantity.symbol);
            if (row.right_pool.balance.quantity > row.right_pool.total_quantity) {
               eosio::print("skip trade: ", side, " side fund snapshot invalid, refreshfund required\n");
               return;
            }
            int64_t max_input_amount = get_input_pool_available(row.right_pool, bot);
            int64_t depth_limited_amount = calc_depth_limited_input_amount(swap_market_itr->right_pool_quant.quantity, market_itr->fluctuation_ratio);
            input_amount = min(min(input_amount, depth_limited_amount), max_input_amount);
            if (input_amount <= 0) {
               eosio::print("skip trade: ", side, " side total quantity is zero\n");
               return;
            }
            double min_price = price * (1 - market_itr->max_slippage);
            // check(false, "side: " + side.to_string() + ", input_amount: " + std::to_string(input_amount) + ", trading_left_amount: " + std::to_string(trading_left_amount) +
            //    ", price: " + std::to_string(price));
            do_trade(side, row, row.right_pool, row.left_pool, min_price, input_amount, bot, bot_group_itr->bots.size());
            trade_sent = true;
         }
         row.last_traded_at = current_time_point();
      } );

      if (trade_sent) {
         uint32_t next_random_interval = calc_trade_wait_seconds(market_itr->min_trade_seconds, market_itr->max_trade_seconds, rand >> 1);
         auto schedule_itr_for_update = schedules.find(bot_market_itr->trade_pair_name.value);
         if (schedule_itr_for_update == schedules.end()) {
            schedules.emplace( get_self(), [&] (auto& row) {
               row.trade_pair_name = bot_market_itr->trade_pair_name;
               row.last_traded_at = current_time_point();
               row.random_interval_seconds = next_random_interval;
            } );
         } else {
            schedules.modify( schedule_itr_for_update, same_payer, [&] (auto& row) {
               row.last_traded_at = current_time_point();
               row.random_interval_seconds = next_random_interval;
            } );
         }
      }

   }


    void tokenx_mm::do_trade(const name& side, const bot_market_t& bot_market, dex_pool_side_t& input_pool, dex_pool_side_t& output_pool,
                             double min_price, int64_t input_amount, const name& bot, size_t bot_size) {

         const auto& input_contract = input_pool.balance.contract;
         const auto& input_symbol = input_pool.balance.quantity.symbol;
         auto& input_pool_balance = input_pool.balance.quantity;

         const auto& output_contract = output_pool.balance.contract;
         const auto& output_symbol = output_pool.balance.quantity.symbol;

         asset input_quantity = asset( input_amount, input_symbol );
         CHECKC( input_quantity.amount > 0, err::PARAM_ERROR, side.to_string() + " side calculated input quantity is invalid" )
         CHECKC( input_quantity <= input_pool.total_quantity, err::PARAM_ERROR,
            side.to_string() + " side calculated input quantity exceed the total quantity" +
            ", input_quantity: " + input_quantity.to_string() + ", total_quantity: " + input_pool.total_quantity.to_string())
         CHECKC( input_pool_balance <= input_pool.total_quantity, err::STATUS_ERROR,
            side.to_string() + " side fund snapshot invalid, refreshfund required" +
            ", balance: " + input_pool_balance.to_string() + ", total_quantity: " + input_pool.total_quantity.to_string())

         asset bot_input_balance_before = flon_token::get_balance( input_contract, bot, input_symbol, false );
         if ( bot_input_balance_before < input_quantity ) {
            int64_t min_transfer = input_quantity.amount - bot_input_balance_before.amount;
            CHECKC( input_pool_balance.amount >= min_transfer, err::PARAM_ERROR,
               side.to_string() + " side balance insufficient" )
            int64_t max_balance_per_bot = input_pool.total_quantity.amount * 0.5 / bot_size;
            int64_t transfer_amount = min(max(min_transfer, max_balance_per_bot), input_pool_balance.amount);
            max_balance_per_bot = min(max_balance_per_bot, input_pool_balance.amount);
            asset transfer_quant = asset(transfer_amount, input_symbol);
            input_pool_balance -= transfer_quant;
            TRANSFER(input_contract, bot_market.fund_account,  bot, transfer_quant, "")
         }

         asset bot_received_before = flon_token::get_balance( output_contract, bot, output_symbol, false );

         // input_quantity will be traded for the asset on the other side, so the following steps are required:
         //    1. Deduct it from the total amount on the current side;
         //    2. After the trade is completed, add the received asset to total amount on the another side.
         input_pool.total_quantity -= input_quantity;

         int64_t min_received_amount = calc_trade_out(min_price, input_quantity.amount, input_symbol, output_symbol);
         asset min_received = asset(min_received_amount, output_symbol);
         // check(false, "side: " + side.to_string() + ", min_received: " + min_received.to_string() + ", input_quantity: " + input_quantity.to_string() +
         //    ", price: " + std::to_string(price) + ", output_boost: " + std::to_string(output_boost) + ", input_boost: " + std::to_string(input_boost) +
         //    ", max_slippage: " + std::to_string(_gstate.max_slippage));
         // eosio::print("bot: ", bot, "\n");
         // eosio::print("side: ", side, "\n");
         // eosio::print("price: ", price, "\n");
         // eosio::print("input_quantity: ", input_quantity, "\n");
         // eosio::print("min_received_amount: ", min_received_amount, "\n");

         // swap
         std::string swap_memo = "swap:" + min_received.to_string() + ":" + bot_market.trade_pair_name.to_string();
         TRANSFER(input_contract, bot, _gstate.dex_contract, input_quantity, swap_memo)

         afterswap_action act{ get_self(), { {get_self(), "active"_n} } };
         act.send( bot, bot_market.trade_pair_name, side, input_quantity, bot_received_before );
    }

   void tokenx_mm::on_transfer(const name& from, const name& to, const asset& quant, const string& memo) {
      if (from == get_self() || to != get_self()) return;

      // CHECKC( from != to, err::ACCOUNT_INVALID, "cannot transfer to self" );
      CHECKC( quant.amount > 0, err::PARAM_ERROR, "non-positive quantity not allowed" )
      const auto& token_contract = get_first_receiver();

      // memo format: addfund:trade_pair_name
      auto memo_params = split(memo, ":");
      if (memo_params.size() < 1) {
         return;
      }
      if (memo_params[0] == "addfund") {
         CHECKC( memo_params.size() == 2, err::PARAM_ERROR, "invalid memo format" )
         name trade_pair_name = name(memo_params[1]);

         auto bot_markets = bot_market_t::idx_t( get_self(), get_self().value );
         auto bot_market_itr = bot_markets.find(trade_pair_name.value);
         CHECKC( bot_market_itr != bot_markets.end(), err::RECORD_NOT_FOUND, "bot market not existing: " + trade_pair_name.to_string() )
         bot_markets.modify( bot_market_itr, same_payer, [&] (auto& row) {
            if (token_contract == row.left_pool.balance.contract && quant.symbol == row.left_pool.balance.quantity.symbol) {
               row.left_pool.balance.quantity += quant;
               row.left_pool.total_quantity += quant;
            } else if (token_contract == row.right_pool.balance.contract && quant.symbol == row.right_pool.balance.quantity.symbol) {
               row.right_pool.balance.quantity += quant;
               row.right_pool.total_quantity += quant;
            } else {
               CHECKC( false, err::PARAM_ERROR, "token contract or symbol not match any side of the bot market" )
            }
         } );
         TRANSFER_OUT(token_contract, bot_market_itr->fund_account, quant, "to bot fund")
      }
   }

   void tokenx_mm::afterswap(const name& bot, const name& trade_pair_name, const name& side, const asset& input_quantity, const asset& bot_received_before) {
      require_auth(get_self());

      auto bot_markets = bot_market_t::idx_t( get_self(), get_self().value );
      auto bot_market_itr = bot_markets.find(trade_pair_name.value);
      CHECKC( bot_market_itr != bot_markets.end(), err::RECORD_NOT_FOUND, "bot market not existing: " + trade_pair_name.to_string() )


      if (side != LEFT_SIDE && side != RIGHT_SIDE) {
         CHECKC( false, err::PARAM_ERROR, "invalid side in afterswap: " + side.to_string() );
      }

      bot_markets.modify( bot_market_itr, same_payer, [&] (auto& row) {
         dex_pool_side_t* input_pool = nullptr;
         dex_pool_side_t* output_pool = nullptr;
         if (side == LEFT_SIDE) {
            input_pool = &row.left_pool;
            output_pool = &row.right_pool;
         } else if (side == RIGHT_SIDE) {
            input_pool = &row.right_pool;
            output_pool = &row.left_pool;
         } else {
            CHECKC( false, err::PARAM_ERROR, "unsupported input quantity symbol in afterswap" );
         }

         ASSERT(input_quantity.symbol == input_pool->balance.quantity.symbol);
         ASSERT(bot_received_before.symbol == output_pool->balance.quantity.symbol);

         asset bot_balance_after = flon_token::get_balance( output_pool->balance.contract, bot, output_pool->balance.quantity.symbol, true );
         CHECKC( bot_balance_after >= bot_received_before, err::STATUS_ERROR, side.to_string() + " side bot balance after swap is less than before" )
         asset actual_received = bot_balance_after - bot_received_before;
         // The asset is still in the bot account, so the received from the swap contract is only added to the total quantity.
         output_pool->total_quantity += actual_received;
      } );

   }

   void tokenx_mm::updatefund(const name& trade_pair_name, const asset& left_pool_balance, const asset& left_pool_total,
                     const asset& right_pool_balance, const asset& right_pool_total) {
      auto admin = require_admin_auth();

      auto bot_markets = bot_market_t::idx_t( get_self(), get_self().value );
      auto bot_market_itr = bot_markets.find(trade_pair_name.value);
      CHECKC( bot_market_itr != bot_markets.end(), err::RECORD_NOT_FOUND, "bot market not existing: " + trade_pair_name.to_string() )

      CHECKC( left_pool_balance.symbol == bot_market_itr->left_pool.balance.quantity.symbol, err::PARAM_ERROR, "left pool balance symbol mismatch" )
      CHECKC( left_pool_total.symbol == bot_market_itr->left_pool.balance.quantity.symbol, err::PARAM_ERROR, "left pool total symbol mismatch" )
      CHECKC( right_pool_balance.symbol == bot_market_itr->right_pool.balance.quantity.symbol, err::PARAM_ERROR, "right pool balance symbol mismatch" )
      CHECKC( right_pool_total.symbol == bot_market_itr->right_pool.balance.quantity.symbol, err::PARAM_ERROR, "right pool total symbol mismatch" )
      CHECKC( left_pool_balance <= left_pool_total, err::PARAM_ERROR, "left pool balance cannot exceed total" )
      CHECKC( right_pool_balance <= right_pool_total, err::PARAM_ERROR, "right pool balance cannot exceed total" )

      bot_markets.modify( bot_market_itr, admin, [&] (auto& row) {
         row.left_pool.balance.quantity = left_pool_balance;
         row.left_pool.total_quantity = left_pool_total;
         row.right_pool.balance.quantity = right_pool_balance;
         row.right_pool.total_quantity = right_pool_total;
      } );
      // TODO: check balance with actual token balance in contract
   }

   void tokenx_mm::refreshfund(const name& trade_pair_name) {
      auto admin = require_admin_auth();

      auto bot_markets = bot_market_t::idx_t( get_self(), get_self().value );
      auto bot_market_itr = bot_markets.find(trade_pair_name.value);
      CHECKC( bot_market_itr != bot_markets.end(), err::RECORD_NOT_FOUND, "bot market not existing: " + trade_pair_name.to_string() )

      auto bot_groups = bot_group_t::idx_t( _gstate.bot_mgr_contract, _gstate.bot_mgr_contract.value );
      auto bot_group_itr = bot_groups.find(bot_market_itr->bot_group_name.value);
      CHECKC( bot_group_itr != bot_groups.end(), err::RECORD_NOT_FOUND, "bot group not existing: " + bot_market_itr->bot_group_name.to_string() )

      const auto& left_symbol = bot_market_itr->left_pool.balance.quantity.symbol;
      const auto& right_symbol = bot_market_itr->right_pool.balance.quantity.symbol;
      const auto& left_contract = bot_market_itr->left_pool.balance.contract;
      const auto& right_contract = bot_market_itr->right_pool.balance.contract;
      asset left_balance = flon_token::get_balance( left_contract, bot_market_itr->fund_account, left_symbol, false );
      asset right_balance = flon_token::get_balance( right_contract, bot_market_itr->fund_account, right_symbol, false );

      asset left_total_quantity = left_balance;
      asset right_total_quantity = right_balance;

      for (const auto& bot : bot_group_itr->bots) {
         left_total_quantity += flon_token::get_balance( left_contract, bot, left_symbol, false );
         right_total_quantity += flon_token::get_balance( right_contract, bot, right_symbol, false );
      }
      bot_markets.modify( bot_market_itr, admin, [&] (auto& row) {
         row.left_pool.balance.quantity = left_balance;
         row.left_pool.total_quantity = left_total_quantity;
         row.right_pool.balance.quantity = right_balance;
         row.right_pool.total_quantity = right_total_quantity;
      } );

   }

   void tokenx_mm::withdrawfund(const name& trade_pair_name, const extended_asset& quantity) {
      auto admin = require_admin_auth();

      auto bot_markets = bot_market_t::idx_t( get_self(), get_self().value );
      auto bot_market_itr = bot_markets.find(trade_pair_name.value);
      CHECKC( bot_market_itr != bot_markets.end(), err::RECORD_NOT_FOUND, "bot market not existing: " + trade_pair_name.to_string() )
      bot_markets.modify( bot_market_itr, same_payer, [&] (auto& row) {
         if (quantity.contract == row.left_pool.balance.contract && quantity.quantity.symbol == row.left_pool.balance.quantity.symbol) {
            CHECKC( quantity.quantity <= row.left_pool.balance.quantity, err::PARAM_ERROR,
               "withdraw quantity exceed left pool balance" )
            ASSERT( row.left_pool.balance.quantity <= row.left_pool.total_quantity )
            row.left_pool.balance.quantity -= quantity.quantity;
            row.left_pool.total_quantity -= quantity.quantity;
         } else if (quantity.contract == row.right_pool.balance.contract && quantity.quantity.symbol == row.right_pool.balance.quantity.symbol) {
            CHECKC( quantity.quantity <= row.right_pool.balance.quantity, err::PARAM_ERROR,
               "withdraw quantity exceed right pool balance" )
            ASSERT( row.right_pool.balance.quantity <= row.right_pool.total_quantity )
            row.right_pool.balance.quantity -= quantity.quantity;
            row.right_pool.total_quantity -= quantity.quantity;
         } else {
            CHECKC( false, err::PARAM_ERROR, "token contract or symbol not match any side of the bot market" )
         }
      } );
   }

}// namespace flon
