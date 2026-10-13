# mblist — スワスデータの任意の値をテキストまたはバイナリの表で出力

`mblist` はスワスデータファイルを読み込み、選択した値（時刻、位置、水深、姿勢、アンプリチュード、サイドスキャンなど多数）を表形式で画面またはファイルに出力します。センタービームのプロファイル、航法、あるいは他のソフトウェア用の「経度 緯度 水深」の測深点を取り出すときや、処理中に個々のビームやピクセルを確認するときに使います。

## 書式

    mblist
        --beam-range=beam_start/beam_end|A|Xexcludepercent {-Mbeam_start/beam_end}
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --binary {-A}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --check-mode=check {-Ucheck}
        --decimate=decimate {-Kdecimate}
        --delimiter=delimiter {-Gdelimiter}
        --dump-mode=dumpmode {-Ddumpmode}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format {-Fformat}
        --help {-H}
        --input=file {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --netcdf {-C}
        --no-value-check {-Q}
        --output-file=outfile {-Xoutfile}
        --output-format=output_format {-Ooutput_format}
        --ping-average=pings {-Ppings}
        --pixel-range=pixel_start/pixel_end|A {-Npixel_start/pixel_end}
        --projection=projection {-Jprojection}
        --secondary-file=secondaryfile {-Ysecondaryfile}
        --segment=segment {-Zsegment}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --use-feet {-W}
        --verbose {-V}

## 説明

既定では、`mblist` はピングごとに 1 行を、タブ区切りの列で出力します。列は `-O` で選択します。これは 1 文字ごとに 1 つの値を指定する文字列で、既定値は `TXYHSLZ`（時刻文字列、経度、緯度、船首方位、速力、累積航走距離、地形高）です。`-G` で別の区切り文字、`-A` でバイナリ出力（倍精度）、`-C` で netCDF CDL 出力を指定できます。

既定のモードでは、水深・アンプリチュード・サイドスキャンの値はソナー直下に最も近いビームとピクセルから取られ、位置はソナーの位置です。`-M` または `-N` でビームやピクセルの範囲を選ぶと、選んだビームやピクセルごとに 1 行が出力され、位置は海底上のビームやピクセルの位置になります。`-D` はすべてのビームまたはピクセルの「経度 緯度 値」を一括出力するショートカットで、グリッド作成プログラムへの入力に便利です。

既定ではフラグ付きの値と欠測値は出力されません。`-Q` または `-U` を使うと、フラグ付きの値（必要に応じて欠測値も）が出力されます。

よく使う `-O` の文字: `T` 時刻 yyyy/mm/dd/hh/mm/ss、`t` 時刻 yyyy mm dd hh mm ss、`M` Unix 時刻（10 進秒）、`X` 経度、`Y` 緯度、`Z` 地形高（上向き正）、`z` 水深（下向き正）、`H` 船首方位、`S` 速力 (km/h)、`L` 航走距離 (km)、`l` 航走距離 (m)、`D` 横方向距離 (m)、`E` ビームの前後方向距離 (m)、`B` アンプリチュード、`b` サイドスキャン、`C` ソナー高度、`c` ソナー深度、`R` ロール、`P` ピッチ、`r` ヒーブ、`F` 数値のビームフラグ、`f` ビームフラグ文字、`#` ビームまたはピクセル番号、`N` ピング番号。接頭文字は次の値を修飾します: `-` 符号反転、`/` 逆数、`^`（`X` または `Y` の前）`-J` で指定した投影（既定は UTM）での東距・北距、`=` と `+` 左舷端・右舷端の有効ビームの値。`,` で始まるコード（往復時間、角度、ビームレンジ）や `.` で始まるコード（フォーマット固有の生の値、ファイル名、セカンダリファイルの列）はマニュアルページを参照してください。特別な文字列 `%fnv` は ".fnv" 高速航法ファイル用の列（`tMXYHScRPr=X=Y+X+Y`）を選択します。

## 主なオプション

- `-Ifile` (`--input=file`) — 入力スワスファイルまたはデータリスト。既定値: datalist.mb-1。
- `-Fformat` (`--format=format`) — MBIO フォーマット ID。データリストは -1。既定値: ファイル名の接尾辞から推定。
- `-Ooutput_format` (`--output-format=output_format`) — 出力列を選ぶ文字列（上記参照）。既定値: `TXYHSLZ`。
- `-Mbeam_start/beam_end` (`--beam-range=...`) — この範囲のビーム（左舷側から 0 で始まる番号）についてレコードを出力します。`-MA` はすべてのビーム、`-MXpercent` は外側 percent % を除くすべてのビームを出力します。
- `-Npixel_start/pixel_end` (`--pixel-range=...`) — この範囲のサイドスキャンピクセルについてレコードを出力します。`-NA` はすべてのピクセルを出力します。
- `-Ddumpmode` (`--dump-mode=dumpmode`) — 一括出力: 1 = `-O` の形式（既定）、2 = 経度 緯度 水深、3 = 経度 緯度 地形高、4 = 経度 緯度 アンプリチュード、5 = 経度 緯度 サイドスキャン。`-O`、`-M`、`-N` より優先されます。
- `-Ucheck` (`--check-mode=check`) — フラグ付き値と欠測値の扱い: 0 = 出力しない（既定）、1 = フラグ付き値を出力し欠測値は除外、2 = フラグ付き値を出力し欠測値はゼロ、3 = フラグ付き値を出力し欠測値は NaN、4 = フラグ付き値と欠測値を NaN で出力。
- `-Q` (`--no-value-check`) — すべての値を出力します。フラグ付き値はそのまま、欠測値はゼロになります（`-U2` と同じ）。
- `-Xoutfile` (`--output-file=outfile`) — 画面ではなくこのファイルに出力します。
- `-Gdelimiter` (`--delimiter=delimiter`) — テキスト出力の列区切り文字。既定値: タブ。
- `-A` (`--binary`) — テキストではなく倍精度のバイナリ値を出力します。
- `-Jprojection` (`--projection=projection`) — `^X` / `^Y` に使う投影座標系。例: `UTM12N`、`epsg32612`、または最初のデータからゾーンを決める UTM を表す `U`。
- `-Kdecimate` (`--decimate=decimate`) — decimate 個ごとに 1 レコードだけ出力します。既定値: 1。
- `-Ppings` (`--ping-average=pings`) — この数のピングを平均して 1 つにします。既定値: 1。
- `-Rwest/east/south/north` (`--bounds=...`) — この範囲内のデータだけを使います。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — この時間内のデータだけを使います。
- `-Zsegment` (`--segment=segment`) — 入力ファイルごとのテキスト出力を segment を含む行で区切ります（GMT のマルチセグメントファイルには `>`）。`swathfile` または `datalist` を指定すると、"#" に続けて元のファイルまたはデータリストのパスが書かれます。
- `-Ysecondaryfile` (`--secondary-file=secondaryfile`) — 1 列目が Unix 時刻、続く最大 19 列がデータのテキストファイル。値はピング時刻に内挿され、`-O` の `.1C` ～ `.19C` で出力できます。
- `-W` (`--use-feet`) — 水深と距離をフィートで出力します。
- `-C` (`--netcdf`) — netCDF CDL を出力します。`-A` と併用すると `ncgen` でバイナリ netCDF ファイルに変換します。
- `-V` (`--verbose`), `-H` (`--help`) — 詳細メッセージ表示、使い方を表示して終了。

## 使用例

各ピングのセンタービームの時刻・位置・水深をテキストファイルに書き出します。

    mblist -I 0001_20250101_000000.mb58 -OtXYz -X centre.txt

データリスト内の有効な全測深点の経度・緯度・水深を出力します。

    mblist -I datalist.mb-1 -D2 -X soundings.xyz

1 ファイルの全ビームについて、フラグ付きビームも含めてビーム番号・ビームフラグ文字・水深を出力します。

    mblist -I line1.mb88 -MA -O#fz -U1 -X beams.txt

UTM の東距・北距と水深をカンマ区切りで出力します（`^` は cmd.exe で特別な意味を持つため、形式文字列を引用符で囲みます）。

    mblist -I datalist.mb-1 -MA "-O^X^Yz" -JU -G, -X soundings_utm.csv

## 注意

- cmd.exe では `^`、`>`、`|`、`&`、`%` が特別な意味を持ちます。これらを含む引数は `"-O^X^Yz"` や `"-Z>"` のように二重引用符で囲んでください。バッチファイルでは `%fnv` の代わりに `%%fnv` と書きます。
- `-X` を指定しないと出力は画面に表示されます。保存するには `-X` を使うか、`> file.txt` でリダイレクトしてください。
- バイナリ netCDF 出力（`-C -A`）には netCDF の `ncgen` プログラムが PATH 上に必要です。結果は `-X` で指定したファイル（指定がなければ mblist.nc）に書き出されます。

## 関連項目

`mbinfo`, `mbnavlist`, `mbdatalist`, `mbformat`, `mbprocess`, `mbgrid`。完全なリファレンスは MB-System のマニュアルページ mblist(1)（英語）です。
