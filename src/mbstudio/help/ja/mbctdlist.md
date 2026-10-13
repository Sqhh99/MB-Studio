# mbctdlist — スワスデータ中の CTD レコードを一覧出力

`mbctdlist` はスワスデータファイルに含まれる CTD（電気伝導度・水温・深度）レコードを、時刻や航法の値とともに一覧出力します。ROV や AUV の測深プラットフォームの中には、CTD データをソナーのデータストリームに記録するものがあります。水質の確認や音速情報の導出のために、このデータを取り出すときに使います。

## 書式

    mbctdlist
        --binary {-A}
        --decimate=decimate {-Ddecimate}
        --delimiter=delimiter {-Gdelimeter}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-format=output_format {-Ooutput_format}
        --segment=segment {-Zsegment}
        --verbose {-V}

## 説明

`mbctdlist` は見つかった CTD レコードごとに、`-O` で選んだ値を含む 1 行を出力します。電気伝導度 (S/m)、水温 (°C)、塩分、水中音速のほか、最大 8 個の補助センサーの生の電圧が利用できます。時刻、位置、船首方位、速力、ソナー深度、高度は測深レコードから取得され、各 CTD レコードの時刻に内挿されます。ファイルに測深レコードがない場合、CTD レコードは出力されません。

既定ではタブ区切りのテキストを出力します。`-G` で別の区切り文字、`-A` で倍精度のバイナリ出力、`-D` で間引きを指定できます。出力が得られるのは CTD レコードを格納するデータフォーマットだけです（当初はフォーマット 88、Reson 7k のみ）。

`-O` で使える文字: `T` 時刻 yyyy/mm/dd/hh/mm/ss、`t` 時刻 yyyy mm dd hh mm ss、`J` と `j` 年と通日による時刻文字列、`M` Unix 時刻（10 進秒）、`m` 最初のレコードからの秒数、`U` Unix 時刻（整数秒）、`u` 最初のレコードからの整数秒、`V` レコード間隔 (s)、`X` 経度、`x` 経度（度 + 10 進分）、`Y` 緯度、`y` 緯度（度 + 10 進分）、`H` 船首方位、`h` 対地針路、`L` 航走距離 (km)、`l` 航走距離 (m)、`C` 電気伝導度、`c` 水温、`S` 塩分、`s` 音速 (m/s)、`P` ポテンシャル水温、`1`～`8` センサー電圧。接頭文字 `^` を付けると: `^C` ソナー高度 (m)、`^c` ソナー深度 (m)、`^S` 速力 (km/h)、`^s` 対地速力 (km/h)。接頭文字 `-`（符号反転）と `/`（逆数）は次の値を修飾します。既定値は `TXYHCc^cSs`（時刻、経度、緯度、船首方位、電気伝導度、水温、ソナー深度、塩分、音速）です。

## 主なオプション

- `-Ifile` (`--input=file`) — 入力スワスファイルまたはデータリスト。既定値: datalist.mb-1。
- `-Fformat` (`--format=format`) — MBIO フォーマット ID。データリストは -1。既定値: ファイル名の接尾辞から推定。
- `-Ooutput_format` (`--output-format=output_format`) — 出力列を選ぶ文字列（上記および「注意」を参照）。既定値: `TXYHCc^cSs`。
- `--decimate=decimate` — decimate 個ごとに 1 レコードだけ出力します。既定値: 1。
- `-Gdelimiter` (`--delimiter=delimiter`) — テキスト出力の列区切り文字。既定値: タブ。
- `-A` (`--binary`) — テキストではなく倍精度のバイナリ値を出力します。
- `-Zsegment` (`--segment=segment`) — 入力ファイルごとの出力の前に segment を含む行を書きます（GMT のマルチセグメントファイルには `>`）。テキスト出力のみ。
- `-Llonflip` (`--longitude-domain=lonflip`) — 経度の範囲: -1 = -360～0、0 = -180～180、1 = 0～360。既定値: 0。
- `-V` (`--verbose`), `-H` (`--help`) — 詳細メッセージ表示、使い方を表示して終了。

## 使用例

Reson 7k ファイルの CTD レコードを既定の列で出力します。

    mbctdlist -I 20080904_231809p.mb88 > ctd.txt

データリスト内の全ファイルの CTD レコードを、10 レコードごとに 1 つだけ、カンマ区切りで出力します。

    mbctdlist -I datalist.mb-1 --decimate=10 -G, > ctd.csv

既定の列を独自の 10 文字に置き換えます（Unix 時刻、経度、緯度、ソナー深度、水温、塩分、音速、電気伝導度、船首方位。`^` は cmd.exe で特別な意味を持つため引数を引用符で囲みます）。

    mbctdlist -I 20080904_231809p.mb88 "-OMXY^ccSsCH" > ctd_custom.txt

## 注意

- このバージョンでは、独自の `-O` 文字列は常に 10 個のフィールドコードとして扱われます。指定した文字は既定の `TXYHCc^cSs`（`^c` は 2 文字と数えます）の先頭部分を置き換え、11 文字目以降は無視されます。すべての列を指定するには、ちょうど 10 文字を指定してください。
- 間引きには長い形式 `--decimate=N` を使ってください。このバージョンでは短い形式 `-D` は値を正しく受け取りません。
- 出力は画面に表示されます。保存するには `> file.txt` でリダイレクトしてください。cmd.exe では `^` や `>` を含む引数を二重引用符で囲んでください。
- CTD レコードを含まないファイルからは何も出力されません。

## 関連項目

`mblist`, `mbnavlist`, `mbsvplist`, `mbinfo`, `mbdatalist`, `mblevitus`。完全なリファレンスは MB-System のマニュアルページ mbctdlist(1)（英語）です。
