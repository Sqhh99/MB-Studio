# mbnavlist — スワスデータから航法・姿勢データを一覧出力

`mbnavlist` はスワスデータファイルを読み込み、航法の値（時刻、位置、船首方位、速力、ソナー深度、ロール、ピッチ、ヒーブなど）を表形式で画面に出力します。作図用の船舶やビークルの航跡の抽出、航法品質の確認、データファイルに記録された非同期の航法レコードや姿勢レコードの一覧表示に使います。

## 書式

    mbnavlist
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --binary {-A}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --data-kind=kind {-Kkind}
        --decimate=decimate {-Ddecimate}
        --delimiter=delimiter {-Gdelimiter}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --nav-channel=navchannel {-Nnavchannel}
        --output-format=output_format {-Ooutput_format}
        --projection=projection {-Jprojection}
        --segment=segment {-Zsegment}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --verbose {-V}

## 説明

既定ではタブ区切りのテキストを出力します。`-G` で別の区切り文字、`-A` で倍精度のバイナリ出力、`-D` で間引きを指定できます。列は `-O` で選択し、既定値は `tMXYHs`（時刻文字列、Unix 時刻、経度、緯度、船首方位、対地速力）です。

既定では、データフォーマットの「航法ソース」に指定されたレコード種別から航法を取得します。多くのフォーマットは非同期の航法を別レコードにも保存しており、複数の航法システムのデータを持つこともあります。`-N0` は主の非同期航法を、`-N1`、`-N2`、`-N3` は補助の航法チャンネルを出力します。指定したレコードがファイルにない場合は何も出力されません。ファイルに含まれるレコード種別は `mbinfo -N` で確認できます。

`-K` を使うと、別のレコード種別の時刻で出力できます。たとえば `-K18` は姿勢レコードの時刻で出力するため、ロール・ピッチ・ヒーブは本来のサンプリングレートで出力され、位置などの値はその時刻に内挿されます。`-K` は `-N` より優先されます。

`-O` で使える文字: `T` 時刻 yyyy/mm/dd/hh/mm/ss、`t` 時刻 yyyy mm dd hh mm ss、`J` と `j` 年と通日による時刻文字列、`M` Unix 時刻（10 進秒）、`m` 最初のレコードからの秒数、`U` Unix 時刻（整数秒）、`u` 最初のレコードからの整数秒、`V` レコード間隔 (s)、`X` 経度、`x` 経度（度 + 10 進分）、`Y` 緯度、`y` 緯度（度 + 10 進分）、`H` 船首方位、`h` 対地針路、`S` 速力 (km/h)、`s` 対地速力 (km/h)、`L` 航走距離 (km)、`l` 航走距離 (m)、`c` ソナー深度 (m)、`p` 喫水 (m)、`R` ロール、`P` ピッチ、`r` ヒーブ。接頭文字 `-`（符号反転）、`/`（逆数）、`^`（続く `X` または `Y` を `-J` の投影での東距・北距にする）は次の値を修飾します。

## 主なオプション

- `-Ifile` (`--input=file`) — 入力スワスファイルまたはデータリスト。既定値: datalist.mb-1。
- `-Fformat` (`--format=format`) — MBIO フォーマット ID。データリストは -1。既定値: ファイル名の接尾辞から推定。
- `-Ooutput_format` (`--output-format=output_format`) — 出力列を選ぶ文字列。既定値: `tMXYHs`。
- `-Nnavchannel` (`--nav-channel=navchannel`) — 非同期の航法を出力: 0 = 主チャンネル、1～3 = 補助チャンネル。
- `-Kkind` (`--data-kind=kind`) — 出力のきっかけとなるレコード種別。例: 12 = 航法、17 = 船首方位、18 = 姿勢、28～30 = 補助航法 1～3、53 = NMEA GGA。全一覧はマニュアルページを参照してください。
- `-Ddecimate` (`--decimate=decimate`) — decimate 個ごとに 1 レコードだけ出力します。既定値: 1。
- `-Gdelimiter` (`--delimiter=delimiter`) — テキスト出力の列区切り文字。既定値: タブ。
- `-A` (`--binary`) — テキストではなく倍精度のバイナリ値を出力します。
- `-Jprojection` (`--projection=projection`) — `^X` / `^Y` に使う投影座標系。例: `UTM32N`、`epsg32632`、または最初のデータからゾーンを決める UTM を表す `U`。
- `-Zsegment` (`--segment=segment`) — 入力ファイルごとの出力を segment を含む行で区切ります（GMT のマルチセグメントファイルには `>`）。`swathfile` または `datalist` を指定すると、"#" に続けて元のファイルまたはデータリストのパスが書かれます。
- `-Rwest/east/south/north` (`--bounds=...`) — この範囲内のデータだけを使います。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — この時間内のデータだけを使います。
- `-Llonflip` (`--longitude-domain=lonflip`) — 経度の範囲: -1 = -360～0、0 = -180～180、1 = 0～360。既定値: 0。
- `-Sspeed` (`--speed-minimum=speed`), `-Ttimegap` (`--time-gap=timegap`) — 読み込み時の最低速力 (km/h) と最大時間ギャップ（分）。
- `-V` (`--verbose`), `-H` (`--help`) — 詳細メッセージ表示、使い方を表示して終了。

## 使用例

1 ファイルの Unix 時刻・経度・緯度を出力し、テキストファイルに保存します。

    mbnavlist -I 0001_20250101_000000.mb58 -OMXY > nav.txt

データリスト内の各ファイルの航跡を GMT マルチセグメントファイルとして書き出します（cmd.exe がリダイレクトと解釈しないよう `>` を引用符で囲みます）。

    mbnavlist -I datalist.mb-1 -OXY "-Z>" > tracks.txt

姿勢レコードの本来のレートでロール・ピッチ・ヒーブを出力します。

    mbnavlist -I line1.mb88 -K18 -OMRPr > attitude.txt

主の非同期航法を UTM の東距・北距で出力します。

    mbnavlist -I line1.mb88 -N0 "-OtM^X^Y" -JU > nav_utm.txt

## 注意

- 出力は画面に表示されます。保存するには `> file.txt` でリダイレクトしてください。出力ファイルを指定するオプションはありません。
- cmd.exe では `^`、`>`、`|`、`&`、`%` を含む引数を二重引用符で囲んでください。

## 関連項目

`mblist`, `mbinfo`, `mbdatalist`, `mbctdlist`, `mbauvloglist`, `mbformat`。完全なリファレンスは MB-System のマニュアルページ mbnavlist(1)（英語）です。
