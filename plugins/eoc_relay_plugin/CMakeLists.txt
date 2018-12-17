file(GLOB HEADERS "include/eosio/eoc_relay_plugin/*.hpp" "include/eosio/http_plugin/*.hpp" "include/eosio/producer_plugin/*.hpp" "include/eosio/net_plugin/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../../programs/cleos/*.hpp"  "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp" )
add_library( eoc_relay_plugin
             eoc_relay_plugin.cpp
             icp_relay.cpp  api.cpp  icp_sync_manager.cpp icp_connection.cpp transaction.cpp
	     ${CMAKE_CURRENT_SOURCE_DIR}/../../programs/cleos/httpc.cpp
             ${HEADERS} )

target_link_libraries( eoc_relay_plugin chain_plugin producer_plugin appbase fc net_plugin http_plugin )
target_include_directories( eoc_relay_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
 "${CMAKE_CURRENT_SOURCE_DIR}/" 
 "${CMAKE_CURRENT_SOURCE_DIR}/../chain_interface/include"  
 "${CMAKE_CURRENT_SOURCE_DIR}/../../libraries/appbase/include" 
 "${CMAKE_CURRENT_SOURCE_DIR}/../../programs/cleos")


