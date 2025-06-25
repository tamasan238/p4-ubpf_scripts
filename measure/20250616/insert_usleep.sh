#!/bin/bash

# 引数確認
if [ "$#" -ne 3 ]; then
    echo "使い方: $0 <ファイルパス> <行番号> <インデント（スペース数）>"
    exit 1
fi

FILE=$1
LINE=$2
INDENT=$3

# インデント作成（指定されたスペース数）
INDENTATION=$(printf "%*s" "$INDENT" "")

# 対象ディレクトリ一覧
DIRS=(1 5 10 50 100 500)

# 相対パスのベース部分を取得（たとえば 'ovs/lib/dpif-netdev.c'）
BASENAME=$(echo "$FILE" | cut -d/ -f2-)

# 各ディレクトリで処理を実行
for DIR in "${DIRS[@]}"; do
    TARGET="${DIR}/${BASENAME}"

    if [ -f "$TARGET" ]; then
        echo "修正中: $TARGET"
        sed -i "${LINE}i\\
${INDENTATION}usleep(${DIR});
" "$TARGET"
    else
        echo "スキップ: $TARGET は存在しません"
    fi
done

