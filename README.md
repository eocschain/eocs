# EOCS - The first cross-chain eos side chain

Welcome to the EOCS source code repository! As the parallel chain of EOS main chain, EOCS Chain uses EOSIO software as the underlying infrastructure of blockchain, and still maintains the same BFT-DPOS consensus mechanism and blockchain data structure. Therefore, we call it a cross-chain between the EOS main chain and the isomorphic chain. EOCS Chain will realize the transmission, parsing and processing of isomorphic cross-chain protocol packets by writing EOSIO system plug-ins and smart contracts while maintaining the independence of EOS main chain.

The isomorphic chain between the EOCS Chain parallel chain and the EOS main chain involves the following components: 

Isomorphic Inter-Chain Protocol (ICP) isomorphic cross-chain contract, deployed simultaneously on the parallel chain and main chain, supports parsing of cross-chain protocol packets, verification and storage of certificates, and EOS native currency (EOS) ), EOCS Chain original currency (EOC), EOS token cross-chain asset transfer isomorphic cross-chain channel, through logic to ensure the stability and security of channel establishment. Replay, securely and quickly transfer cross-chain protocol packets between the parallel chain and the main chain
