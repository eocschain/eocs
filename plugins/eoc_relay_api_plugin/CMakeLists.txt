file( GLOB HEADERS "*.hpp"  "include/eosio/eoc_relay_plugin/*.hpp" "include/eosio/icp_relay_plugin/*.hpp")
add_library( eoc_relay_api_plugin eoc_relay_api_plugin.cpp ${HEADERS} )

target_link_libraries( eoc_relay_api_plugin http_plugin chain_plugin appbase eoc_relay_plugin icp_relay_plugin)
target_include_directories( eoc_relay_api_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" )
