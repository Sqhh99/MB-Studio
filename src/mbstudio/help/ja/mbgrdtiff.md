# mbgrdtiff — GMT グリッドから地理参照付き GeoTIFF 画像を作成

`mbgrdtiff` は MB-System の GMT サプリメントに含まれるモジュールで、`mbgrid` で作成した水深グリッドや `mbmosaic` で作成したサイドスキャンモザイクなどの GMT グリッドを GeoTIFF 画像に変換します。グリッド値は GMT カラーパレット（CPT）ファイルで着色され、強度グリッドで陰影を付けることもできます。GIS や海図作成ソフトウェアで正しい位置に読み込める画像として調査結果を納品するときに使います。

## 書式

    gmt mbgrdtiff -I<grdfile> -C<cptfile> -J<parameters> -O<tiff_file>
        [-I<intensgrid>|<value>] [-M] [-N<nudge_x>/<nudge_y>]
        [-R<west>/<east>/<south>/<north>] [-V]
    gmt mbgrdtiff <grd_r> <grd_g> <grd_b> -J<parameters> -O<tiff_file> [options]

## 説明

最初の `-I` オプションで変換するグリッドを指定します。各グリッドセルが画像の 1 ピクセルとなり、その色は `-C` で指定した CPT ファイルから決まります。データのないセル（NaN）には CPT の NaN 色が使われます。CPT のすべての色がグレー（赤 = 緑 = 青）の場合、TIFF は 8 ビットグレースケール画像になり、それ以外は 24 ビットカラー画像になります。1 つのグリッドと CPT の代わりに、赤・緑・青の成分（0-255）を持つ 3 つのグリッドをファイル名だけで指定することもできます。

2 つ目の `-I` オプションで陰影を加えます。強度グリッド（例えば `gmt grdgradient` で作成した、データグリッドと同じ大きさのもの）または一定の強度値を指定します。

地理参照情報は GeoTIFF タグとして TIFF に書き込まれます。`mbgrdtiff` は `mbgrid` と `mbmosaic` がグリッドに記録した座標系を認識します。UTM ゾーンと EPSG の投影座標系は対応する投影座標系として書き込まれ、それ以外のグリッドはすべて地理座標（経度・緯度、WGS84）として扱われます。GeoTIFF タグを読まないソフトウェア用に、同じ名前で拡張子 ".tfw" のワールドファイルが常に TIFF の隣に書き出されます。

`-R` で画像をグリッドの一部に限定できます。`-N` は画像の地理参照位置を指定したメートル数だけずらします。例えば既知の測位オフセットを補正するときに使います。

## 主なオプション

- `-I<grdfile>` — 変換するグリッド（最初の `-I`）。r/g/b の 3 つのグリッドを指定する場合を除き必須です。
- `-I<intensgrid>|<value>` — 2 つ目の `-I`: 陰影に使う強度グリッド、または一定の強度値。
- `-C<cptfile>` — グリッド値を色に変換する GMT カラーパレットファイル。r/g/b の 3 つのグリッドを指定する場合を除き必須です。
- `-O<tiff_file>` — 出力 GeoTIFF ファイルの名前（必須）。".tif" で終わる名前を使ってください。ワールドファイル名は最後の 4 文字を ".tfw" に置き換えて作られます。
- `-J<parameters>` — GMT では投影の指定が必要です。`-JX15c` のような線形投影を使ってください。サイズは関係なく、画像は常にグリッドセル 1 つにつき 1 ピクセルです。
- `-M` — 色をグレーレベルに変換し、モノクロ（8 ビットグレースケール）画像を作成します。
- `-N<nudge_x>/<nudge_y>` — 画像の位置を東へ nudge_x メートル、北へ nudge_y メートルずらします。
- `-R<west>/<east>/<south>/<north>` — グリッドのこの部分だけを使います。既定値: グリッド全体。
- `-V` — 詳細な進行状況を表示します。

## 使用例

`mbgrid` で作成した水深グリッドの範囲からカラーパレットを作成し（深い所が青）、グリッドを GeoTIFF に変換します（survey.tfw も書き出されます）。

    gmt grd2cpt survey.grd -Chaxby -Z -I > survey.cpt
    gmt mbgrdtiff -Isurvey.grd -Csurvey.cpt -JX15c -Osurvey.tif

北西から照明した強度グリッドを使って陰影起伏画像を作成します。

    gmt grdgradient survey.grd -A315 -Ne0.6 -Gsurvey_int.grd
    gmt mbgrdtiff -Isurvey.grd -Isurvey_int.grd -Csurvey.cpt -JX15c -Osurvey_shaded.tif

`mbmosaic` で作成したサイドスキャンモザイクを 8 ビットグレースケールの GeoTIFF に変換します。

    gmt makecpt -Cgray -T0/60/1 -Z > ss.cpt
    gmt mbgrdtiff -Imosaic.grd -Css.cpt -JX15c -Omosaic.tif

画像を東へ 2.5 m、南へ 1 m ずらします。

    gmt mbgrdtiff -Isurvey.grd -Csurvey.cpt -JX15c -N2.5/-1 -Osurvey_shifted.tif

## 注意

- `mbgrdtiff` は独立したプログラムではなく、MB-System の GMT サプリメントのモジュールです。常に GMT ドライバから `gmt mbgrdtiff` として実行します。GMT はインストールフォルダの `bin\gmt_plugins` からサプリメントを自動的に見つけるため、GMT_CUSTOM_LIBS の設定は不要です。
- GMT の作図モジュールと異なり、`mbgrdtiff` は PostScript を書き出しません。ここでの `-O` は出力ファイル名であり、GMT の重ね描きオプションではありません。
- `mbgrdtiff` で `-JM` のような非線形投影を使わないでください。グリッドは地図用に再サンプリングされますが、GeoTIFF には元の座標が記録されたままになります。
- `mbgrid` のグリッドは既定では水深（下向き正）、`-A2` では地形（上向き正）を含みます。`gmt grd2cpt` はカラーパレットをグリッドの値の範囲に合わせます。地形グリッドの場合は最初の例から `-I` を外してください。
- オプションの引数はオプション文字の直後に空白を入れずに書きます。空白を含むファイル名は引用符で囲んでください。

## 関連項目

`mbgrid`、`mbmosaic`、`mbswath`、`mbcontour`。完全なリファレンスは MB-System のマニュアルページ mbgrdtiff(1)（英語）です。
