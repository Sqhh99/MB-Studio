# mbsslayout — 時系列サイドスキャンを海底に展開（フォーマット 71）

`mbsslayout` は、時間軸の生サイドスキャンソナーデータ（左舷・右舷の時系列。例: Edgetech の `.jsf` ファイル）を、各サンプルを平坦海底または地形グリッドに投影することで、横断方向に等間隔のピクセルを持つスワスサイドスキャンに変換します。結果は MB-System のフォーマット 71 ファイルに書き出され、その後 `mbbackangle` で補正し、`mbprocess` で処理し、`mbmosaic` でモザイク化できます。

## 書式

    mbsslayout [
        --verbose
        --help
        --input=datalist
        --format=format
        --platform-file=FILE
        --platform-target-sensor=SENSORID
        --output-source=record_kind
        --line-time-list=filename
        --line-route=filename
        --line-position-list=filename
        --line-name1=name
        --line-name2=name
        --line-range-threshold=value
        --topo-grid-file=filename
        --altitude-altitude
        --altitude-bottompick
        --altitude-bottompick-threshold=threshold[/blank]
        --altitude-topo-grid
        --channel-swap
        --swath-width=value
        --gain=value
        --interpolation=value
        --nav-file=filename
        --nav-file-format=format_id
        --nav-async=record_kind
        --sensordepth-file=filename
        --sensordepth-file-format=format_id
        --sensordepth-async=record_kind
        --altitude-file=filename
        --altitude-file-format=format_id
        --altitude-async=record_kind
        --heading-file=filename
        --heading-file-format=format_id
        --heading-async=record_kind
        --attitude-file=filename
        --attitude-file-format=format_id
        --attitude-async=record_kind
        --soundspeed-constant=value
        --soundspeed-file=filename
        --soundspeed-file-format=format_id
        --soundspeed-async=record_kind
        --time-latency-file=filename
        --time-latency-constant=value
        --time-latency-apply-nav
        --time-latency-apply-sensordepth
        --time-latency-apply-altitude
        --time-latency-apply-heading
        --time-latency-apply-attitude
        --time-latency-apply-all-ancilliary
        --time-latency-apply-survey
        --time-latency-apply-all
        --filter=value
        --filter-apply-nav
        --filter-apply-sensordepth
        --filter-apply-altitude
        --filter-apply-heading
        --filter-apply-attitude
        --filter-apply-all-ancilliary
        ]

## 説明

各サイドスキャンサンプルは、往復走時、左右舷の別、音速、ソナーの位置と姿勢を用いて海底上に配置されます。サンプルはソナー直下を中心とする等間隔の横断方向配列にビン分けされ、1 つのピクセルに複数のサンプルが入る場合は平均されます。小さな空きは補間で埋めることができます。出力スワスのピクセル数は最大 4001 です。既定ではスワス幅はピングごとに変化し（W = 2.2 × sqrt(R² − A²)、R は最大斜距離、A は高度）、ピクセルサイズは W / 4000 です。`--swath-width` を指定すると一定の幅になります。

地形モデルがない場合、サイドスキャンはソナー高度の平坦海底上に展開されます。高度は既定ではデータとともに記録された高度を使います。`--altitude-bottompick` を指定すると、左舷・右舷の時系列で最初の海底反射を検出し、その平均を高度とします。`--topo-grid-file` を指定すると、各サンプルを GMT 地形グリッド（例: 同時または以前のマルチビームデータから `mbgrid` で作成したグリッド）に投影するため、起伏の大きい海底でも位置が正しくなります。

航法、センサー深度、高度、方位、姿勢、音速は、既定では入力ファイルの値を使います。それぞれ外部ファイル（`--nav-file` など）またはデータ中の別のレコード種別（`--nav-async` など）から置き換えることができ、時間遅延補正やガウス型の時間領域平滑化も適用できます。航法の基準点とサイドスキャンの間のオフセットは、プラットフォームファイル（`mbmakeplatform`）で記述できます。

既定では入力ファイル 1 つにつき出力ファイル 1 つが作成され、名前は入力名から拡張子を除いたものに `_sidescan.mb71` を付けたものになります（`sidescan` の部分は `--line-name2` で変更できます）。`--line-time-list`、`--line-route`、`--line-position-list` で測線を定義した場合、出力は測線ごとに分割され、`Survey_sidescan_0000.mb71`、`Survey_sidescan_0001.mb71` … という名前になります（`Survey` は `--line-name1` で変更できます）。

## 主なオプション

- `--input=datalist` — 時系列サイドスキャンを含むデータリストまたは単一ファイル。既定値: `datalist.mb-1`。
- `--format=format` — 入力のフォーマット ID。既定ではファイル名から判定します。
- `--output-source=record_kind` — 展開するサイドスキャンを含むレコード種別。1（測深データ、既定）や 38（第 2 サイドスキャン）などの番号、または `low` / `sidescan`（種別 1）、`high`（種別 38）。Edgetech の `.jsf` ファイルでは通常、低周波が種別 1、高周波が種別 38 です。ファイル中のレコード種別は `mbinfo -N` で確認できます。
- `--topo-grid-file=filename` — この GMT 地形グリッドにサイドスキャンを展開します。
- `--altitude-bottompick` — 時系列中の最初の海底反射を高度として使います（平坦海底展開）。`--altitude-bottompick-threshold=threshold[/blank]` は検出しきい値（最大振幅に対する比率、既定値 0.5）と、任意のブランキング時間（秒）を設定し、海底検出も有効にします。
- `--altitude-altitude` — 記録された高度を使います（平坦海底展開の既定）。
- `--swath-width=value` — 出力スワス幅を一定値（メートル）にします。
- `--interpolation=value` — 最大 `value` 個の連続した空ピクセルを補間で埋めます。既定値: 0（補間なし）。
- `--gain=value` — このゲイン値でゲイン補正を行います。
- `--channel-swap` — 左舷と右舷のチャンネルを入れ替えます（注意を参照）。
- `--line-time-list=filename` — このファイルの時刻（エポック秒、1 行に 1 つ。例: `mbroutetime` で作成）で出力を測線に分割します。
- `--line-route=filename` — MB-System のルートファイルのウェイポイントで出力を分割します。`--line-position-list=filename` は単純な `longitude latitude` のウェイポイント一覧を使います。
- `--line-range-threshold=value` — ウェイポイントに到達したとみなす距離（メートル）。既定値: 50。
- `--line-name1=name`、`--line-name2=name` — 出力ファイル名の構成部分。既定値: `Survey` と `sidescan`。
- `--nav-file=filename`、`--nav-file-format=format_id` — 外部ファイルから航法を取り込みます（例: フォーマット 1 = `time_d lon lat speed`、2 = `year month day hour minute second lon lat`、6 または 7 = NMEA 文）。`--nav-async=record_kind` はデータ中の別のレコード種別（例: 12、29、30、31）から航法を取ります。
- `--sensordepth-file`、`--altitude-file`、`--heading-file`、`--attitude-file`、`--soundspeed-file` — 同様にセンサー深度、高度、方位、姿勢、音速を取り込みます。それぞれ対応する `-file-format` と `-async` オプションがあります。
- `--soundspeed-constant=value` — 一定の水中音速（m/s）。既定値: 1500。
- `--platform-file=FILE`、`--platform-target-sensor=SENSORID` — センサーオフセットを記述したプラットフォームファイルと、その中のサイドスキャンセンサーの ID。
- `--time-latency-constant=value` または `--time-latency-file=filename` — 時間遅延補正（秒）。`--time-latency-apply-nav`、`-sensordepth`、`-altitude`、`-heading`、`-attitude`、`-all-ancilliary`、`-survey`、`-all` で選んだデータに適用します。
- `--filter=value` — `--filter-apply-nav`、`-sensordepth`、`-altitude`、`-heading`、`-attitude`、`-all-ancilliary` で選んだ補助データに適用するガウス平滑化フィルタの長さ（秒）。既定値: 0（フィルタなし）。
- `--verbose` — パラメータと進行状況を表示します。`--help` は使用法を表示します。

## 使用例

データリスト中のすべての Edgetech ファイルの低周波サイドスキャンを、記録された高度を使って平坦海底上に展開し、最大 5 ピクセルの空きを埋めます。

    mbsslayout --input=datalist.mb-1 --output-source=low --interpolation=5 --verbose

高周波サイドスキャンを、一定の 150 m スワス幅で水深グリッド上に展開します。

    mbsslayout --input=datalist.mb-1 --output-source=high --topo-grid-file=survey_bathy.grd --swath-width=150 --line-name2=hf

高度に海底検出を使い、姿勢を 2 秒で平滑化し、時刻リストに従って出力を測線に分割します。

    mbsslayout --input=datalist.mb-1 --altitude-bottompick-threshold=0.5/0.01 --filter=2.0 --filter-apply-attitude --line-time-list=line_times.txt --line-name1=Area1

## 注意

- すべてのオプションは長い `--name=value` 形式です。1 文字のオプションはありません。
- 出力ファイルはフォーマット 71 で書き出されます。以後の処理のために（例: `mbdatalist` や `mbmakedatalist` で）データリストを作成してください。
- 測線を定義した場合、出力ファイルは現在のディレクトリに、そうでない場合は各入力ファイルと同じ場所に書き出されます。
- プロットスクリプト `Survey_sidescan_ssswathplot.cmd`（`--line-name1` と `--line-name2` に基づく名前）も書き出されます。これは Windows では利用できない `mbm_plot` マクロを呼び出すものです。`chmod` が見つからないというメッセージは無視して構いません。
- 現在のソースでは `--channel-swap` は値を必要とします。`--channel-swap=1` のように指定してください（値は何でも入れ替えが有効になります）。

## 関連項目

`mbbackangle`、`mbprocess`、`mbmosaic`、`mbgrid`、`mbmakeplatform`、`mbroutetime`、`mbinfo`、`mbdatalist`、`mb7k2jstar`。完全なリファレンスは MB-System のマニュアルページ mbsslayout(1)（英語）です。
