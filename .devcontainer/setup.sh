#!/usr/bin/bash

echo export DISPLAY="127.0.0.1:10.0" >> ~/.bashrc
echo PS1="\"\[\033[1;35m\]\u\[\033[0m\]@\[\033[1;36m\]\h\[\033[0m\] \[\033[34m\]\w\[\033[0m\]\n└> \[\033[1;32m\]$\[\033[0m\] \"" >> ~/.bashrc
echo alias ls="\"eza --icons -lah --sort=Name --group-directories-first --git\"" >> ~/.bashrc

# XDG_RUNTIME_DIR
cd /run/user
sudo mkdir dev
sudo chown dev dev
chmod 0700 dev

echo export XDG_RUNTIME_DIR=/run/user/dev >> ~/.bashrc