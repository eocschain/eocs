#! /bin/bash

OPT_LOCATION=$HOME/opt

binaries=(
   cleos
   eosio-abigen
   eosio-launcher
   eosio-s2wasm
   eosio-wast2wasm
   eosiocpp
   keosd
   nodeos
   eosio-applesdemo
)

if [ -d $OPT_LOCATION/lemon ]; then
   printf "Do you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\nThis requires sudo, please run ./eosio_uninstall.sh with sudo\n\n"
               exit -1
            fi

            pushd $HOME &> /dev/null
            pushd opt &> /dev/null
            rm -rf lemon
            # Handle cleanup of directories created from installation
            if [ "$1" == "--full" ]; then
               if [ -d ~/Library/Application\ Support/lemon ]; then rm -rf ~/Library/Application\ Support/lemon; fi # Mac OS
               if [ -d ~/.local/share/lemon ]; then rm -rf ~/.local/share/lemon; fi # Linux
            fi
            popd &> /dev/null
            pushd bin &> /dev/null
            for binary in ${binaries[@]}; do
               rm ${binary}
            done
            popd &> /dev/null
            pushd lib/cmake &> /dev/null
            rm -rf lemon
            popd &> /dev/null

            break;;
         [Nn]* )
            printf "Aborting uninstall\n\n"
            exit -1;;
      esac
   done
fi

if [ -d "/usr/local/lemon" ]; then
   printf "Do you wish to remove this install? (requires sudo)\n"
   select yn in "Yes" "No"; do
      case $yn in
         [Yy]* )
            if [ "$(id -u)" -ne 0 ]; then
               printf "\nThis requires sudo, please run ./eosio_uninstall.sh with sudo\n\n"
               exit -1
            fi

            pushd /usr/local &> /dev/null
            pushd opt &> /dev/null
            rm -rf lemon
            # Handle cleanup of directories created from installation
            if [ "$1" == "--full" ]; then
               if [ -d ~/Library/Application\ Support/lemon ]; then rm -rf ~/Library/Application\ Support/lemon; fi # Mac OS
               if [ -d ~/.local/share/lemon ]; then rm -rf ~/.local/share/lemon; fi # Linux
            fi
            popd &> /dev/null
            pushd bin &> /dev/null
            for binary in ${binaries[@]}; do
               rm ${binary}
            done
            popd &> /dev/null
            pushd lib/cmake &> /dev/null
            rm -rf lemon
            popd &> /dev/null

            break;;
         [Nn]* )
            printf "Aborting uninstall\n\n"
            exit -1;;
      esac
   done
fi
