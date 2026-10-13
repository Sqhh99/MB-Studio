# mbfilter — サイドスキャン／振幅の平滑化・ハイパス・コントラストフィルタ

`mbfilter` は、サイドスキャンやビーム振幅データに簡単な画像処理フィルタを適用します。スペックルを低減するローパス平滑化、微細な構造を強調するハイパスフィルタ、コントラスト強調が利用できます。通常は、`mbmosaic` で後方散乱モザイクを作成する前に、処理済み（およびグレージング角補正済み）のファイルに対して実行します。

## 書式

    mbfilter
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --buffer-size=buffersize {-Nbuffersize}
        --contrast-filter=mode/xdim/ldim/iteration {-Cmode/xdim/ldim/iteration}
        --data-kind=kind {-Akind}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --hipass-filter=mode/xdim/ldim/iteration/offset {-Dmode/xdim/ldim/iteration/offset}
        --input=infile {-Iinfile}
        --smooth-filter=mode/xdim/ldim/iteration/threshold_lo/threshold_hi {-Smode/xdim/ldim/iteration/threshold_lo/threshold_hi}
        --threshold=threshold_lo/threshold_hi {-Tthreshold_lo/threshold_hi}
        --verbose {-V}

## 説明

スワスデータは、x 軸をピクセル（またはビーム）番号、y 軸をピング番号とする画像として扱われます。各フィルタは `xdim` × `ldim`（アクロストラック × アロングトラック）のウィンドウで、この画像に畳み込まれます。欠測値やフラグ付きの値は無視されます。利用できるフィルタは次のとおりです。

- `-S1` ボックスカー平均、`-S2` ガウス平均、`-S3` ボックスカーメディアン、`-S4` 逆勾配（ローパス、平滑化）
- `-D1` ボックスカー平均、`-D2` ガウス平均、`-D3` ボックスカーメディアンの差し引き（ハイパス）
- `-C1` エッジ検出、`-C2` 勾配強度の差し引き（コントラスト強調）

フィルタは最大 10 個まで指定でき、コマンドラインに書いた順に、それぞれ指定した反復回数で適用されます。スペックル低減には 3x3 や 5x5 のウィンドウを反復適用するのが一般的で、ハイパスフィルタには 3x10 のような細長いウィンドウがよく使われます。逆勾配フィルタはエッジより平坦な領域を強く平滑化するため、地形の特徴をより鮮明に保ちます。メディアンフィルタは低／高比率しきい値でスパイク除去のみに限定できます。元の値とメディアンの比が `threshold_lo` 未満または `threshold_hi` を超える場合にのみ値が置き換えられるため、不良ピングによる縞の抑制に役立ちます。

ハイパスフィルタはデータのローパス版を差し引き、結果が正の値になるようオフセットを加えます。コントラストフィルタはノイズを増幅するため、一般に平滑化の後でないと効果がありません。エッジ検出には 5x5 以上のウィンドウが必要です。

スワスファイル自体は変更されません。フィルタ処理した値は各入力ファイルと同じフォルダの補助ファイルに書き込まれます。ファイル名はサイドスキャンでは `.ffs`、振幅では `.ffa` を付けたものです。フィルタ処理済みデータを使うには、プロットやモザイク作成プログラムのデータ種別に `F` を付けます。たとえば `mbmosaic -A4F`（フィルタ済みサイドスキャン）、`-A3F`（フィルタ済み振幅）、または `gmt mbswath -Z5F`／`-Z4F` です。

## 主なオプション

- `-I<file>` / `--input=<file>` — スワスファイルまたはデータリスト。既定値: `datalist.mb-1`。
- `-A<kind>` / `--data-kind=<kind>` — ビーム振幅（`1`）またはサイドスキャン（`2`）をフィルタ処理します。既定値: 2。
- `-S<mode>/<xdim>/<ldim>[/<iteration>[/<threshold_lo>/<threshold_hi>]]` / `--smooth-filter=...` — ローパスフィルタ。mode 1 ボックスカー平均、2 ガウス平均、3 ボックスカーメディアン、4 逆勾配。しきい値はメディアンフィルタにのみ有効です。既定値: 反復 1 回。
- `-D<mode>/<xdim>/<ldim>[/<iteration>[/<offset>]]` / `--hipass-filter=...` — ハイパスフィルタ。mode 1 ボックスカー平均、2 ガウス平均、3 ボックスカーメディアンの差し引き。`offset` は結果を正に保つために加える値で、データの値の範囲に合わせて選びます。既定値: 反復 1 回、オフセット 1000。
- `-C<mode>/<xdim>/<ldim>[/<iteration>]` / `--contrast-filter=...` — コントラスト強調。mode 1 エッジ検出、2 勾配強度の差し引き。既定値: 反復 1 回。
- `-T<threshold_lo>/<threshold_hi>` / `--threshold=...` — このオプションより後に指定したメディアン平滑化フィルタの比率しきい値。
- `-B<yr/mo/da/hr/mn/sc>`, `-E<yr/mo/da/hr/mn/sc>` / `--begin-time`, `--end-time` — フィルタ処理するデータの時間範囲。
- `-R<west/east/south/north>` / `--bounds=...` — フィルタ処理するデータの地理的範囲。
- `-N<buffersize>` / `--buffer-size=<buffersize>` — バッファに保持するレコード数（10〜5000）。既定値: 5000。
- `-F<format>` / `--format=<format>` — MBIO フォーマット ID。通常はファイル名から判別されます。
- `-V` / `--verbose` — フィルタ設定と進行状況を表示します。

## 使用例

処理済みファイルのサイドスキャンに 3x3 ボックスカー平均フィルタを 1 回適用します。

    mbfilter -Idatalistp.mb-1 -S1/3/3/1

3x3 の逆勾配フィルタを 5 回適用します。

    mbfilter -Idatalistp.mb-1 -S4/3/3/5

ハイパスフィルタで微細な構造を強調し、その後平滑化してスペックルを低減します。

    mbfilter -Idatalistp.mb-1 -D1/3/10/1/10000 -S4/3/3/5

7x7 のメディアンフィルタで平滑化し、その後 5x5 のエッジ検出を適用します。

    mbfilter -Idatalistp.mb-1 -S3/7/7/1 -C1/5/5/1

## 注意

- 実行するたびに `.ffs` や `.ffa` ファイルは上書きされます。前回のフィルタ結果は累積されないため、フィルタの組み合わせは 1 回のコマンドですべて指定してください。
- フィルタには少なくとも `mode/xdim/ldim` が必要です。値が足りない場合、そのフィルタは無視されます。
- `-T` はコマンドライン上でそれより後にある `-S` フィルタにのみ適用されます。代わりに `-S3` の 5 番目と 6 番目の値としてしきい値を指定することもできます。
- 処理済みファイルのデータリスト（`datalistp.mb-1`）は `mbdatalist -Idatalist.mb-1 -Z` で作成できます。

## 関連項目

`mbbackangle`, `mbprocess`, `mbmosaic`, `mbdatalist`, `gmt mbswath`。完全なリファレンスは MB-System のマニュアルページ mbfilter(1)（英語）です。
