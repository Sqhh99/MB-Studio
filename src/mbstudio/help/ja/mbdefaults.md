# mbdefaults — MBIO の既定制御パラメータの設定・表示

`mbdefaults` は、ユーザーごとの MB-System（MBIO）既定制御パラメータを表示または変更します。経度範囲の表し方、データの途切れとみなす時間間隔、I/O バッファサイズ、「高速水深」（`.fbt`）ファイルのバージョンなどが対象です。オプションなしで実行すると現在の既定値を表示し、オプションを指定すると新しい値を保存して以後のすべての MB-System プログラムで使用します。

## 書式

    mbdefaults
        --fbt-version=fbtversion {-Ffbtversion}
        --file-io-buffer=fileiobuffer {-Bfileiobuffer}
        --help {-H}
        --image-display=imagedisplay {-Iimagedisplay}
        --lonflip=lonflip {-Llonflip}
        --mbview-settings=mbviewsettings {-Mmbviewsettings}
        --project=mbproject {-Wmbproject}
        --ps-display=psdisplay {-Dpsdisplay}
        --time-gap=timegap {-Ttimegap}
        --use-lock-files=yes|no {-Uyes|no}
        --verbose {-V}

## 説明

既定値はユーザーのホームディレクトリにあるテキストファイル `.mbio_defaults` に保存されます。Windows では、環境変数 `HOME` が設定されていればそのフォルダ、なければ `%USERPROFILE%` が使われます（例: `C:\Users\name\.mbio_defaults`）。このファイルがない場合は MBIO の組み込み既定値が使われます。

設定用のオプションを指定しない場合、`mbdefaults` は現在の値を表示するだけです。設定用のオプションを 1 つでも指定すると、新しい値と変更されなかった値をすべて含む `.mbio_defaults` ファイルが新たに書き出され、新しいパラメータ一覧が表示されます。

既定値は MBIO を通してスワスデータを読むプログラムに影響します。たとえば `mblist` や `mbgrid` が使う経度の表し方や、航法データの途切れの検出です。旧バージョンの MB-System ではフォーマット ID、ピング平均、範囲や時間の切り出しもここに保存していましたが、現在は既定値に含まれません。

## 主なオプション

- `-Llonflip` — 出力する経度の範囲（`--lonflip`）。`-1` は -360～0 度、`0` は -180～180 度、`1` は 0～360 度。既定値: 0。
- `-Ttimegap` — 隣接するピングの間隔がこれ（分）を超えるとデータの途切れとみなします（`--time-gap`）。既定値: 1。
- `-Bfileiobuffer` — 対応するフォーマット（現在はフォーマット 58 と 88）のファイル I/O バッファサイズ（キロバイト）。例: `-B10000`（`--file-io-buffer`）。主にネットワーク経由でデータを読む場合に有効です。既定値: 0（システム既定のバッファリング）。
- `-Ffbtversion` — `mbdatalist` と `mbprocess` が書き出す `.fbt` ファイルのバージョン（`--fbt-version`）。`3`、`new`、`NEW` は現行版、`2`、`old`、`OLD` は 5.3.1897 より前の版。既定値: 3。
- `-Uyes|no` — 1 つのスワスファイルを同時に 1 つのプログラムだけが処理・編集するようにロックファイルを使うかどうか（`--use-lock-files`）。`yes`/`1` または `no`/`0`。
- `-Mmbviewsettings` — 3D ビューアの既定表示設定（`--mbview-settings`）。`-MPcolortable/mode/shade_mode`、`-MGcolortable/mode`（傾斜）、`-MOcolortable/mode`（オーバーレイ）、`-MImagnitude/elevation/azimuth`（照明）、`-MSmagnitude`（傾斜による陰影）。カラーテーブル: 0 Haxby、1 Bright、2 Muted、3 Grayscale、4 Flat gray、5 Sealevel 1、6 Sealevel 2。
- `-Dpsdisplay` — プロットスクリプトが PostScript ファイルの表示に使うプログラム（`--ps-display`）。既定値: `Default`（システムの既定ビューア）。
- `-Iimagedisplay` — 画像ファイルの表示に使うプログラム（`--image-display`）。既定値: `Default`。
- `-Wmbproject` — 既定のプロジェクト名（`--project`）。既定値: `none`。
- `-V` — プログラムのバージョンも表示します（`--verbose`）。
- `-H` — 簡単な説明と使用法を表示して終了します（`--help`）。

## 使用例

現在の既定パラメータを表示します。

    mbdefaults

経度を 0～360 度の範囲で出力します（180 度経線をまたぐ調査で便利です）。

    mbdefaults -L1

途切れとみなす時間間隔を 2 分、ファイル I/O バッファを 10 MB に設定します。

    mbdefaults -T2 -B10000

## 注意

- 設定はユーザーごとです。Windows のユーザーアカウントごとに `.mbio_defaults` ファイルがあります。
- 変更のたびにファイル全体が現在の値で書き直されるため、変更されるのは指定したオプションの値だけです。
- Windows 版ではロックファイルの仕組みは使われないため、`-U` の設定は実質的に効果がありません。
- PostScript/画像ビューアと mbview の設定は、Windows 版に含まれないツール（プロットマクロ、3D ビューア）だけが使用します。
- `-MS` の `S` は大文字で指定してください。

## 関連項目

`mbinfo`、`mblist`、`mbgrid`、`mbdatalist`、`mbprocess`、`gmt mbswath`、`gmt mbcontour`、`gmt mbgrdtiff`。完全なリファレンスは MB-System のマニュアルページ mbdefaults(1)（英語）です。
