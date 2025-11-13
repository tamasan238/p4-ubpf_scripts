cat << 'EOF' >> ~root/.bashrc

# === P4 to uBPF 環境設定 ===
export P4SAMPLE=~iwai/p4c/testdata/p4_16_samples
export UBPFBIN=~iwai/ubpf/build/bin
export UBPFRUNTIME=~iwai/p4c/backends/ubpf/runtime
export P4FILENAME=\$P4SAMPLE/ubpf

# P4 -> C
alias p4-c='p4c-ubpf \$P4FILENAME.p4 -o \$P4FILENAME.c --emit-externs'

# C -> uBPF Bytecode
alias c-ubpf='sudo clang -O2 -target bpf -c \$P4FILENAME\_ext.c -o \$P4FILENAME.o -I \$UBPFRUNTIME -include \$P4FILENAME\_ext.h -include \$P4FILENAME.c'

# Run
alias run-ubpf='\$UBPFBIN/ubpf_test \$P4FILENAME.o'

# Concat
alias p4u='p4-c && c-ubpf && run-ubpf'
EOF
