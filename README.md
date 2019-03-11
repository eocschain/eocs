# EOCS - The first cross-chain eos side chain

Welcome to the EOCS source code repository! As the parallel chain of EOS main chain, EOCS Chain uses EOSIO software as the underlying infrastructure of blockchain, and still maintains the same BFT-DPOS consensus mechanism and blockchain data structure. Therefore, we call it a cross-chain between the EOS main chain and the isomorphic chain. EOCS Chain will realize the transmission, parsing and processing of isomorphic cross-chain protocol packets by writing EOSIO system plug-ins and smart contracts while maintaining the independence of EOS main chain.

The isomorphic chain between the EOCS Chain parallel chain and the EOS main chain involves the following components: 

<<<<<<< HEAD
Isomorphic Inter-Chain Protocol (ICP) isomorphic cross-chain contract, deployed simultaneously on the parallel chain and main chain, supports parsing of cross-chain protocol packets, verification and storage of certificates, and EOS native currency (EOS) ), EOCS Chain original currency (EOC), EOS token cross-chain asset transfer isomorphic cross-chain channel, through logic to ensure the stability and security of channel establishment. Replay, securely and quickly transfer cross-chain protocol packets between the parallel chain and the main chain
=======
Welcome to the EOSIO source code repository! This software enables businesses to rapidly build and deploy high-performance and high-security blockchain-based applications.

Some of the groundbreaking features of EOSIO include:

1. Free Rate Limited Transactions
1. Low Latency Block confirmation (0.5 seconds)
1. Low-overhead Byzantine Fault Tolerant Finality
1. Designed for optional high-overhead, low-latency BFT finality
1. Smart contract platform powered by WebAssembly
1. Designed for Sparse Header Light Client Validation
1. Scheduled Recurring Transactions
1. Time Delay Security
1. Hierarchical Role Based Permissions
1. Support for Biometric Hardware Secured Keys (e.g. Apple Secure Enclave)
1. Designed for Parallel Execution of Context Free Validation Logic
1. Designed for Inter Blockchain Communication

EOSIO is released under the open source MIT license and is offered “AS IS” without warranty of any kind, express or implied. Any security provided by the EOSIO software depends in part on how it is used, configured, and deployed. EOSIO is built upon many third-party libraries such as WABT (Apache License) and WAVM (BSD 3-clause) which are also provided “AS IS” without warranty of any kind. Without limiting the generality of the foregoing, Block.one makes no representation or guarantee that EOSIO or any third-party libraries will perform as intended or will be free of errors, bugs or faulty code. Both may fail in large or small ways that could completely or partially limit functionality or compromise computer systems. If you use or implement EOSIO, you do so at your own risk. In no event will Block.one be liable to any party for any damages whatsoever, even if it had been advised of the possibility of damage.  

Block.one is neither launching nor operating any initial public blockchains based upon the EOSIO software. This release refers only to version 1.0 of our open source software. We caution those who wish to use blockchains built on EOSIO to carefully vet the companies and organizations launching blockchains based on EOSIO before disclosing any private keys to their derivative software.

There is no public testnet running currently.
>>>>>>> eosiobranch

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
<<<<<<< HEAD
$ wget https://github.com/eosio/eos/releases/download/v1.6.2/eosio_1.6.2-1-ubuntu-18.04_amd64.deb
$ sudo apt install ./eosio_1.6.2-1-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Debian Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.6.2/eosio_1.6.2-1-ubuntu-16.04_amd64.deb
$ sudo apt install ./eosio_1.6.2-1-ubuntu-16.04_amd64.deb
=======
$ wget https://github.com/eosio/eos/releases/download/v1.6.0/eosio_1.7.0-rc2-ubuntu-18.04_amd64.deb
$ sudo apt install ./eosio_1.7.0-rc2-ubuntu-18.04_amd64.deb
```
#### Ubuntu 16.04 Debian Package Install
```sh
$ wget https://github.com/eosio/eos/releases/download/v1.6.0/eosio_1.7.0-rc2-ubuntu-16.04_amd64.deb
$ sudo apt install ./eosio_1.7.0-rc2-ubuntu-16.04_amd64.deb
>>>>>>> eosiobranch
```
#### Debian Package Uninstall
```sh
$ sudo apt remove eosio
```
#### Centos RPM Package Install
```sh
<<<<<<< HEAD
$ wget https://github.com/eosio/eos/releases/download/v1.6.2/eosio-1.6.2-1.el7.x86_64.rpm
$ sudo yum install ./eosio-1.6.2-1.el7.x86_64.rpm
=======
$ wget https://github.com/eosio/eos/releases/download/v1.6.0/eosio-1.7.0-rc2.el7.x86_64.rpm
$ sudo yum install ./eosio-1.7.0-rc2.el7.x86_64.rpm
>>>>>>> eosiobranch
```
#### Centos RPM Package Uninstall
```sh
$ sudo yum remove eosio.cdt
```
#### Fedora RPM Package Install
```sh
<<<<<<< HEAD
$ wget https://github.com/eosio/eos/releases/download/v1.6.2/eosio-1.6.2-1.fc27.x86_64.rpm
$ sudo yum install ./eosio-1.6.2-1.fc27.x86_64.rpm
=======
$ wget https://github.com/eosio/eos/releases/download/v1.6.0/eosio-1.7.0-rc2.fc27.x86_64.rpm
$ sudo yum install ./eosio-1.7.0-rc2.fc27.x86_64.rpm
>>>>>>> eosiobranch
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
1. [Website](https://www.eocs.io)
1. [Developer Portal](https://developers.eos.io)
1. [StackExchange for Q&A](https://eosio.stackexchange.com/)
1. [Community Telegram Group](https://t.me/eocschain)
1. [Icp Document](https://github.com/eocschain/eocs/blob/master/ICP.md)


<a name="gettingstarted"></a>
## Getting Started
Instructions detailing the process of getting the software, building it, running a simple test network that produces blocks, account creation and uploading a sample contract to the blockchain can be found in [Getting Started](https://developers.eos.io/eosio-nodeos/docs/overview-1) on the [EOSIO Developer Portal](https://developers.eos.io).
