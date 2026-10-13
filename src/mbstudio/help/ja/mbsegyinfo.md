# mbsegyinfo — SEGY ファイルの内容を要約

`mbsegyinfo` は SEGY 形式の地震探査またはサブボトムプロファイラのファイルを
読み込み、基本的な統計情報を表示します。`mbextractsegy`（またはその他の
SEGY 出力元）で作成した SEGY ファイルの確認や、`mbsegygrid` と `mbsegypsd` が
グリッドの大きさを決めるために使う `.sinf` 情報ファイルの作成に使用します。

## 書式

    mbsegyinfo -Ifile [-Llonflip -O -H -V]
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --output-file {-O}
        --verbose {-V}

## 説明

レポートには、ファイルヘッダの値（チャンネル数、サンプル間隔、トレースあたりの
サンプル数、トレース長、サンプルのデータ形式、CDP 重合数）、トレース数、
ショット番号・トレース番号・RP（CMP）番号・遅延・距離・送受波器の標高・水深の
最小値、最大値、範囲、最初と最後のトレースの時刻と位置、データの地理的範囲が
含まれます。

既定ではレポートは画面に表示されます。`-O` を指定すると、入力ファイル名に
`.sinf` を付けたファイル（例：`line_0001_env.segy.sinf`）に書き出されます。
他の MB-System プログラムはこの `.sinf` ファイルを読み込みます。
`mbextractsegy` は書き出す SEGY ファイルごとにこれを自動作成し、`mbsegygrid` と
`mbsegypsd` は `.sinf` ファイルがない場合や SEGY ファイルより古い場合に自分で
`mbsegyinfo -O` を実行します。

`mbsegyinfo` は 1 回の実行で 1 つの SEGY ファイルを読み込みます。データリストは
受け付けません。

## 主なオプション

- `-I<file>` — 入力 SEGY ファイル。必須。
- `-O` — レポートを画面ではなく `<file>.sinf` に書き出します。
- `-L<lonflip>` — 表示する経度の範囲：`-1` = -360～0、`0` = -180～180、
  `1` = 0～360。既定：`mbdefaults` の設定値。
- `-V` — 詳細モード。プログラムのバージョンと読み込みエラーも表示します。
- `-H` — 簡単なヘルプを表示して終了します。

## 使用例

SEGY ファイルの統計情報を表示します。

    mbsegyinfo -I sbp_0001_env.segy

`.sinf` ファイル `sbp_0001_env.segy.sinf` を作成（または更新）します。

    mbsegyinfo -I sbp_0001_env.segy -O

画面出力をリダイレクトして、任意の名前でレポートを保存します。

    mbsegyinfo -I sbp_0001_env.segy > sbp_0001_report.txt

## 注意

- 空白を含むファイル名は引用符で囲んでください（例：`-I "Line 12.segy"`）。
- `.sinf` ファイルは入力ファイルと同じ場所に書き出されます。

## 関連項目

`mbextractsegy`、`mbsegylist`、`mbsegygrid`、`mbsegypsd`、`mbinfo`。完全な
リファレンスは MB-System のマニュアルページ mbsegyinfo(1)（英語）です。
