#!/bin/bash

# Determina la directory dello script e la root della repo dinamicamente
SCRIPT_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
REPO_ROOT=$(realpath "$SCRIPT_DIR/../../../")

# Helper per creare la cartella build in automatico se non presente,
# eseguire cmake con le opzioni desiderate, e tornare alla cartella in cui si era quando viene chiamato
cmake_config() {
    local original_dir=$(pwd)
    cd "$REPO_ROOT"
    mkdir -p build
    cd build
    cmake -DEUDAQ_BUILD_ONLINE_ROOT_MONITOR=OFF -DEUDAQ_LIBRARY_BUILD_TTREE=ON -DUSER_HIDRA_BUILD=ON --fresh ..
    cd "$original_dir"
}

# Helper per buildare il codice senza dover tornare a mano in build.
# Si sposta nella cartella eudaq_hidra/build, esegue make -j 10, e ritorna nella cartella di esecuzione
build_hidra() {
    local original_dir=$(pwd)
    cd "$REPO_ROOT/build"
    make -j 10
    make install -j 10
    cd "$original_dir"
}

# Helper per pulire la cartella di build e relative altre cartelle (lib, etc.).
# Si sposta nella cartella eudaq_hidra, esegue make_clean, e ritorna nella cartella di esecuzione
cmake_clean() {
    local original_dir=$(pwd)
    cd "$REPO_ROOT"
    sh "$REPO_ROOT/make_clean.sh"
    cd "$original_dir"
}

# Crea alias per tornare velocemente alle cartelle
alias build_dir="cd $REPO_ROOT/build"
alias hidra_run="cd $REPO_ROOT/user/hidra/run"
alias hidra_dir="cd $REPO_ROOT/user/hidra"
