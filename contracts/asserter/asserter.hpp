/**
 *  @file
<<<<<<< HEAD
 *  @copyright defined in eos/LICENSE.txt
=======
 *  @copyright defined in eos/LICENSE
>>>>>>> otherb
 */

#include <eosiolib/eosio.hpp>

namespace asserter {
   struct assertdef {
      int8_t      condition;
      std::string message;

      EOSLIB_SERIALIZE( assertdef, (condition)(message) )
   };
}
