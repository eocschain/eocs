# EOCS - The first cross-chain eos side chain

Welcome to the EOCS source code repository! As the parallel chain of EOS main chain, EOCS Chain uses EOSIO software as the underlying infrastructure of blockchain, and still maintains the same BFT-DPOS consensus mechanism and blockchain data structure. Therefore, we call it a cross-chain between the EOS main chain and the isomorphic chain. EOCS Chain will realize the transmission, parsing and processing of isomorphic cross-chain protocol packets by writing EOSIO system plug-ins and smart contracts while maintaining the independence of EOS main chain.

The isomorphic chain between the EOCS Chain parallel chain and the EOS main chain involves the following components: 

Isomorphic Inter-Chain Protocol (ICP) isomorphic cross-chain contract, deployed simultaneously on the parallel chain and main chain, supports parsing of cross-chain protocol packets, verification and storage of certificates, and EOS native currency (EOS) ), EOCS Chain original currency (EOC), EOS token cross-chain asset transfer isomorphic cross-chain channel, through logic to ensure the stability and security of channel establishment. Replay, securely and quickly transfer cross-chain protocol packets between the parallel chain and the main chain


**If you have previously installed EOSIO, please run the `eosio_uninstall` script (it is in the directory where you cloned EOSIO) before downloading and using the binary releases.**

#### Mac OS X Brew Install
```sh
$ brew tap eosio/eosio
$ brew install eosio
```
#### Mac OS X Brew Uninstall
```sh
$ brew remove eosio
```
#### Ubuntu 18.04 Debian Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.5.1/eosio_1.5.1-1-ubuntu-18.04_amd64.deb
$ sudo apt install ./eosio_1.5.1-1-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Debian Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.5.1/eosio_1.5.1-1-ubuntu-16.04_amd64.deb
$ sudo apt install ./eosio_1.5.1-1-ubuntu-16.04_amd64.deb
```
#### Debian Package Uninstall
```sh
$ sudo apt remove eosio
```
#### Centos RPM Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.5.1/eosio-1.5.1-1.el7.x86_64.rpm
$ sudo yum install ./eosio-1.5.1-1.el7.x86_64.rpm
```
#### Centos RPM Package Uninstall
```sh
$ sudo yum remove eosio.cdt
```
#### Fedora RPM Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.5.1/eosio-1.5.1-1.fc27.x86_64.rpm
$ sudo yum install ./eosio-1.5.1-1.fc27.x86_64.rpm
```
#### Fedora RPM Package Uninstall
```sh
$ sudo yum remove eosio.cdt
```

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
1. [Website](http://eocs.io)
1. [Developer Portal](https://developers.eos.io)
1. [StackExchange for Q&A](https://eosio.stackexchange.com/)
1. [Community Telegram Group](https://t.me/eocschain)
1. [Icp Document](https://github.com/eocschain/eocs/blob/master/ICP.md)


<a name="gettingstarted"></a>
## Getting Started
Instructions detailing the process of getting the software, building it, running a simple test network that produces blocks, account creation and uploading a sample contract to the blockchain can be found in [Getting Started](https://developers.eos.io/eosio-nodeos/docs/overview-1) on the [EOSIO Developer Portal](https://developers.eos.io).
