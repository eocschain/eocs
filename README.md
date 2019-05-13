# EOCS - The first cross-chain eos side chain

Welcome to the EOCS source code repository! As the first cross-chain parallel chain of EOS main chain, EOCS Chain uses EOSIO software as the underlying infrastructure of blockchain, and still maintains the same BFT-DPOS consensus mechanism and blockchain data structure. Therefore, we call it a cross-chain between the EOS main chain and the isomorphic chain. EOCS Chain will realize the transmission, parsing and processing of isomorphic cross-chain protocol packets by writing EOSIO system plug-ins and smart contracts while maintaining the independence of EOS main chain.

The isomorphic chain between the EOCS Chain parallel chain and the EOS main chain involves the following components: 
Isomorphic Inter-Chain Protocol (ICP) isomorphic cross-chain contract, deployed simultaneously on the parallel chain and main chain, supports parsing of cross-chain protocol packets, verification and storage of certificates, and EOS native currency (EOS) ), EOCS Chain original currency (EOC), EOS token cross-chain asset transfer isomorphic cross-chain channel, through logic to ensure the stability and security of channel establishment. Replay, securely and quickly transfer cross-chain protocol packets between the parallel chain and the main chain


**If you have previously installed EOCS, please run the `eosio_uninstall` script (it is in the directory where you cloned EOSIO) before downloading and using the binary releases.**

#### EOCSIO Install
```sh
$ git clone https://github.com/eocschain/eocs.git --recursive
$ or
$ git clone https://github.com/eocschain/eocs.git
$ cd eocs
$ git submodule update --init --recursive
$ ./scripts/eosio_build.sh
```

The EOCS cross-chain components are as follows:

#### Isomorphic cross-chain protocol
```sh
$ Inter-chain protocols are meant to be able to express state transitions in decentralized inter-chain interoperability. Interoperability is the only point to be taken into consideration in building isomorphic inter-chain protocols, which are symmetric and two-way protocols. Based on the starting point of avoiding changes to the underlying EOSIO softwar e, we will implement inter-chain contracts that are deployed to both isomorphic chains. Therefore, the isomorphic inter-chain protocol is designed to contain data packets of state data and block certificates, and the Replay performs chain-to-chain packet relay, that is, the interface that calls the inter-chain contract.
```

#### Isomorphic cross-chain contract
```sh
$ We will deploy two identical contracts on the EOS main chain and EOCS. The contract account name is also eocseosioibc and provides an inter-chain operation interface for handling two-way transaction information.The ICP protocol is analyzed in the cross-chain contract, and the cross-chain transaction data is stored in the cross-chain contract ram to ensure the safety, openness, transparency and traceability of the cross-chain transaction. 
```
#### Cross-chain channel 
```sh
$ The isomorphic inter-chain contract can realize the contract call interface and perform inter-chain verification through the isomorphic inter-chain protocol. We need to introduce the concept of inter-chain channel as the connection channel between the inter-chain isomorphic contracts and pass the logic. Prove to ensure the stability and safety of the channel establishment. 
```
#### Replay
```sh
$ Although the EOSIO blockchain is designed to support inter-chain friendliness and the development of isomorphic inter-chain contracts on the EOSIO blockchain can express and record inter-chain intents themselves, EOSIO does not support contracts that can initiate inter-chain communication proactively and be externally called through the design interface. We will implement the EOSIO software plug-in to achieve the Replay of the isomorphic inter-chain contract, which can be deployed in the EOCS Chain and EOS main chain all nodes, and the relay is responsible for the EOCS Chain and real-time synchronization of bidirectional ICP packets linking to EOS main chain.
```

#### The documentation of EOCS Cross-chain ICP  
[The ICP Document](https://github.com/eocschain/eocs/blob/master/ICP.md)


## Supported Operating Systems
EOSIO currently supports the following operating systems:  
1. Amazon 2017.09 and higher
2. Centos 7
3. Fedora 25 and higher (Fedora 27 recommended)
4. Mint 18
5. Ubuntu 16.04 (Ubuntu 16.10 recommended)
6. Ubuntu 18.04
7. MacOS Darwin 10.12 and higher (MacOS 10.13.x recommended)

## Resources
1. [Website](https://www.eocs.io)
1. [Developer Portal](https://developers.eos.io)
1. [StackExchange for Q&A](https://eosio.stackexchange.com/)
1. [Community Telegram Group](https://t.me/eocschain)
1. [Icp Document](https://github.com/eocschain/eocs/blob/master/ICP.md)


<a name="gettingstarted"></a>
## Getting Started
Instructions detailing the process of getting the software, building it, running a simple test network that produces blocks, account creation and uploading a sample contract to the blockchain can be found in [Getting Started](https://developers.eos.io/eosio-nodeos/docs/overview-1) on the [EOSIO Developer Portal](https://developers.eos.io).
