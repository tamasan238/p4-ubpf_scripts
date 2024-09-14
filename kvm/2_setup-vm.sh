#!/bin/bash

# root権限で実行されているか確認
if [ "$(id -u)" -ne 0 ]; then
  echo "このスクリプトはroot権限で実行する必要があります。"
  exit 1
fi

# hostnameを設定
read -p "新しく設定するホスト名を入力してください: " hostname
hostnamectl set-hostname ${hostname}

cat << "EOL" >> /root/.bashrc
alias c='clear'
alias e='exit'
EOL

cat << "EOL" >> ~iwai/.bashrc
alias c='clear'
alias e='exit'
EOL

## iwaiをNOPASSWDでsudoersに追加
# iwaiユーザーが存在するか確認
if id "iwai" &>/dev/null; then
    echo "User 'iwai' exists."
else
    echo "User 'iwai' does not exist. Please create the user first."
    exit 1
fi

# /etc/sudoers.d/iwai ファイルにNOPASSWD設定を追加
echo "iwai ALL=(ALL) NOPASSWD:ALL" | sudo tee /etc/sudoers.d/iwai > /dev/null

# 設定が正しいか確認
if sudo visudo -cf /etc/sudoers.d/iwai; then
    echo "iwai has been successfully added to sudoers with NOPASSWD."
else
    echo "There was an error in the sudoers configuration."
    exit 1
fi

## 必要に応じてプロセスを自動再起動

CONFIG_FILE="/etc/needrestart/needrestart.conf"

# ファイルが存在するか確認
if [ ! -f "$CONFIG_FILE" ]; then
    echo "$CONFIG_FILE does not exist. Please ensure needrestart is installed."
    exit 1
fi

# 設定ファイル内の値を変更
sed -i 's/^$nrconf{restart}.*$/$nrconf{restart} = '\''a'\'';/g' $CONFIG_FILE
sed -i 's/^$nrconf{kernelhints}.*$/$nrconf{kernelhints} = 0;/g' $CONFIG_FILE

# 変更内容を確認
echo "Updated $CONFIG_FILE:"
grep -E 'nrconf{restart}|nrconf{kernelhints}' $CONFIG_FILE

echo "Configuration updated. 'Daemons using outdated libraries' warnings will be suppressed, and automatic restarts are enabled."


## その他

git config --global user.email "m1117@aol.jp"
git config --global user.name "masaki"

timedatectl set-timezone Asia/Tokyo

apt update
apt upgrade -y

apt install -y avahi-daemon
systemctl enable avahi-daemon
systemctl start avahi-daemon

reboot
