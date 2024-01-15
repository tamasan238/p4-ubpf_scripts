cat << "EOL" >> ~/.bashrc
export P4SAMPLE=~/p4c/testdata/p4_16_samples
export UBPFBIN=~/ubpf/build/bin
export UBPFRUNTIME=~/p4c/backends/ubpf/runtime
export P4FILENAME=$P4SAMPLE/action_call_ubpf

# P4 -> C
alias p4-c='p4c-ubpf $P4FILENAME.p4 -o $P4FILENAME.c'
alias p4-c_='p4c-ubpf $P4FILENAME\_ext.p4 -o $P4FILENAME.c --emit-externs'

# C -> Assembly code (for check only)
alias c-asm='clang -O2 -S -target bpf -c $P4FILENAME.c -o $P4FILENAME.s -I $UBPFRUNTIME'
alias c-asm_='clang -O2 -S -target bpf -c $P4FILENAME.c -o $P4FILENAME.s -I $UBPFRUNTIME -include $P4FILENAME\_ext.c'

# C -> uBPF Bytecode
alias c-ubpf='clang -O2 -target bpf -c $P4FILENAME.c -o $P4FILENAME.o -I $UBPFRUNTIME'
alias c-ubpf_='clang -O2 -target bpf -c $P4FILENAME.c -o $P4FILENAME.o -I $UBPFRUNTIME -include $P4FILENAME\_ext.c'

# Run
alias run-ubpf='$UBPFBIN/ubpf_test $P4FILENAME.o'

# Concat
alias p4u='p4-c && c-asm && c-ubpf && echo $P4FILENAME && run-ubpf'
alias p4u_='p4-c_ && c-asm_ && c-ubpf_ && echo $P4FILENAME\_ext.p4 && echo "* with _ext.c" && run-ubpf'
EOL
source ~/.bashrc
