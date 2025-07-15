# コンパイラとオプション
CC      := gcc
CFLAGS  := -Wall -O2

# ビルド対象
TARGETS := p4l_server p4l demo
# インストール先
PREFIX  := /usr/local
BINDIR  := $(PREFIX)/bin

# デフォルトビルド
all: $(TARGETS)

# 個別ビルドルール
p4l_server: p4l_server.c common.h
	$(CC) $(CFLAGS) -o $@ p4l_server.c

p4l: p4l_client.c common.h
	$(CC) $(CFLAGS) -o $@ p4l_client.c

demo: demo_app.c
	$(CC) $(CFLAGS) -o $@ demo_app.c

# クリーン
clean:
	rm -f $(TARGETS) *.o

# インストール
install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 p4l_server $(DESTDIR)$(BINDIR)/
	install -m 755 p4l $(DESTDIR)$(BINDIR)/
	install -m 755 demo $(DESTDIR)$(BINDIR)/

# アンインストール
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/p4l_server
	rm -f $(DESTDIR)$(BINDIR)/p4l
	rm -f $(DESTDIR)$(BINDIR)/demo

.PHONY: all clean install uninstall
