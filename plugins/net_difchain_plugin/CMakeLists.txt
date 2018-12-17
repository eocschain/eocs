file(GLOB HEADERS "include/eosio/net_difchain_plugin/*.hpp" "include/eosio/http_plugin/*.hpp" "include/eosio/net_plugin/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../../programs/cleos/*.hpp")
add_library( net_difchain_plugin
             net_difchain_plugin.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/../../programs/cleos/httpc.cpp
             ${HEADERS} )

target_link_libraries( net_difchain_plugin chain_plugin producer_plugin appbase fc net_plugin http_plugin)
target_include_directories( net_difchain_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include  "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include" "${CMAKE_CURRENT_SOURCE_DIR}/../../programs/cleos")
