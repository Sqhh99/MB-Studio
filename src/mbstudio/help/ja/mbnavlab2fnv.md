# mbnavlab2fnv — Navlab バイナリを fnv 航法ファイルに変換

`mbnavlab2fnv` は、Kongsberg Navlab のスムージング済み航法バイナリファイル（通常 `navlab_smooth.bin`）を MB-System の高速航法（fnv）テキストファイルに変換します。Navlab は Kongsberg Hugin AUV のスムージング済み航法・姿勢解を計算する後処理ソフトウェアです。この解を MB-System に取り込む場合、たとえば `mbpreprocess` の航法、センサー深度、姿勢、方位のソースとして使う場合に使用します。

## 書式

    mbnavlab2fnv [-I navlab_smooth.bin] [-O output.fnv] [-V] [-H]
    mbnavlab2fnv [--input=file] [--output=file] [--verbose] [--help]

## 説明

入力ファイルにはヘッダがなく、168 バイトのレコードが並びます。各レコードはリトルエンディアンの IEEE 754 倍精度値 21 個からなり、公称サンプリングレートは 100 Hz です。本プログラムは各レコードから時刻（Unix エポック秒）、緯度、経度、深度、針路、ロール、ピッチ、対水速度を使用し、その他のフィールド（不確かさ、共分散項、鉛直速度）は無視します。

出力はタブ区切りの fnv テキストファイルです。1 行目は列名を示す `##` コメント行です。以降の各行には、日時（第 1 フィールド内はスペース区切りの `yyyy mm dd hh mm ss.ssssss`）、エポック秒、経度、緯度、針路、速度（km/h）、喫水（m）、ロール、ピッチ、ヒーブが入ります。Navlab の深度は喫水として書き込まれ、速度は m/s から km/h に換算され、日時は UTC で計算されます。

Navlab 形式にはヒーブが含まれないため、ヒーブは常に 0.0 として書き込まれます。ヒーブが必要な場合はソナーのデータストリームから取得してください。一部の fnv ファイルにあるポート側・スターボード側のスワス端の列は、スワス形状の情報がないため出力されません。

タイムスタンプが直前のレコード以前のレコードは読み飛ばされます。`-V` を指定すると読み飛ばした各レコードが報告され、最後に読み込み・書き込みレコード数が表示されます。

## 主なオプション

- `-I navlab_smooth.bin` — 入力 Navlab バイナリファイル（ロング形式 `--input=file`）。省略すると標準入力から読み込みます。
- `-O output.fnv` — 出力 fnv テキストファイル（ロング形式 `--output=file`）。省略すると標準出力に書き込みます。
- `-V` — 詳細モード（ロング形式 `--verbose`）。ファイル名、スキップしたレコード、最終集計を標準エラーに出力します。
- `-H` — プログラム名、使い方、Navlab のフィールド構成を表示して終了します（ロング形式 `--help`）。

## 使用例

Navlab バイナリファイルを fnv ファイルに変換します：

    mbnavlab2fnv -I navlab_smooth.bin -O mission.fnv

ロング形式のオプションを使い、進行状況を表示しながら変換します：

    mbnavlab2fnv --verbose --input=navlab_smooth.bin --output=mission.fnv

fnv ファイル（航法フォーマット 9）を、Kongsberg KMALL データ（フォーマット 261）の前処理における航法、センサー深度、姿勢、方位のソースとして使用します：

    mbpreprocess --format=261 --input=datalist.mb-1 --nav-file=mission.fnv --nav-file-format=9 --sensordepth-file=mission.fnv --sensordepth-file-format=9 --attitude-file=mission.fnv --attitude-file-format=9 --heading-file=mission.fnv --heading-file-format=9

## 注意

- Windows ではバイナリ入力は必ず `-I` で指定してください。コマンドプロンプトで標準入力からバイナリファイルを与える方法（`< navlab_smooth.bin`）は確実に動作しません。
- `mbpreprocess` はフォーマット 12 として Navlab バイナリファイルを直接読むこともできます。fnv への変換は、航法をテキストとして確認・編集・プロットしたい場合に便利です。
- スペースを含むパスは引用符で囲んでください。例：`-I "D:\AUV dive 12\navlab_smooth.bin"`。

## 関連項目

`mbfnv2navlab`、`mbpreprocess`、`mbprocess`、`mbnavlist`。完全なリファレンスは MB-System のマニュアルページ mbnavlab2fnv(1)（英語）です。
