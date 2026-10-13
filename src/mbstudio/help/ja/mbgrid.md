# mbgrid — スワスデータから水深・反射強度・サイドスキャンをグリッド化

`mbgrid` は、データリストに記載されたスワスソナーファイルの水深、地形、ビーム反射強度、またはサイドスキャンデータをグリッド化し、GMT グリッド（または他のグリッド形式）として書き出します。処理済みの MB-System データから数値地形モデルを作成する標準的な方法で、通常は編集と `mbprocess` の後に実行します。作成したグリッドは作図、`gmt mbgrdtiff`、`mbgrd2gltf`、GIS への書き出しなどに使用します。

## 書式

    mbgrid
        --algorithm=mode[/threshold] {-Fmode[/threshold]}
        --background-data=background {-Kbackground}
        --border=border {-Bborder}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --bounds=factor {-Rfactor}
        --clip=clip[/mode] {-Cclip[/mode]}
        --data-type=datatype {-Adatatype}
        --dimensions=xdim/ydim {-Dxdim/ydim}
        --extend=extend {-Xextend}
        --extra-grids {-M}
        --gaussian-scale=scale {-Wscale}
        --help {-H}
        --input=filelist {-Ifilelist}
        --longitude-domain=lonflip {-Llonflip}
        --output=root {-Oroot}
        --output-format=gridkind {-Ggridkind}
        --pings=pings {-Ppings}
        --projection=projection {-Jprojection}
        --shift=shiftx/shifty[/mode] {-Yshiftx/shifty[/mode]}
        --spacing=dx/dy/units[!] {-Edx/dy/units[!]}
        --speed-minimum=speed {-Sspeed}
        --tension=tension {-Ttension}
        --time-difference=time {-Utime}
        --use-feet {-Q}
        --use-nan {-N}
        --verbose {-V}

## 説明

入力はスワスファイルとそのフォーマットを記載したデータリスト（`-I`）です。水深の場合、各スワスファイルにコンパクトな `.fbt` ファイルがあればそちらを読み込みます。フォーマット 0 のデータリスト項目は経度・緯度・値の 3 列の ASCII ファイルとして読まれるため、通常の測深データも取り込めます。出力ファイル名は `-O` で指定したルートから作られます。グリッドは `root.grd`（ASCII 形式では `root.asc`、旧バイナリ形式では `root.grd1`）となり、実際にグリッドに寄与したファイルのデータリストが `root.mb-1` に書き出されます。

範囲（`-R`）またはグリッドの大きさ（`-D` または `-E`）を指定しない場合、`mbgrid` は全データを含む範囲と、最大ソナー高度の 0.02 倍のセルサイズを使います。この範囲は各スワスファイルの `.inf` ファイルから求められ、存在しないか古い `.inf`、`.fbt`、`.fnv` ファイルは先に自動生成されます（`mbdatalist -N` と同じ処理）。範囲を明示した場合は、`.inf` ファイルからグリッド内にデータがないと判断されるファイルは読み飛ばされるため、1 つのマスターデータリストを効率よく使い続けられます。

`-F` で 8 種類のグリッド化アルゴリズムを選べます: 1 ガウス加重平均（既定）、2 メディアンフィルタ、3 最小値フィルタ、4 最大値フィルタ、5 傾斜を考慮したビームフットプリント、6 ビームフットプリント、および最小・最大加重平均（`-F3/threshold`、`-F4/threshold`。セル内の最浅または最深の測深から `threshold` メートル以内の測深のみを平均）です。編集済みの水深には通常ガウス加重平均を、未編集の水深、反射強度、サイドスキャンにはメディアンフィルタを使います。フットプリント法（5、6）は水深専用で、ビームフットプリントより小さいセルでのグリッド化に適しています。水路測量では最浅測深を残すために最小値フィルタがよく使われます。

データで埋まらないセルは、薄板スプライン補間（`-C`、`-T`）や、水深の場合は背景グリッド（`-K`）で埋めることができます。空のセルは 99999.9、`-N` を指定すると NaN として書き出されます（GMT や GIS で使う場合は NaN を推奨）。既定ではグリッドは地理座標（WGS84 の経度・緯度）です。`-J` を指定すると UTM などの投影座標系（東距・北距、メートル）のグリッドを作成します。

## 主なオプション

- `-Ifilelist` — 入力ファイルのデータリスト（`--input`）。既定値: `datalist.mb-1`。
- `-Oroot` — 出力ファイル名のルート（`--output`）。既定値: `grid`。
- `-Adatatype` — グリッド化するデータ（`--data-type`）: 1 水深（下向き正、既定）、2 地形（上向き正）、3 ビーム反射強度、4 サイドスキャン。
- `-Edx/dy/units[!]` — セルサイズ（`--spacing`）。単位は `meters`（既定）、`km`、`feet`、`degrees`、`arcmin`、`arcsec`。`dy` を 0 にすると正方形のセルになります。末尾に `!` を付けると範囲を調整して正確なセルサイズを保ちます。
- `-Dxdim/ydim` — セルサイズの代わりにグリッドの節点数を指定します（`--dimensions`）。既定値: 101/101。
- `-Rwest/east/south/north` — グリッドの範囲（度）。`-J` で投影を完全に指定した場合は東距・北距でも指定できます（`--bounds`）。`-Rfactor`（1 より大きい値）は自動で求めた範囲を拡大します（例: `-R1.1`）。
- `-Fmode[/threshold]` — グリッド化アルゴリズム（`--algorithm`）。上記参照。既定値: 1。
- `-Cclip[/mode]` — データから `clip` セル以内の空セルをスプライン補間します（`--clip`）。mode 1（既定）は両側にデータがある隙間だけを埋め、mode 2 はデータから `clip` セル以内のすべてのセルを埋め、mode 3 はグリッド全体を埋めます。既定値: 補間なし。
- `-Ttension` — スプライン補間のテンション（`--tension`）。0 は最小曲率解です。既定値: 0。
- `-Wscale` — セルサイズに対するガウス重み関数の幅（`--gaussian-scale`）。既定値: 1.0。
- `-N` — 空のセルを 99999.9 ではなく NaN で書き出します（`--use-nan`）。
- `-Jprojection` — 投影座標系で出力します（`--projection`）。`-JU` は UTM ゾーンを自動選択、`-JUTM32N` / `-JUTM56S` はゾーンを指定、`-Jepsg32632` は EPSG コードを使用します。
- `-Ggridkind` — 出力グリッド形式（`--output-format`）。既定値: GMT netCDF 浮動小数点（`nf`）。`bf`、`nd`、`ef`（ESRI ASCII）などの GMT 形式 ID や、完全な形式 `=id/scale/offset/nan` も使えます。旧来の数値 ID 1（ASCII）、2（旧 GMT バイナリ）、3（netCDF）、4（Arc/Info ASCII グリッド）も受け付けます。ESRI ASCII グリッドでは x と y のセルサイズを等しくする必要があります。
- `-M` — `root_num.grd`（セルごとのデータ数）と `root_sd.grd`（セルごとの標準偏差）も書き出します（`--extra-grids`）。最小値・最大値フィルタでは無視されます。
- `-Utime` — 重なり合うスワスを平均しません（`--time-difference`）。各セルで最初のデータから `time` 分以上離れたデータを無視し、負の値では最後のデータを残します。主に反射強度とサイドスキャン用です。
- `-Xextend` — 内部の作業グリッドを各辺でグリッドサイズの `extend` 倍だけ広げ、グリッド外のデータで縁の補間を導きます（`--extend`）。既定値: 0。
- `-Kbackground` — 残った空セルを背景 GMT グリッドファイルで埋めます（`--background-data`）。
- `-Bborder` — 補間したグリッドの空の縁セルを値 `border` に設定します（`--border`）。
- `-Q` — 水深・地形をメートルではなくフィートでグリッド化します（`--use-feet`）。
- `-Llonflip` — 経度範囲: -1、0、1（`--longitude-domain`）。
- `-Ppings`、`-Sspeed` — 入力データのピング平均と最低速度（km/h）（`--pings`、`--speed-minimum`）。
- `-Yshiftx/shifty[/mode]` — グリッド範囲（mode 1、既定）またはすべての入力データ（mode 2）を東・北へ指定メートルだけずらします（`--shift`）。
- `-V` — 詳細表示（`--verbose`）。`-H` は使用法を表示します（`--help`）。

## 使用例

データリストの水深を、全データ範囲で 5 m セルにグリッド化し、空セルを NaN にします。

    mbgrid -Idatalist.mb-1 -Osurvey_bathy -E5/5/meters -N -V

2 m セルの UTM 地形グリッドを作成し、小さな隙間をスプライン補間で埋め、範囲を 10 % 広げます。

    mbgrid -Idatalist.mb-1 -Osurvey_utm -A2 -JU -E2/2/meters -R1.1 -C3/1 -N

指定範囲を最小値フィルタでグリッド化し、10 m セルごとに最浅測深を残します。

    mbgrid -Idatalistp.mb-1 -Ominimum_depth -R139.65/139.90/35.20/35.45 -E10/10/meters -F3 -N

重なり合う測線では各セルで最初のデータだけを使って、サイドスキャンをメディアンフィルタでグリッド化します。

    mbgrid -Idatalistp.mb-1 -Osidescan -A4 -F2 -E1/1/meters -U10 -N

## 注意

- Windows では、通常 `mbm_grdplot` マクロで作成されるプロットスクリプト（`root.grd.cmd`）は作成されません。グリッドは MB-Studio で表示するか、`gmt mbgrdtiff` などの GMT で作図してください。
- `-O` にパスを含めない限り、出力ファイルは現在のディレクトリに書き出されます。空白を含むパスは引用符で囲みます（例: `-O"D:\Survey 2025\grids\area1"`）。
- 処理済みデータをグリッド化してください。処理済みファイルを記載したデータリスト（`$PROCESSED` を含むもの、例: `mbdatalist -Z` で作成）を使うと、編集や補正が反映されます。
- 大きな clip 値と clip mode 1 の組み合わせは非常に時間がかかります。

## 関連項目

`mbdatalist`、`mbinfo`、`mbprocess`、`mbmosaic`、`gmt mbgrdtiff`、`gmt mbswath`、`mbgrd2gltf`、`mbmakedatalist`。完全なリファレンスは MB-System のマニュアルページ mbgrid(1)（英語）です。
