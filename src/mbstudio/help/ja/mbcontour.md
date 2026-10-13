# mbcontour — スワスデータから等深線と航跡を描画

`mbcontour` は MB-System の GMT サプリメントに含まれるモジュールで、スワス水深をデータファイルから直接ピングごとに等深線化し、時刻、日付、ピング番号、ファイル名の注記付きで航跡を描画します。地図に等深線や注記付きの航跡を加えるとき（多くの場合 `mbswath` の画像の上に重ねます）や、調査の簡単な航跡図を作るときに使います。

## 書式

    gmt mbcontour -I<inputfile> -J<parameters> -R<west>/<east>/<south>/<north>
        [-A<cont_int>/<col_int>/<tick_int>/<label_int>/<tick_len>/<label_hgt>/<label_spacing>]
        [-B<tickinfo>] [-b<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-C<contourfile>] [-D<time_tick>/<time_annot>/<date_annot>/<time_tick_len>]
        [-e<year>/<month>/<day>/<hour>/<minute>/<second>]
        [-F<format>] [-G<name_hgt>[/<name_perp>]] [-L<lonflip>]
        [-M<pingnumber_tick>/<pingnumber_annot>/<pingnumber_tick_len>]
        [-N<nplot>] [-p<pings>] [-Q] [-S<speed>] [-T<timegap>] [-Z<algorithm>]
        [-K] [-O] [-P] [-U] [-V] [-X<x-shift>] [-Y<y-shift>]

## 説明

`mbcontour` は 1 つのスワスデータファイル、またはデータリストに記載されたすべてのファイルを読み込み、読み込んだピングのグループごとに水深を等深線化します。水深はメートル単位で下向き正です。不良フラグの付いたビームは使われません。

`-C` を指定しない場合、等深線は cont_int メートルごと（`-A`）に描かれ、col_int メートルごとに 4 色（黒、赤、緑、青）で色が変わります。tick_int の倍数の等深線には下り斜面を向くティックが付き、label_int の倍数の等深線にはラベルが付きます。`-C` を指定すると、等深線ファイルで深度値とそのスタイル・色を明示的に指定します。この場合 `-A` は無視されます。

等深線化のアルゴリズムは 2 種類あります（`-Z`）。既定のピング間アルゴリズムは高速ですが、あるピングのビームが前のピングのビームより後ろにある場所（急旋回、ピッチステアリング）では等深線の見た目が悪くなります。三角形アルゴリズムは測深点から三角形網を作って等深線化します。より良い等深線が得られますが、処理時間が測深数の 2 乗に比例して増えるため、一度に等深線化するピング数を少なくします（`-N`）。

航跡は `-D`（時刻ティックと注記）、`-M`（ピング番号ティック）、`-G`（各ファイルの先頭にファイル名）で描画します。`-A`、`-C`、`-D`、`-M`、`-Q` のいずれも指定しない場合は、等深線（既定の間隔）と航跡の両方が描かれます。`-A` または `-C` だけを指定した場合、航跡は描かれません。

データは `-R` の範囲を四方に 25% 広げた範囲内で読み込まれます。`mbcontour` はファイルを読む前にその ".inf" ファイル（`mbinfo` の出力）を確認し、範囲内にデータがなければそのファイルを読み飛ばします。

## 主なオプション

- `-I<inputfile>` — 描画するスワスデータファイルまたはデータリスト（必須）。".mb-1" で終わるファイルはデータリストとして読まれます。
- `-J<parameters>` — GMT の地図投影とサイズ。例: `-JM15c`。必須。
- `-R<west>/<east>/<south>/<north>` — 地図の範囲（度）。必須。
- `-A<cont_int>/<col_int>/<tick_int>/<label_int>/<tick_len>/<label_hgt>/<label_spacing>` — 等深線間隔、色の変更間隔、ティック間隔、ラベル間隔（メートル）、ティックの長さとラベルの高さ（インチ）、ラベルの最小間隔（インチ。0 = ラベルの高さから決定）。末尾の値を省略すると既定値のままになります。既定値: 25/100/100/100/0.05/0.1/0。
- `-C<contourfile>` — 等深線の深度値を 1 行に 1 つ記したテキストファイル: `level label tick red green blue`。label は `a`（注記あり）または `n`、tick は `t`（ティックあり）または `n`、red、green、blue は 0-255 です。有効な label と tick のない行は、ラベルとティックなしの黒で描かれます。GMT の CPT ファイルを指定すると、その色の境界が黒の等深線になります。
- `-D<time_tick>/<time_annot>/<date_annot>/<time_tick_len>` — 航跡を描画し、time_tick 時間ごとに時刻マーク、time_annot 時間ごとに時刻注記（hh:mm）、date_annot 時間ごとに日付注記（hh:mm/通日）を付けます。time_tick_len はマークの大きさ（インチ）です。既定値: 0.25/1/4/0.1。
- `-G<name_hgt>[/<name_perp>]` — 各ファイルの先頭にファイル名を高さ name_hgt インチの文字で注記します。name_perp = 1 では名前を航跡に直交する向きに書きます。航跡の描画も行います。既定の高さ: 0.1。
- `-M<pingnumber_tick>/<pingnumber_annot>/<pingnumber_tick_len>` — pingnumber_tick ピングごとに航跡にティックを付け、pingnumber_annot ピングごとにピング番号を注記します。ティックの長さはインチです。既定値: 50/100/0.1。
- `-Z<algorithm>` — 等深線化アルゴリズム: 0 = ピング間（既定）、1 = 三角形網。
- `-N<nplot>` — 等深線化を 1 回行う前に読み込むピング数。既定値: 50、`-Z1` では 5。
- `-Q` — 三角形アルゴリズム（`-Z1`）の三角形も描画し、データの分布を示します。
- `-p<pings>` — ピング平均: 1 = なし、n = n ピングを平均、0 = 自動。既定値: `mbdefaults` の設定。
- `-F<format>` — 入力の MBIO フォーマット ID（`-f` も同じ）。既定値: ファイル名の拡張子から判定。-1 はデータリストを表します。
- `-b<year>/<month>/<day>/<hour>/<minute>/<second>`、`-e<year>/<month>/<day>/<hour>/<minute>/<second>` — この時刻範囲のピングだけを使います。既定値: `mbdefaults` の設定。
- `-S<speed>` — 船速がこの値（km/h）未満のピングを読み飛ばします。既定値: `mbdefaults` の設定。
- `-T<timegap>` — データに途切れがあるとみなすピング間の時間間隔（分）。既定値: `mbdefaults` の設定。
- `-L<lonflip>` — 経度範囲: -1 = -360～0、0 = -180～180、1 = 0～360。既定値: `-R` から決定。
- `-B<tickinfo>` — GMT の地図枠と注記。例: `-Ba`。
- `-K`、`-O`、`-P`、`-X`、`-Y`、`-U`、`-V` — GMT クラシックモードのオプション: PostScript が続く、既存の図に重ねる、縦向き、作図原点の移動、タイムスタンプ、詳細な進行状況の表示。

## 使用例

既定の設定で等深線と航跡を描画します。

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Ba -P > contours.ps

三角形アルゴリズムを使い、10 m 間隔の等深線を描画し、50 m ごとに色の変更、ティック、ラベルを付けます。

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -A10/50/50/50/0.03/0.08 -Z1 -Ba -P > contours.ps

航跡だけを描画し、15 分ごとの時刻マーク、1 時間ごとの時刻注記、4 時間ごとの日付注記、航跡に直交するファイル名を付けます。

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -D0.25/1/4/0.1 -G0.08/1 -Ba -P > track.ps

levels.txt から読み込んだ深度値で等深線を描画し、図を PNG 画像に変換します。

    gmt mbcontour -Idatalist.mb-1 -JM15c -R-122.10/-121.85/36.70/36.85 -Clevels.txt -Ba -P > contours.ps
    gmt psconvert contours.ps -Tg -A

levels.txt の内容の例:

    100 a t 0 0 0
    150 n t 0 0 255
    200 a t 0 0 0

## 注意

- `mbcontour` は独立したプログラムではなく、MB-System の GMT サプリメントのモジュールです。常に GMT ドライバから `gmt mbcontour` として実行します。GMT はインストールフォルダの `bin\gmt_plugins` からサプリメントを自動的に見つけるため、GMT_CUSTOM_LIBS の設定は不要です。
- 使用例のように GMT クラシックモード（`-K`/`-O` と、`>` および `>>` による出力のリダイレクト）を使ってください。`mbswath` の画像に等深線を重ねるには、まず `gmt mbswath ... -K > map.ps` を実行し、次に同じ `-J` と `-R` で `gmt mbcontour ... -O -K >> map.ps` を実行します。モダンモードの 1 行形式（例: `-png map`）はこのモジュールでは認識されません。また、ここでの `-p` はピング平均を意味します。
- ラベル、ティック、ファイル名の大きさは、GMT の長さ単位の設定に関係なくインチで指定します。
- マニュアルページにある `-W` オプション（水深をフィートで扱う）はこのバージョンでは使えません。`-W` は受け付けられますが効果はありません。
- オプションの引数はオプション文字の直後に空白を入れずに書きます。空白を含むファイル名は引用符で囲んでください。

## 関連項目

`mbswath`、`mbgrdtiff`、`mbgrid`、`mbdatalist`、`mbinfo`、`mbnavlist`。完全なリファレンスは MB-System のマニュアルページ mbcontour(1)（英語）です。
