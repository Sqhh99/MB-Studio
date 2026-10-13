# mbfnv2navlab — fnv 航法ファイルを Navlab バイナリに変換

`mbfnv2navlab` は、MB-System の高速航法（fnv）テキストファイルを Kongsberg Navlab のスムージング済み航法バイナリファイル（`navlab_smooth.bin` 形式）に変換します。MB-System で補正・調整した航法を、Navlab 形式の航法ソースとして処理に戻す場合（たとえば `mbpreprocess` の航法フォーマット 12 として与える場合）に使用します。`mbnavlab2fnv` の逆変換にあたります。

## 書式

    mbfnv2navlab [-I input.fnv] [-O navlab_smooth.bin] [-V] [-H]
    mbfnv2navlab [--input=file] [--output=file] [--verbose] [--help]

## 説明

入力は、`mbpreprocess` や `mbnavlab2fnv` などの MB-System プログラムが出力するタブ区切りの fnv テキストファイルです。`#` で始まる行と空行は読み飛ばされます。各データ行には少なくとも 10 個のタブ区切りフィールドが必要です：日時（フィールド内はスペース区切りの `yyyy mm dd hh mm ss.ssssss`）、エポック秒、経度、緯度、針路、速度（km/h）、喫水（m）、ロール、ピッチ、ヒーブ。その後に続くことがあるスワス端の 4 フィールドは受け付けますが使用しません。

出力は入力 1 行につき 1 レコードで、各レコードはリトルエンディアンの IEEE 754 倍精度値 21 個（168 バイト）からなり、ファイルヘッダはありません。時刻はエポック秒フィールドから取得します。緯度、経度、喫水（深度として書き込み）、針路、ロール、ピッチはそのままコピーされ、速度は km/h から m/s に換算して 2 つの速度フィールド（10 と 11）の両方に書き込まれます。Navlab 形式にはヒーブのフィールドがないため、ヒーブは読み込まれますが破棄されます。

Navlab レコードのその他のフィールド（4、5、9、12～20：位置オフセット、鉛直速度、不確かさ、共分散項）はすべて 0.0 になるため、出力は元の Navlab ファイルを完全には再現できません。このファイルを `mbpreprocess` や `mbprocess` の航法ソースとして使う場合は値の入っているフィールドだけが使われ、0 のフィールドは影響しません。出力のサンプリングレートは入力 fnv ファイルと同じで、通常は元の Navlab 出力の 100 Hz よりかなり低くなります。

タブフィールドが 10 個未満の行、およびタイムスタンプが 0 以下または直前のレコード以前のレコードは読み飛ばされます。`-V` を指定すると、読み飛ばした各行が行番号と理由とともに報告され、最後に読み込み行数、書き込みレコード数、スキップ行数の集計が表示されます。

## 主なオプション

- `-I input.fnv` — 入力 fnv テキストファイル（ロング形式 `--input=file`）。省略すると標準入力から読み込みます。
- `-O navlab_smooth.bin` — 出力 Navlab バイナリファイル（ロング形式 `--output=file`）。省略すると標準出力に書き込みます。
- `-V` — 詳細モード（ロング形式 `--verbose`）。ファイル名、スキップした行、最終集計を標準エラーに出力します。
- `-H` — プログラム名、使い方、fnv から Navlab へのフィールド対応を表示して終了します（ロング形式 `--help`）。

## 使用例

fnv ファイルを Navlab バイナリファイルに変換します：

    mbfnv2navlab -I mission.fnv -O adjusted_nav.bin

ロング形式のオプションを使い、進行状況を表示しながら変換します：

    mbfnv2navlab --verbose --input=mission.fnv --output=adjusted_nav.bin

変換結果を、Kongsberg KMALL データ（フォーマット 261）の前処理における航法、センサー深度、姿勢、方位のソースとして使用します：

    mbpreprocess --format=261 --input=datalist.mb-1 --nav-file=adjusted_nav.bin --nav-file-format=12 --sensordepth-file=adjusted_nav.bin --sensordepth-file-format=12 --attitude-file=adjusted_nav.bin --attitude-file-format=12 --heading-file=adjusted_nav.bin --heading-file-format=12

## 注意

- Windows ではバイナリ出力は必ず `-O` で指定してください。コマンドプロンプトで標準出力をファイルにリダイレクト（`> file.bin`）すると、バイナリデータが壊れることがあります。
- 入力はタブ区切りである必要があります。列がスペースだけで区切られたファイルは、行ごとに読み飛ばされます（理由は `-V` で確認できます）。
- スペースを含むパスは引用符で囲んでください。例：`-I "D:\Survey 1\mission.fnv"`。

## 関連項目

`mbnavlab2fnv`、`mbpreprocess`、`mbprocess`、`mblist`、`mbnavlist`。完全なリファレンスは MB-System のマニュアルページ mbfnv2navlab(1)（英語）です。
