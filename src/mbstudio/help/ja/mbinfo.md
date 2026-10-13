# mbinfo — スワスデータファイルの統計と範囲を表示

`mbinfo` は 1 つのスワスデータファイル、またはデータリストが参照するすべてのファイルを読み込み、データ数、航跡の集計、最初と最後のレコード、各値の最小・最大と地理的範囲などの基本統計を表示します。処理の初期段階でファイルの内容を確認するときや、ほかの MB-System プログラムが参照する ".inf" メタデータファイルを作成するときに使います。

## 書式

    mbinfo
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --comments {-C}
        --debug-record-types
        --debug-record-contents=identifier
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format_id { -Fformat_id}
        --good-nav {-G}
        --help {-H}
        --input=[file | datalist] {-Ifile}
        --longitude-domain=lonflip {-Llonflip}
        --mask-dimensions=londim/latdim[/west/east/south/north] {-Mlondim/latdim[/west/east/south/north]}
        --notices {-N}
        --output-file {-O}
        --output-format=format_id {-Xformat_id}
        --ping-variances=number {-Ppings}
        --quick {-Q}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --use-feet {-W}
        --verbose {-V}

## 説明

レポートには、ファイル名、MBIO フォーマット ID とフォーマットの説明、`mbprocess` が書き込んだメタデータ（ソナー機種、船名、バイアスパラメータなど）、および水深・アンプリチュード・サイドスキャンの値の個数が含まれます。データ種別ごとに、有効値・ゼロ値・フラグ付き値の個数と割合が示されます。「有効値」とはゼロでもなく、不良としてフラグも付いていない値です。さらに、総時間、航跡長、平均速力、最初と最後のレコードの時刻と位置、経度・緯度・水深・高度・アンプリチュード・サイドスキャンの最小値と最大値が表示されます。

`mbinfo` の重要な用途は ".inf" ファイルの作成です。`mbgrid` や `mbmosaic` などのプログラムは、各スワスファイルの ".inf" ファイルがあればそれを読み、対象範囲にデータを含まないファイルを読み飛ばします。`-O` オプションを付けると、レポートは画面ではなく、入力ファイル名に ".inf" を付けた名前のファイルに書き出されます。データリスト内のすべてのファイルについて ".inf" ファイルを作るには `mbdatalist -N` または `mbdatalist -O` を使います。

`-X` オプションでレポートの形式を変更できます。`-X1` は JSON、`-X2` は XML を出力します。`-O` と組み合わせると、出力ファイル名の接尾辞は "_inf.json" または "_inf.xml" になります。

入力がデータリスト（フォーマット -1、たとえば ".mb-1" で終わるファイル）の場合、参照されるすべてのファイルを合算した統計が表示されます。フォーマットを指定しない場合、`mbinfo` はファイル名の接尾辞からフォーマットを推定します（たとえば ".mb58" はフォーマット 58）。

`-P` を指定すると分散計算を行います。一定数のピングごとにまとめ、ビームごと（水深はトレンド除去後）およびピクセルごとの分散を出力します。これはスワス幅方向のノイズの大まかな指標になります。`-M` を指定すると、データの範囲内のどこにデータがあるかを 0 と 1 の表で示す粗いカバレッジマスクが追加されます。データリストを読む場合と `-Q` を使う場合は分散計算は行われません。

## 主なオプション

- `-Ifile` (`--input=file`) — 入力スワスデータファイルまたはデータリスト。既定値: 標準入力。
- `-Fformat_id` (`--format=format_id`) — 入力の MBIO フォーマット ID。負の値 (-1) は入力がデータリストであることを示します。既定値: ファイル名の接尾辞から推定。
- `-O` (`--output-file`) — レポートを画面ではなく、入力ファイル名に ".inf" を付けたファイルに書き出します（`-X1` / `-X2` のときは ".inf" が "_inf.json" / "_inf.xml" になります）。
- `-Xformat_id` (`--output-format=format_id`) — レポート形式: 0 = テキスト（既定）、1 = JSON、2 = XML。
- `-N` (`--notices`) — 検出されたデータレコード種別、致命的でない読み込みエラー、ゼロ航法や過大な水深などのデータの問題を追加表示します。
- `-G` (`--good-nav`) — 経度・緯度の範囲を計算するとき、ゼロの位置や非常に大きな速力を示す位置を除外します。作図範囲を適切に求めるのに便利です。
- `-Mlondim/latdim[/west/east/south/north]` (`--mask-dimensions=...`) — londim × latdim のビンによるカバレッジマスクを追加します。範囲を省略するとデータを 2 回読み込みます。
- `-Ppings` (`--ping-variances=number`) — この数のピングごとにビームとピクセルの分散を計算します。既定値: 1（分散計算なし）。
- `-Q` (`--quick`) — スワスファイルではなく既存の ".inf" ファイルから統計を読み込みます。このモードでは分散、通知、マスクは出力できません。
- `-C` (`--comments`) — データ中のコメントレコードを表示します。
- `-Rwest/east/south/north` (`--bounds=...`) — この経度・緯度範囲内のデータだけを使います。既定値: -360/360/-90/90。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — この時間内のデータだけを使います。開始時刻が終了時刻より後の場合は、逆に 2 つの時刻の間のデータが除外されます。
- `-Llonflip` (`--longitude-domain=lonflip`) — 出力する経度の範囲: -1 = -360～0、0 = -180～180、1 = 0～360。既定値: 最初のデータから自動選択。
- `-Sspeed` (`--speed-minimum=speed`) — 船速がこの値 (km/h) 未満のピングを無視します。既定値: 0。
- `-Ttimegap` — ピング間の時間間隔がこの値（分）を超えるとデータギャップとみなします。既定値: 1。
- `-W` (`--use-feet`) — 水深をメートルではなくフィートで表示します。
- `-V` (`--verbose`) — プログラムのバージョンや読み込みエラーメッセージを含む詳細出力を行います。
- `-H` (`--help`) — 簡単な説明と使い方を表示して終了します。

## 使用例

Kongsberg の 1 ファイルの統計を表示します（フォーマットは接尾辞から判定されます）。

    mbinfo -I 0001_20250101_000000.mb58

レコード種別の通知と 10 × 10 のカバレッジマスクを含む ".inf" ファイルをデータファイルの隣に作成します。

    mbinfo -I 0001_20250101_000000.mb58 -N -M10/10 -O

データリスト内の全ファイルを合算した統計を、適切な範囲で表示します。

    mbinfo -I datalist.mb-1 -G

ファイルのメタデータを JSON で書き出します（"line1.mb88_inf.json" が作成されます）。

    mbinfo -I line1.mb88 -X1 -O

## 注意

- 空白を含むパスは引用符で囲んでください。例: `-I "D:\Survey 2025\line1.mb58"`。
- `-O` で作成される ".inf" ファイルは、カレントディレクトリではなく入力ファイルと同じ場所（入力パスに接尾辞を付けた名前）に書き出されます。
- 時間ギャップは短い形式 `-T` で指定してください。このバージョンでは長い形式 `--time-gap` が正しく認識されません。
- `--debug-record-types` と `--debug-record-contents=identifier` は、レコード種別の一覧やレコード内容のダンプを行う開発者向けオプションです。

## 関連項目

`mbdatalist`, `mbformat`, `mblist`, `mbprocess`, `mbgrid`, `mbmosaic`, `mbset`。完全なリファレンスは MB-System のマニュアルページ mbinfo(1)（英語）です。
