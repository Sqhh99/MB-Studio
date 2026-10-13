# mbotps — OSU の TPXO 潮汐モデルによる潮位予測（OTPS 方式）

`mbotps` は、オレゴン州立大学（OSU）の全球潮汐モデル TPXO から、OSU Tidal Prediction Software（OTPS）の手法を用いて海洋潮汐を予測します。1 地点・指定期間の潮位時系列、またはスワスファイルごとに航跡に沿った潮位ファイルを作成でき、`mbprocess` で潮位補正を適用するよう設定することもできます。測量に対して験潮所のデータや GPS 潮位がない場合に使用します。

## 書式

    mbotps
        [
        --input=datalist {-Idatalist}
        --format=format_id {-Fformat_id}
        --use-mbprocess {-M}
        --skip-existing {-S}
        --tide-output=file {-Ooutput}
        --tide-position=lon/lat {-Rlon/lat}
        --tide-format=format_id {-Atideformat}
        --interval=interval {-Dinterval}
        --start-time=year/month/day/hour/minute/second {-Byear/month/day/hour/minute/second}
        --end-time=year/month/day/hour/minute/second {-Eyear/month/day/hour/minute/second}
        --tide-station-file=file {-Ntidestationfile}
        --tide-station-position=lon/lat {-Ulon/lat}
        --tide-station-format=format_id {-Ctidestationformat}
        --otps-model=model_name {-Tmodel}
        --otps-path=path {-Ppath}
        --help {-H}
        --verbose {-V}
        ]

## 説明

`mbotps` は潮位を自前で計算します（交点補正と副次分潮の推定を含む調和予測を、モデルグリッドから内挿）。OTPS の Fortran プログラムは不要です。ただし、OSU から TPXO 潮汐モデルを入手し（学術・非商用は無償、商用は有償ライセンス）、そのデータファイルを `--otps-path` で指定する OTPS の場所の下の `DATA` フォルダに配置する必要があります（例: `C:\otps\DATA\TPXO10_atlas_v2`）。読み込まれるのは `h_*` の潮位ファイルだけです。`mbotps --help` を実行すると、使用中の OTPS の場所と見つかったモデルが表示されます。

地点モード（`--input` なし）: `--tide-position` で指定した地点の潮位を、`--start-time` から `--end-time` まで `--interval` 秒ごと（既定値 60）に予測し、`--tide-output`（既定値 `tide_model.txt`）に書き出します。

スワスモード（`--input` あり）: ファイルまたはデータリストの各スワスファイルについて、航跡に沿って（`.fnv` ファイルがあればそれを使用）指定間隔で潮位を予測し、スワスファイル名に `.tde` を付けたファイルをスワスファイルと同じ場所に書き出します。`--use-mbprocess` を指定すると、`mbprocess` がこの潮位ファイルを適用するよう各ファイルの処理パラメータが設定されます。モデルグリッドの範囲外や陸上の位置はメッセージを表示してスキップされます。

験潮所による補正: `--tide-station-file`、`--tide-station-position`、`--tide-station-format` を指定すると、近くの験潮所の観測潮位とその地点のモデル値を比較し、その差を時間変化する補正値としてどちらのモードでも予測潮位に加えます。

## 主なオプション

- `--input=datalist` — スワスファイルまたはデータリスト。スワスモードになります（短縮形 `-Idatalist`）。
- `--format=format_id` — 入力の MB-System フォーマット。通常はファイル名から判別され、データリストでは負の値です（`-Fformat_id`）。
- `--use-mbprocess` — `mbprocess` が `.tde` ファイルを適用するよう、スワスファイルの処理パラメータを設定します（`-M`）。
- `--skip-existing` — `.tde` ファイルがすでにデータより新しいスワスファイルをスキップします（`-S`）。
- `--tide-output=file` — 地点モードの出力ファイル（`-Ooutput`）。既定値は `tide_model.txt` です。
- `--tide-position=lon/lat` — 地点モードの位置（10 進度）（`-Rlon/lat`）。
- `--start-time=year/month/day/hour/minute/second` — 予測する時系列の開始時刻（`-B...`）。
- `--end-time=year/month/day/hour/minute/second` — 予測する時系列の終了時刻（`-E...`）。
- `--interval=interval` — 時間間隔（秒）（`-Dinterval`）。既定値は 60 です。
- `--tide-format=format_id` — 2 = `yr mon day hour min sec tide`（既定値）、1・3・4 = エポック秒による `time_d tide`（`-Atideformat`）。
- `--tide-station-file=file` — モデルの補正に使う験潮所の観測データ（`-Ntidestationfile`）。
- `--tide-station-position=lon/lat` — 験潮所の位置（`-Ulon/lat`）。
- `--tide-station-format=format_id` — 験潮所ファイルの形式。1 = `time_d tide`、2 = `yr mon day hour min sec tide`（既定値）、3 = `yr jday hour min sec tide`、4 = `yr jday daymin sec tide`（`-Ctidestationformat`）。
- `--otps-model=model_name` — 使用するモデル。`DATA` 下のモデルフォルダ名（例: `TPXO10_atlas_v2`）または `Model_` 制御ファイルの名前（`-Tmodel`）。既定値は `tpxo10_atlas` で、指定したモデルがない場合は他のインストール済みモデルが使われます。
- `--otps-path=path` — OTPS の場所、つまり `DATA` を含むフォルダ（`-Ppath`）。既定値はビルド時に設定されます。
- `--verbose` — 利用可能なモデルの一覧を含む詳細な出力（`-V`）。
- `--help` — 使い方と利用可能なモデルを表示して終了します（`-H`）。

## 使用例

`mbotps` が潮汐モデルを探す場所と、見つかったモデルを確認します：

    mbotps --otps-path=C:\otps --help

西経 121.9 度・北緯 36.6 度の 1 日分の潮位を 60 秒間隔で予測します：

    mbotps --otps-path=C:\otps --tide-position=-121.9/36.6 --start-time=2025/02/21/00/00/00 --end-time=2025/02/21/23/59/59 --tide-output=tide_20250221.txt

データリスト内の全ファイルについて航跡に沿った潮位ファイルを作成し、`mbprocess` で適用します：

    mbotps --otps-path=C:\otps --input=datalist.mb-1 --use-mbprocess --verbose
    mbprocess -I datalist.mb-1

近くの験潮所の観測値（形式 2 のファイル）でモデルを補正します：

    mbotps --otps-path=C:\otps --input=datalist.mb-1 --tide-station-file=gauge_2025.txt --tide-station-position=-121.89/36.61 --tide-station-format=2 --use-mbprocess

## 注意

- `--otps-path`、`--input`、`--tide-output`、`--tide-station-file` の値は最初のスペースまでしか読み込まれないため、スペースを含まないパスを使ってください（例: `C:\otps`）。
- 使用できるモデルが見つからない場合は "Unable to find a valid OTPS tidal model" と表示して終了します。`DATA` フォルダの構成と `--otps-path` の値を確認してください。
- `.tde` ファイルはカレントディレクトリではなく、データリストに記載されたスワスファイルと同じ場所に書き出されます。
- 予測値にはモデルの分潮による潮汐成分だけが含まれ、地域の基準面や潮汐以外の水位変化は含まれません（験潮所による補正でこうした差を小さくできます）。

## 関連項目

`mbprocess`, `mbset`, `mbgpstide`, `mbdatalist`, `mbnavlist`。完全なリファレンスは MB-System のマニュアルページ mbotps(1)（英語）です。
