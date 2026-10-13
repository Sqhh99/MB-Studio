# mbusbl2fnv — USBL 追跡 CSV ファイルを fnv 航法に変換

`mbusbl2fnv` は、CSV 形式の USBL（超短基線）による ROV 追跡ファイルを MB-System の高速航法（fnv）テキストファイルに変換します。USBL 位置と ROV の深度を MB-System に取り込む場合、たとえば ROV 搭載ソナーのデータ処理で `mbpreprocess` の航法とセンサー深度のソースとして使う場合に使用します。

## 書式

    mbusbl2fnv [-I usbl.csv] [-O output.fnv] [-V] [-H]
    mbusbl2fnv [--input=usbl.csv] [--output=output.fnv] [--verbose] [--help]

## 説明

入力 CSV ファイルは 1 行のヘッダと、それに続くカンマ区切りの 4 フィールドのデータ行からなります。フィールドは Unix エポック秒（UTC）の時刻、10 進度の緯度（北が正）、10 進度の経度（東が正）、ROV の深度（m、下向きが正）です。最初の空白以外の文字が数値の先頭になり得る行だけが読み込まれるため、ヘッダ行と `#` で始まるコメント行は自動的に読み飛ばされます。数値がちょうど 4 つでない行や、時刻が直前のレコードより後でないレコードも読み飛ばされます。

出力はタブ区切りの fnv テキストファイルです。1 行目は列名を示す `##` コメント行で、以降の各行には日時（`yyyy mm dd hh mm ss.ssssss`）、エポック秒、経度、緯度、針路、速度（km/h）、喫水（m）、ロール、ピッチ、ヒーブが入ります。ROV の深度は喫水として書き込まれます。針路、速度、ロール、ピッチ、ヒーブは USBL 追跡データに含まれないため 0.0 として書き込まれ、一部の fnv ファイルにあるスワス端の列は出力されません。

`-I` を省略すると標準入力から読み込み、`-O` を省略すると標準出力に書き込みます。

## 主なオプション

- `-I usbl.csv` — 入力 USBL 追跡 CSV ファイル（ロング形式 `--input=usbl.csv`）。既定値は標準入力です。
- `-O output.fnv` — 出力 fnv ファイル（ロング形式 `--output=output.fnv`）。既定値は標準出力です。
- `-V` — 詳細モード（ロング形式 `--verbose`）。ファイル名、スキップした行、読み込み・書き込み・スキップしたレコード数を表示します。
- `-H` — 使い方と想定する CSV の構成を表示して終了します（ロング形式 `--help`）。

## 使用例

USBL 追跡ファイルを fnv ファイルに変換します：

    mbusbl2fnv -I usbl_track.csv -O rov.fnv

ロング形式のオプションを使い、進行状況を表示しながら変換します：

    mbusbl2fnv --verbose --input=usbl_track.csv --output=rov.fnv

fnv ファイル（航法フォーマット 9）を `mbpreprocess` の航法とセンサー深度のソースとして使います：

    mbpreprocess --input=datalist.mb-1 --nav-file=rov.fnv --nav-file-format=9 --sensordepth-file=rov.fnv --sensordepth-file-format=9

## 注意

- 針路、ロール、ピッチ、ヒーブは 0.0 として書き込まれるため、この fnv ファイルを針路や姿勢のソースとして使わないでください。これらは機体自身のセンサーから取得してください。
- USBL の測位値はノイズが多いことがよくあります。処理に使う前に位置を確認し、必要ならクリーニングしてください。
- コマンドプロンプトでもリダイレクトを使えます（例: `mbusbl2fnv < usbl_track.csv > rov.fnv`）。スペースを含むパスは引用符で囲んでください。

## 関連項目

`mbpreprocess`, `mbprocess`, `mbnavlab2fnv`, `mbfnv2navlab`, `mbminirovnav`, `mbnavlist`。完全なリファレンスは MB-System のマニュアルページ mbusbl2fnv(1)（英語）です。
