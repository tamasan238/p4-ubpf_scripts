git clone https://github.com/wolfssl/wolfssl
cd wolfssl
git checkout v5.8.2-stable
./autogen.sh
./configure --enable-intelasm --enable-sp-asm
make -j$(nproc)
make test
sudo make install
sudo ldconfig
./wolfcrypt/benchmark/benchmark