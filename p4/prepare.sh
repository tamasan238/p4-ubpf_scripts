cat << "EOL" >> ~/.bashrc
export P4SAMPLE=~/p4c/testdata/p4_16_samples
export UBPFBIN=~/ubpf/build/bin
export UBPFRUNTIME=~/p4c/backends/ubpf/runtime
export P4FILENAME=$P4SAMPLE/action_call_ubpf

# P4 -> C
alias p4-c='p4c-ubpf $P4FILENAME.p4 -o $P4FILENAME.c'

# C -> Assembly code (for check only)
alias p4-asm='clang -O2 -S -target bpf -c $P4FILENAME.c -o $P4FILENAME.s -I $UBPFRUNTIME'

# C -> uBPF Bytecode
alias c-ubpf='clang -O2 -target bpf -c $P4FILENAME.c -o $P4FILENAME.o -I $UBPFRUNTIME'

# Compile
alias p4compile='p4-c && c-ubpf && echo "please exec run-ubpf"'

# Run
alias run-ubpf='$UBPFBIN/ubpf_test $P4FILENAME.o'

EOL
source ~/.bashrc
