#!/bin/bash

ALIAS_COMMAND="alias build-ubpf=\"sudo make --build build -v --target install'\""

echo "$ALIAS_COMMAND" >> /root/.bashrc
echo "$ALIAS_COMMAND" >> /home/iwai/.bashrc

chown root:root /root/.bashrc
chown iwai:iwai /home/iwai/.bashrc