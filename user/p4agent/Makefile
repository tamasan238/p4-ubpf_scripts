# コンパイラとオプション
CC      := gcc
CFLAGS  := -Wall -O2

# ビルド対象
TARGETS := p4agent
# インストール先
PREFIX  := /usr/local
BINDIR  := $(PREFIX)/bin

# デフォルトビルド
all: $(TARGETS)

# 個別ビルドルール
p4agent: p4agent.c
	$(CC) $(CFLAGS) -o $@ p4agent.c

# クリーン
clean:
	rm -f $(TARGETS) *.o

# インストール
install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 p4agent $(DESTDIR)$(BINDIR)/

# アンインストール
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/p4agent

.PHONY: all clean install uninstall
