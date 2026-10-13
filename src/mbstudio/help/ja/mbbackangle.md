# mbbackangle — 振幅／サイドスキャンのグレージング角補正テーブル作成

`mbbackangle` は、ビーム振幅とサイドスキャンの値が海底へのグレージング角によってどのように変化するかを計算し、その関数を補正テーブルとして書き出します。`mbprocess` はこのテーブルを使って後方散乱の強い角度依存性を平坦化し、`mbmosaic` で作成するモザイクがスワスの幾何形状ではなく海底の反射特性を表すようにします。

## 書式

    mbbackangle
        --altitude=altitude {-Zaltitude}
        --angle-bins=nangles/angle_max {-Nnangles/angle_max}
        --beam-pattern-mode=mode[/beamwidth/depression] {-Bmode[/beamwidth/depression]}
        --data-kind=kind {-Akind}
        --dump-output {-D}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --output-grid=gridmode/angle/min/max/n_columns/n_rows {-Ggridmode/angle/min/max/n_columns/n_rows}
        --pings-average=pings {-Ppings}
        --reference-angle=refangle {-Rrefangle}
        --skip-existing {-S}
        --slope-correction {-Q}
        --symmetric-correction {-C}
        --topography-grid=topogridfile {-Ttopogridfile}
        --verbose {-V}

## 説明

ビーム振幅やサイドスキャンを含むスワスファイル（1 ファイルまたはデータリスト内の全ファイル）を読み込み、値をグレージング角ごとのビンに分けて平均します。`-angle_max` から `+angle_max` までのスワスを `nangles` 個のビンに分割し、直下領域の外側（鉛直から 15 度を超える範囲）では隣接する 3 つのビンを平均します。各テーブルは一定数のピング（既定 50）の平均で、平均ピング時刻のタイムタグが付くため、通常 1 ファイルに多数のテーブルが作成されます。

既定では、各ビームまたはピクセルのアクロストラック距離と水深から、平坦な海底を仮定してグレージング角を計算します。`-Q` を指定するとスワス水深から求めたアクロストラック方向の傾斜を考慮し、`-T` を指定すると地形グリッドから傾斜を求めます。水深がない場所では、`-Z` で指定した高度に平坦な海底があると仮定します。`-Z` を指定しない場合、そのようなデータは使用されません。

テーブルは各スワスファイルと同じフォルダに、ファイル名に `.aga`（振幅）または `.sga`（サイドスキャン）を付けた名前で書き込まれます。スワスファイルの `mbprocess` パラメータファイルが作成または更新され、これらのテーブルによる振幅・サイドスキャン補正が有効になります（`AMPCORRFILE`、`SSCORRFILE` などのパラメータ）。`mbprocess` 実行時には、テーブルが時刻とグレージング角で補間されます。dB 単位のデータでは補正値を差し引いてから基準角（`-R`）でのテーブル値を加え戻し、線形データでは補正値で割ってから基準値を掛けます。

`-G` を指定すると、スワスファイルごとに、グレージング角に対する値の正規化ヒストグラムを GMT グリッドとして書き出します（スワスファイル名に `_aga.grd` または `_sga.grd` を付けた名前）。テーブルの確認に役立ちます。

## 主なオプション

- `-I<file>` / `--input=<file>` — スワスファイルまたはデータリスト。既定値: `datalist.mb-1`。
- `-A<kind>` / `--data-kind=<kind>` — ビーム振幅（`1`）またはサイドスキャン（`2`）を処理します。両方を処理するには 2 回指定します。既定値: 両方。
- `-N<nangles>/<angle_max>` / `--angle-bins=...` — 角度ビンの数と最大角度（度）。中央のビンが 0 度になるよう奇数を指定します。既定値: `81/80.0`（2 度間隔）。
- `-P<pings>` / `--pings-average=<pings>` — 1 つのテーブルで平均するピング数。既定値: 50。
- `-Q` / `--slope-correction` — グレージング角の計算にスワス水深から求めたアクロストラック方向の傾斜を使用します。
- `-T<grid>` / `--topography-grid=<grid>` — スワス水深の代わりに、この地形グリッド（`mbgrid` で作成したものなど）から求めた傾斜を使用します。
- `-Z<altitude>` / `--altitude=<altitude>` — 水深のないデータに対して、平坦な海底を仮定する既定のソナー高度（メートル）。
- `-R<refangle>` / `--reference-angle=<refangle>` — `mbprocess` が補正を適用するときの基準角。既定値: 30 度。
- `-C` / `--symmetric-correction` — テーブルを鉛直軸に対して強制的に対称にします。通常はテーブルを非対称のままにし、後で `mbset`（`AMPCORRSYMMETRY`、`SSCORRSYMMETRY`）で対称性を設定するほうが適切です。
- `-B<mode>[/<beamwidth>/<depression>]` / `--beam-pattern-mode=...` — `0`: 観測された平均値による経験的テーブル（既定）、`1`: ガウス型ビームパターンと球面拡散を組み合わせたパラメトリックモデル。実効ビーム幅と俯角を度で指定します（既定値 50 と 20）。
- `-G<kind>/<angle>/<min>/<max>/<n_columns>/<n_rows>` / `--output-grid=...` — 振幅（`kind` 1）またはサイドスキャン（`kind` 2）のヒストグラムグリッドを、角度 `-angle` から `+angle`、値 `min` から `max` の範囲で書き出します。両方を出力するには 2 回指定します。
- `-S` / `--skip-existing` — 補正テーブルが既に存在し最新であるスワスファイルをスキップします。
- `-D` / `--dump-output` — テーブルを `.aga`／`.sga` ファイルではなく画面（標準出力）に出力します。
- `-F<format>` / `--format=<format>` — MBIO フォーマット ID。通常はファイル名から判別されます。
- `-V` / `--verbose` — 進行状況と書き込んだテーブル数を表示します。

## 使用例

データリストのすべてのファイルについて、2 度間隔のビンで 50 ピングずつ平均したサイドスキャンと振幅のテーブルを作成します。

    mbbackangle -Idatalist.mb-1 -N81/80.0 -P50 -V

サイドスキャンのテーブルのみを作成し、海底の傾斜を水深グリッドから求め、処理済みのファイルはスキップします。

    mbbackangle -Idatalist.mb-1 -A2 -Tsurvey_bathy.grd -S -V

1 つのファイルについて、テーブル確認用のヒストグラムグリッドも書き出します。

    mbbackangle -I0003_20020425_022926.mb57 -G1/80/60/81/121 -G2/80/60/81/121 -V

補正を適用します。

    mbprocess -Idatalist.mb-1

## 注意

- 振幅やサイドスキャンを含まないフォーマットは、メッセージを表示してスキップされます。
- `-G` を指定すると、プロット作成のために `mbm_grdplot` スクリプトの実行も試みます。Windows では MB-System のスクリプトは利用できないため、この手順は失敗します（コマンドプロンプトがコマンドを認識できないと表示します）が、グリッド自体は書き込まれ、GMT でプロットできます。
- `-G` では `min` を省略できます（`kind/angle/max/n_columns/n_rows`）。その場合 `min` は 0 になります。
- 生のスワスファイル（`$PROCESSED` 指定のないデータリスト）に対して実行してください。テーブルは実際に読み込んだファイルのパラメータファイルに設定され、`mbprocess` は生ファイルのパラメータファイルを使用します。

## 関連項目

`mbprocess`, `mbset`, `mbfilter`, `mbmosaic`, `mbgrid`, `mbpreprocess`。完全なリファレンスは MB-System のマニュアルページ mbbackangle(1)（英語）です。
