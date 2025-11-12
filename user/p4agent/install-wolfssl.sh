gh repo clone wolfssl/wolfssl
cd wolfssl
git checkout v5.8.2-stable
./autogen.sh
./configure
make -j$(nproc)
make test
sudo make install