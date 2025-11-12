git clone https:/github.com/wolfssl/wolfssl.git
cd wolfssl
git checkout v5.8.2-stable
./autogen.sh
./configure
make -j$(nproc)
make test
sudo make install