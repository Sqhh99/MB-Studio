# mbextractsegy — スワスファイルから地層探査データを SEGY に抽出

`mbextractsegy` は、サブボトムプロファイラ、センタービーム反射、地震反射の
レコードを含むスワスソナーデータファイルを読み込み、そのトレースを SEGY
ファイルに書き出します。データ取得後、Reson 7k や Edgetech Jstar のファイルから
地層探査データを取り出し、`mbsegyinfo` で確認、`mbsegylist` で一覧表示、
`mbsegygrid` で断面グリッド化するために使用します。

## 書式

    mbextractsegy
        --begin-time=yr/mo/da/hr/mn/sc/us {-Byr/mo/da/hr/mn/sc/us}
        --check-route-bearing {-M}
        --end-time=yr/mo/da/hr/mn/sc/us {-Eyr/mo/da/hr/mn/sc/us}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --output=segyfile {-Osegyfile}
        --plot-max=plotmax {-Zplotmax}
        --plot-scale=xscale/yscale/maxwidth {-Jxscale/yscale/maxwidth}
        --range-threshold=rangethreshold {-Urangethreshold}
        --route=routefile {-Rroutefile}
        --sample-format=sampleformat {-Ssampleformat}
        --start-line=startline/lineroot {-Lstartline/lineroot}
        --time-list=timelistfile {-Qtimelistfile}
        --time-shift=timeshift {-Ttimeshift}
        --verbose {-V}

## 説明

入力は 1 つのスワスファイルまたはデータリストです（`-I`）。地層探査トレースを
抽出できるのは、そのようなレコードを格納するフォーマットに限られます。例えば
Reson 7k（フォーマット 88）や Edgetech Jstar（フォーマット 132、133）です。
ソナー深度、海底深度、位置、方位、ロール、ピッチが SEGY のトレースヘッダに
書き込まれます。SEGY ファイルは地震探査処理パッケージ SIOSEIS の規約に従い、
サンプルは 32 ビット IEEE 浮動小数点です。

出力の構成は次の 3 通りです。

- `-O`、`-R`、`-Q` のいずれも指定しない場合、入力スワスファイルごとに SEGY
  ファイルが作成されます。ファイル名は `-L` で指定するラインルートと連番
  （既定はルート `sbp`、開始番号 1）から付けられます。例：`sbp_0001_env.segy`、
  `sbp_0002_env.segy`、…
- `-O` を指定すると、すべての入力ファイルのトレースが指定したルート名の
  1 つの出力にまとめられます。例：`-O survey` で `survey_env.segy`。
- ルートファイル（`-R`、`mbgrdviz` のルートファイル）またはウェイポイント
  時刻リスト（`-Q`、`mbroutetime` で作成）を指定すると、計画測線ごとに
  ファイルが分割され、`<lineroot>_<number>` という名前になります。

名前の末尾は `-S` で選ぶサンプル形式によって変わります。エンベロープ出力
（既定）では `_env` が付き、トレース出力では何も付きません。解析信号出力では
`_cor`（実部）、`_hil`（虚部）、`_env`（エンベロープ）の 3 ファイルが
作成されます。拡張子は `.segy` で、`-Osgy` を指定すると `.sgy` になります。

書き出した各 SEGY ファイルに対して `mbsegyinfo -O` が実行され、SEGY ファイル名に
`.sinf` を付けた統計ファイルが作成されます。また、断面グリッド用の `mbsegygrid`
コマンドを含む簡易プロットスクリプト `..._section.cmd` も作成されます
（「注意」を参照）。

## 主なオプション

- `-I<file>` — 入力スワスファイルまたはデータリスト。既定：`datalist.mb-1`。
- `-F<format>` — 入力の MB-System フォーマット番号。負の値は入力がデータ
  リストであることを示します。既定：ファイル名の拡張子から推定。
- `-O<root>` — すべての出力を、この名前ルートの 1 組の SEGY ファイルに
  書き出します。特別な値 `-Osegy` と `-Osgy` は拡張子だけを選択します。
- `-S<sampleformat>` — 出力サンプル形式：`1` = 格納形式のまま（変換なし）、
  `2` = 単一トレース（解析信号の場合は実部）、`3` = エンベロープ（既定）、
  `4` = 解析信号（3 ファイル）。
- `-R<routefile>` — `mbgrdviz` ルートファイルのウェイポイントを使って出力を
  測線ごとに分割します。
- `-Q<timelistfile>` — 各ウェイポイント到達時刻のリスト（`mbroutetime` で
  作成）を使って出力を測線ごとに分割します。
- `-L<startline>/<lineroot>` — 出力ファイルの開始測線番号とファイル名ルート。
  既定：`1/sbp`。
- `-M` — ルートモードで旋回中のトレースを除外します。方位が次の測線方位から
  15 度以内になった時点で出力を開始します。
- `-U<rangethreshold>` — ウェイポイント到達とみなす距離（m）。既定：25。
- `-T<timeshift>` — トレースのタイムスタンプにこの秒数を加えます。既定：0。
- `-B<yr/mo/da/hr/mn/sc>` / `-E<yr/mo/da/hr/mn/sc>` — 抽出するデータの
  時間範囲の開始と終了。
- `-J<xscale>/<yscale>/<maxwidth>` — プロットスクリプト内の断面図の縮尺
  （ショットあたりインチ、秒あたりインチ、最大幅インチ）。
  既定：`0.01/50.0/30.0`。
- `-Z<plotmax>` — 断面図で黒（最大濃度）に対応するトレース値。既定：50。
- `-V` — 詳細表示。書き出したトレース 10 本ごとに情報を表示します。

## 使用例

Reson 7k ファイル 1 本の地層探査データを抽出します（`sbp_0001_env.segy` と
`sbp_0001_env.segy.sinf` が作成されます）。

    mbextractsegy -I 20040722_152111.s7k

データリストの全ファイルを 1 つの SEGY ファイル `survey_sbp_env.segy` に
抽出します。

    mbextractsegy -I datalist.mb-1 -O survey_sbp

データを計画測線 `line_0001_env.segy`、`line_0002_env.segy`、… に分割し、
旋回部分を除外します。

    mbextractsegy -I datalist.mb-1 -R survey_route.rte -L1/line -M

エンベロープではなく生のトレースを書き出します。

    mbextractsegy -I 20040722_152111.s7k -S2

## 注意

- 出力ファイルはカレントディレクトリに書き出されます。
- `-O` には拡張子を付けずに名前ルートを指定してください。`_env.segy` などは
  プログラムが自動的に付けます。
- `_section.cmd` プロットスクリプトは `mbm_grdplot` を呼び出す Unix シェル
  スクリプトで、Windows では利用できません。中の `mbsegygrid` コマンド行を
  参考にグリッドを作成し、`gmt` でプロットしてください。`chmod` が認識され
  ないというメッセージは無視して構いません。

## 関連項目

`mbsegyinfo`、`mbsegylist`、`mbsegygrid`、`mbsegypsd`、`mbroutetime`、
`mb7k2jstar`、`mbdatalist`。完全なリファレンスは MB-System のマニュアル
ページ mbextractsegy(1)（英語）です。
