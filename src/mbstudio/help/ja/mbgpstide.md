# mbgpstide — スワスデータの GPS 楕円体高から潮位ファイルを作成

`mbgpstide` は、スワスデータファイルに記録された GPS の高さ（楕円体高）から潮位ファイルを作成します。高さには一定のオフセットを加えたり、ジオイドモデルで補正したりできるため、水深を水位ではなく楕円体またはジオイド基準にできます。RTK や PPK による GPS 高を使った測量で、験潮所や潮汐モデルの代わりに使用し、結果は `mbprocess` で適用します。

## 書式

    mbgpstide [-Atideformat -Dinterval -Fformat -Idatalist -M -Ooutput -Roffset -S -Tgeoid -Usource -V -H]
    mbgpstide [--tideformat=tideformat --interval=interval --format=format --input=datalist
              --setparameters --output=output --offset=offset --skipexisting --geoid=geoid
              --use=source --verbose --help]

## 説明

本プログラムは単一のスワスファイルまたはデータリスト（既定値 `datalist.mb-1`）を読み込み、GPS 高を一定の時間間隔（既定値 300 秒）で平均します。潮位ファイルに書き込まれる各値は、平均した高さに `-R` のオフセットを加え、`-T` を使う場合はジオイド高を差し引いたものです。

高さの取得元はフォーマットによって異なります。Kongsberg（Simrad）EM のフォーマット 56、57、58、59 では Height データグラムから取得します。GSF ファイル（フォーマット 121）では ping ヘッダから取得し、`-U1` を指定すると ping の分離量（`sep`）が加算され、GSF ファイルの鉛直基準面に対する高さになります。その他のフォーマットには利用できる高さが含まれず、作成される潮位ファイルは空になります。

`-O` を指定しない場合は、スワスファイルごとに、スワスファイル名に `.gps.tde` を付けた潮位ファイルがスワスファイルと同じ場所に作成されます。`-O` を指定すると、すべての値が 1 つのファイルに書き込まれます。`-M` は各スワスファイルのパラメータファイルを更新し、`mbprocess` が新しい潮位ファイルを適用するようにします。補正を反映させるには、その後で `mbprocess` を実行してください。

## 主なオプション

- `-I datalist` — 入力スワスファイルまたはデータリスト（ロング形式 `--input=datalist`）。既定値は `datalist.mb-1` です。
- `-F format` — 入力の MB-System フォーマット ID。負の値は入力がデータリストであることを示します（ロング形式 `--format=format`）。通常はファイル名から判別されます。
- `-D interval` — 潮位値の時間間隔（秒）（ロング形式 `--interval=interval`）。既定値は 300 です。`-D0` ではすべての高さを書き出すので、時刻の重複を避けるため `-A1` を使ってください。
- `-A tideformat` — 出力形式（ロング形式 `--tideformat=tideformat`）。2 = `year month day hour minute second tide`（既定値）、1 = `time_d tide`（エポック秒）、5 = CARIS 形式 `yyyy/mm/dd hh:mm:ss.sss tide`。5 は `mbprocess` が読めないため `-M` と併用しないでください。
- `-O output` — すべての潮位値をこの 1 つのファイルに書き込みます（ロング形式 `--output=output`）。`-O-` は標準出力に書き出します（`-M` とは併用不可）。
- `-M` — `mbprocess` が潮位ファイルを適用するよう、スワスファイルの処理パラメータを設定します（ロング形式 `--setparameters`）。
- `-R offset` — すべての潮位値にこの定数（m）を加えます（ロング形式 `--offset=offset`）。
- `-S` — `.gps.tde` ファイルがすでにデータより新しいスワスファイルをスキップします（ロング形式 `--skipexisting`）。`-O` を使わない場合のみ有効です。
- `-T geoid` — 楕円体からのジオイド高の GMT グリッド（例: EGM2008 から変換したもの）。航跡に沿った値が差し引かれます（ロング形式 `--geoid=geoid`）。
- `-U source` — 高さの取得元（ロング形式 `--use=source`）。0 = Kongsberg Height データグラムまたは GSF の楕円体高（既定値）、1 = GSF の鉛直基準面からの高さ。
- `-V` — 詳細な出力（ロング形式 `--verbose`）。
- `-H` — 使い方を表示して終了します（ロング形式 `--help`）。

## 使用例

データリスト内の各ファイルの潮位ファイルを作成し、`mbprocess` で適用するよう設定します：

    mbgpstide -I datalist.mb-1 -M
    mbprocess -I datalist.mb-1

60 秒平均を使い、一定のオフセット -35.20 m を加え、最新の潮位ファイルがあるファイルはスキップします：

    mbgpstide -I datalist.mb-1 -D60 -R-35.20 -S -M

調査全体の潮位をエポック秒形式で 1 つのファイルに書き出します：

    mbgpstide -I datalist.mb-1 -A1 -O survey_gps_tide.txt

1 つのファイルについて、GSF の鉛直基準面からの高さを使います：

    mbgpstide -I line_0001.gsf -F121 -U1 -M

## 注意

- 潮位ファイルはカレントディレクトリではなく、データリストに記載されたスワスファイルと同じ場所に書き出されます（`-O` を使う場合を除く）。
- `-T` は、`awk` または `mblist` の出力を GMT の `grdtrack` にパイプで渡すコマンドをシステムのシェルで実行してジオイド高を取得します。Windows のコマンドプロンプトでは通常これらのコマンドがこの名前で利用できないため、`-T` は失敗する可能性があります。その場合、調査範囲でジオイド高がほぼ一定であれば `-R` で定数として補正してください。
- スペースを含むパスは引用符で囲んでください。

## 関連項目

`mbprocess`, `mbset`, `mbotps`, `mbdatalist`, `mblist`。完全なリファレンスは MB-System のマニュアルページ mbgpstide(1)（英語）です。
