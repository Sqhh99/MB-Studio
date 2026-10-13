# mbmosaic — スワスデータから反射強度・サイドスキャンのモザイクを作成

`mbmosaic` は、データリストに記載されたスワスファイルのビーム反射強度またはサイドスキャン後方散乱から、グリッド化したモザイクを作成します。各サンプルに見かけのグレージング角、ルック方位、プラットフォーム方位による優先度を付けられるため、たとえば鏡面反射の強い直下域を抑えたり、特定の方向から見たモザイクを作成したりできます。通常は `mbbackangle` と `mbprocess` で後方散乱を補正した後に実行します。

## 書式

    mbmosaic
        --altitude-default=bathdef {-Zbathdef}
        --border=border {-Bborder}
        --bounds=west/east/south/north {-Rwest/east/south/north}
        --bounds=factor {-Rfactor}
        --data-type=datatype {-Adatatype}
        --directional-priority=bearing/factor[/mode] {-Ubearing/factor[/mode]}
        --extend=extend {-Xextend}
        --extra-grids {-M}
        --grid-dimensions=xdim/ydim {-Dxdim/ydim}
        --grid-format=gridkind {-Ggridkind}
        --grid-spacing=dx/dy/units {-Edx/dy/units}
        --help {-H}
        --input=filelist {-Ifilelist}
        --longitude-domain=lonflip {-Llonflip}
        --output=root {-Oroot}
        --pings=pings {-Ppings}
        --priority-range=priority_range[/weight] {-Fpriority_range[/weight]}
        --priority-source=priority_source {-Ypriority_source}
        --projection=projection {-Jprojection}
        --speed-minimum=speed {-Sspeed}
        --spline-interpolation=clip/mode/tension {-Cclip/mode/tension}
        --topography-grid=topogridfile {-Ttopogridfile}
        --use-nan {-N}
        --verbose {-V}
        --weighting-scale=scale {-Wscale}

## 説明

入力データリスト（`-I`）と出力ルート（`-O`）の扱いは `mbgrid` と同じです。モザイクは `root.grd`（ASCII 形式・旧バイナリ形式では `root.asc` / `root.grd1`）に、寄与したファイルのデータリストは `root.mb-1` に書き出されます。範囲やグリッドの大きさを指定しない場合は、全データを含む範囲と最大ソナー高度の 0.02 倍のセルサイズが使われます。範囲は `.inf` ファイルから求められ、存在しないか古い場合は自動生成されます。水深はモザイク化できないので、`mbgrid` でグリッド化してください。

各サンプルの優先度は、最大 3 つの係数の積です。見かけのグレージング角に応じた優先度（`-Y`、既定のテーブルまたはファイル）、ルック方位の優先度、プラットフォーム方位の優先度（`-U`）です。既定では各セルは最も優先度の高いサンプル 1 つの値をとります。`-F` を指定すると、セル内の最高優先度から `priority_range` 以内の優先度を持つすべてのサンプルのガウス加重平均を使います。オプションの `weight` を指定すると、各サンプルをさらに優先度（1）または優先度の 2 乗（2）で重み付けします。

グレージング角は横断距離 x とソナー高度 z から求めた角度 arctan(x/z) で、直下で 0、左舷で負、右舷で正となります。優先度ファイルは角度（度）と優先度（0～1）の 2 列で、左舷端の角度から右舷端の角度へ順に並べます。間の角度では優先度が線形補間され、テーブルの範囲外ではゼロになります。

空のセルは薄板スプライン補間（`-C`）で埋めることができます。空のセルは 99999.9、`-N` を指定すると NaN として書き出されます。`mbgrid` と同様に、`-J` で UTM などの投影座標系のモザイクを作成できます。

## 主なオプション

- `-Ifilelist` — 入力ファイルのデータリスト（`--input`）。既定値: `datalist.mb-1`。
- `-Oroot` — 出力ファイル名のルート（`--output`）。既定値: `grid`。
- `-Adatatype` — モザイク化するデータ（`--data-type`）: 3 ビーム反射強度、4 サイドスキャン（既定）、5 平坦海底のグレージング角、6 横断方向グレージング角、7 横断方向傾斜。`F` を付けると（例: `-A4F`）、`mbfilter` でフィルタ処理したデータ（`.ffa`/`.ffs` ファイル）を使います。
- `-Edx/dy/units` — セルサイズ（`--grid-spacing`）。単位は `meters`（既定）、`km`、`feet`、`degrees`、`arcmin`、`arcsec`。末尾に `!` を付けると範囲を調整して正確なセルサイズを保ちます。
- `-Dxdim/ydim` — セルサイズの代わりにグリッドの節点数を指定します（`--grid-dimensions`）。既定値: 101/101。
- `-Rwest/east/south/north` — モザイクの範囲（`--bounds`）。`-Rfactor`（1 より大きい値）は自動で求めた範囲を拡大します。
- `-Ypriority_source` — グレージング角による優先度（`--priority-source`）。1～8 の数値で既定のテーブルを選ぶか（1～4 は ±60、67、75、85 度までの外側スワスを優先、5～8 は直下を優先）、優先度ファイル名を指定します。
- `-Ubearing/factor[/mode]` — 方向による優先度（`--directional-priority`）。mode 0（既定）はルック方位 `bearing` を、mode 1 はプラットフォーム方位 `bearing` を優先します。優先度は cos(factor × 差) で、差が 90 度 / factor を超えるとゼロです。
- `-Fpriority_range[/weight]` — 最高優先度から `priority_range` 以内のサンプルのガウス加重平均を使います（`--priority-range`）。既定値: 最高優先度のサンプルのみ。
- `-Wscale` — セルサイズに対するガウス重み関数の幅（`--weighting-scale`）。既定値: 1.0。
- `-Cclip/mode/tension` — スプライン補間（`--spline-interpolation`）。データから `clip` セル以内の空セルを埋めます。mode 1 は隙間、mode 2 はデータ近傍のすべてのセル、mode 3 はグリッド全体を埋めます。`tension` 0 は最小曲率です。既定値: 補間なし。
- `-N` — 空のセルを 99999.9 ではなく NaN で書き出します（`--use-nan`）。
- `-Jprojection` — 投影座標系で出力します（`--projection`）。例: `-JU`、`-JUTM32N`、`-Jepsg32632`。
- `-Ggridkind` — 出力グリッド形式（`--grid-format`）。`mbgrid` と同じです。既定値: GMT netCDF 浮動小数点。
- `-Ttopogridfile` — 平坦海底の代わりに地形グリッドを使って各サンプルのフットプリントを計算します（`--topography-grid`）。
- `-Zbathdef` — 水深が得られない場合にグレージング角の計算に使う水深（`--altitude-default`）。既定値: 1000。
- `-M` — セルごとのサンプル数と標準偏差（`root_num.grd`、`root_sd.grd`）も書き出します。`-F` が必要です（`--extra-grids`）。
- `-Xextend` — 内部の作業グリッドを各辺でグリッドサイズの `extend` 倍だけ広げます（`--extend`）。
- `-Bborder`、`-Llonflip`、`-Ppings`、`-Sspeed` — 縁の値、経度範囲、ピング平均、最低速度（km/h）。`mbgrid` と同じです。
- `-V` — 詳細表示（`--verbose`）。`-H` は使用法を表示します（`--help`）。

## 使用例

外側スワスを優先し（既定テーブル 2）、最高優先度から 0.1 以内のサンプルをすべて平均して、1 m セルのサイドスキャンモザイクを作成します。

    mbmosaic -Idatalistp.mb-1 -Osidescan_mosaic -A4 -E1/1/meters -Y2 -F0.1 -N -V

優先度ファイルを使い、小さな隙間を補間で埋めて、UTM のビーム反射強度モザイクを作成します。

    mbmosaic -Idatalistp.mb-1 -Oamplitude_utm -A3 -JU -E2/2/meters -Yangle_priority.txt -F0.1 -C5/1/0 -N

東向きのサイドスキャンモザイク（優先ルック方位 90 度）を作成します。

    mbmosaic -Idatalistp.mb-1 -Ossmosaic_east -A4 -E2/2/meters -U90/2 -F0.05 -N

直下域の優先度を低く、45 度の優先度を最も高くする優先度ファイル `angle_priority.txt` の例です。

    -60.0 0.2
    -45.0 1.0
    -15.0 0.8
    -14.9 0.1
    14.9 0.1
    15.0 0.8
    45.0 1.0
    60.0 0.2

## 注意

- Windows では、通常 `mbm_grdplot` マクロで作成されるプロットスクリプト（`root.grd.cmd`）は作成されません。モザイクは MB-Studio または `gmt mbgrdtiff` で表示してください。
- 先にグレージング角の影響に対する後方散乱補正（`mbbackangle`、続いて `mbprocess`）を行い、処理済みファイルをモザイク化してください。
- 空白を含むパスは引用符で囲みます。

## 関連項目

`mbgrid`、`mbbackangle`、`mbprocess`、`mbfilter`、`mbdatalist`、`gmt mbgrdtiff`。完全なリファレンスは MB-System のマニュアルページ mbmosaic(1)（英語）です。
