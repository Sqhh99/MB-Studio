# mbhistogram — 水深・アンプリチュード・サイドスキャン値のヒストグラム

`mbhistogram` はスワスデータを読み込み、水深、ビームアンプリチュード、またはサイドスキャンの値のヒストグラムを計算します。ヒストグラムの代わりに、データの分布を等しい大きさに分割する値の組を出力することもでき、アンプリチュードやサイドスキャン画像用のヒストグラム平坦化カラーテーブルの作成に役立ちます。

## 書式

    mbhistogram [-Akind -Byr/mo/da/hr/mn/sc -Dmin/max -Eyr/mo/da/hr/mn/sc -Fformat -G -Ifile -Llonflip -Mnintervals -Nnbins -Ppings -Rw/e/s/n -Sspeed -V -H]
        --begin-time=yr/mo/da/hr/mn/sc {-Byr/mo/da/hr/mn/sc}
        --bins=nbins {-Nnbins}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --data-type=kind {-Akind}
        --end-time=yr/mo/da/hr/mn/sc {-Eyr/mo/da/hr/mn/sc}
        --format=format_id {-Fformat_id}
        --gaussian {-G}
        --help {-H}
        --input=[file | datalist] {-Ifile}
        --intervals=nintervals {-Mnintervals}
        --longitude-domain=lonflip {-Llonflip}
        --pings=pings {-Ppings}
        --speed-minimum=speed {-Sspeed}
        --time-gap=timegap {-Ttimegap}
        --value-range=min/max {-Dmin/max}
        --verbose {-V}

## 説明

データの種類は `-A` で選びます: 0 = 水深、1 = ビームアンプリチュード、2 = サイドスキャン（既定）。ヒストグラムは `-D` で指定した範囲を対象とし、`-N` で指定した数のビンに分割されます。min の値が最初のビンの中心、max の値が最後のビンの中心になり、ビンの範囲外の値は無視されます。各出力行はビンの中心値とそのビンに入る値の個数です。結果は画面に出力されます。

`-M` を指定すると、ヒストグラムの代わりに、分布を等しい大きさの領域に分ける指定個数の値を出力します（最初と最後の値は、`-D` の範囲内に制限したデータの最小値と最大値です）。これらの値をカラーテーブルの境界に使うと、各色が同じ数のデータ値を受け持つようになります（ヒストグラム平坦化）。`-G` を指定すると、出力や区間の計算の前にヒストグラムをガウス分布に変換します。これは GMT プログラム grdhisteq のガウスオプションに似ており、画像の陰影付けに適しています。線形の形式はグレースケールのアンプリチュード画像やサイドスキャン画像に適しています。

## 主なオプション

- `-Ifile` (`--input=file`) — 入力スワスファイルまたはデータリスト。既定値: 標準入力。
- `-Fformat_id` (`--format=format_id`) — MBIO フォーマット ID。データリストは -1。既定値: ファイル名の接尾辞から推定。
- `-Akind` (`--data-type=kind`) — 使用するデータ: 0 = 水深、1 = ビームアンプリチュード、2 = サイドスキャン。既定値: 2。
- `-Dmin/max` (`--value-range=min/max`) — 最初と最後のビンの中心値。既定値: 0/128。
- `-Nnbins` (`--bins=nbins`) — ビンの数。既定値: 16、`-M` 使用時は区間数の 50 倍。
- `-Mnintervals` (`--intervals=nintervals`) — ヒストグラムの代わりに、分布を等分するこの個数の値を出力します。
- `-G` (`--gaussian`) — ヒストグラムをガウス分布に変換します。
- `-Ppings` (`--pings=pings`) — この数のピングを平均して 1 つにします。既定値: 1。
- `-Rwest/east/south/north` (`--bounds=...`) — この範囲内のデータだけを使います。
- `-Byr/mo/da/hr/mn/sc`, `-Eyr/mo/da/hr/mn/sc` (`--begin-time=...`, `--end-time=...`) — この時間内のデータだけを使います。
- `-Llonflip` (`--longitude-domain=lonflip`) — 経度の範囲: -1 = -360～0、0 = -180～180、1 = 0～360。
- `-Sspeed` (`--speed-minimum=speed`), `-Ttimegap` (`--time-gap=timegap`) — 読み込み時の最低速力 (km/h) と最大時間ギャップ（分）。
- `-V` (`--verbose`), `-H` (`--help`) — 詳細メッセージ表示、使い方を表示して終了。

## 使用例

ファイルのサイドスキャン値について、0 から 45000 までを 25 ビンとしたヒストグラムを求めます。

    mbhistogram -I line1.mb41 -D0/45000 -N25

データリスト内の全ファイルについて、0 から 500 m までを 10 m ビンとした水深のヒストグラムを求めます。

    mbhistogram -I datalist.mb-1 -A0 -D0/500 -N51 > depth_hist.txt

カラーテーブル用に、アンプリチュードの分布を等分する 16 個の値を求めます。

    mbhistogram -I datalist.mb-1 -A1 -D0/255 -M16

## 注意

- `-D` は実際のデータ範囲を含むように選んでください（`mbinfo` が表示する最小値と最大値を参照）。既定の 0/128 では多くの値がビンの範囲外になることがあります。
- 入力は必ず `-I` で指定してください。既定は標準入力からの読み込みですが、実用的な場面はほとんどありません。

## 関連項目

`mbinfo`, `mblist`, `mbdatalist`, `mbgrid`, `mbmosaic`, `mbbackangle`。完全なリファレンスは MB-System のマニュアルページ mbhistogram(1)（英語）です。
