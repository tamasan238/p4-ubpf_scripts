#!/bin/bash

ALIAS_COMMAND="alias build-ovs=\"sudo bash -c 'make clean ; ./boot.sh ; ./configure ; make -j\\\$(nproc) && make install'\""

echo "$ALIAS_COMMAND" >> /root/.bashrc
echo "$ALIAS_COMMAND" >> /home/iwai/.bashrc

chown root:root /root/.bashrc
chown iwai:iwai /home/iwai/.bashrc