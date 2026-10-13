# mbgrd2gltf — 水深グリッドを glTF 3D モデルに変換

`mbgrd2gltf` は、GMT の水深グリッド（netCDF の `.grd`。例: `mbgrid` で作成）を glTF 3D モデル（テキスト形式の `.gltf` またはバイナリ形式の `.glb`）に変換します。3D ビューア、ゲームエンジン、Web ページ、プレゼンテーションツールなどで利用できます。鉛直誇張、Draco メッシュ圧縮、描画精度を高めるローカル座標原点、HTML ビューアページの作成をオプションで指定できます。

## 書式

    mbgrd2gltf --input FILE [--binary] [--output ROOT] [--exaggeration NUM]
               [--geoorigin[=LON,LAT,ELEV]] [--draco] [--quantize-position NUM]
               [--quantize-normal NUM] [--quantize-texcoord NUM] [--quantize-color NUM]
               [--html] [--verbose] [--help]

    mbgrd2gltf -IFILE [-B] [-OROOT] [-ENUM] [-G[LON,LAT,ELEV]] [-D]
               [-QpNUM] [-QnNUM] [-QtNUM] [-QcNUM] [-W] [-V] [-H]

## 説明

入力は netCDF 形式の GMT グリッドで（COARDS の経度・緯度グリッドと旧 GMT netCDF グリッドを読み込めます）、経度・緯度座標と標高または水深の値を持つものです。グリッドは三角形メッシュに変換され、大きなグリッドはタイル分割で処理されます。頂点の位置は地球中心・地球固定（ECEF）の直交座標系（単位メートル）で表され、原点は地球の中心、Z 軸は北極方向、X 軸は赤道上の本初子午線方向です。

ECEF 座標は数百万メートルの大きさになるため、グラフィックスカードの単精度演算では精度が落ちることがあります。`--geoorigin` を指定すると、頂点はローカル原点からのオフセットとして書き出されます。原点は経度・緯度・標高で指定するか、グリッドの中心と平均高度から自動的に求めます。`--geoorigin` を指定しない場合は ECEF 座標がそのまま使われます。

出力は `ROOT.gltf`、`--binary` を指定した場合は `ROOT.glb` です。`ROOT` の既定値は入力ファイル名から拡張子を除いたもので、入力と同じフォルダに書き出されます。Draco 圧縮（`--draco`）でファイルサイズを大きく削減できます。量子化オプションは位置、法線、テクスチャ座標、色に使うビット数を設定します（ビット数が少ないほどファイルは小さく、精度は低くなります）。`--html` を指定すると、X3DOM でモデルを処理情報とともに表示する HTML ビューアページ `ROOT.html` も書き出されます。

## 主なオプション

- `--input FILE`、`-IFILE` — 入力 GMT グリッドファイル（必須）。
- `--output ROOT`、`-OROOT` — 出力ファイルのルート。`.gltf` または `.glb` が付加されます。既定値: 入力フォルダ内の、拡張子を除いた入力ファイル名。
- `--binary`、`-B` — バイナリ glTF（`.glb`）を書き出します。よりコンパクトで読み込みも速くなります。既定値: テキスト glTF。
- `--exaggeration NUM`、`-ENUM` — 鉛直誇張率（0 より大きい値）。既定値: 1.0。
- `--geoorigin[=LON,LAT,ELEV]`、`-G[LON,LAT,ELEV]` — 頂点をローカル原点からの相対座標で書き出します。値を省略するとグリッドの中心と平均高度を使います。既定値: ECEF 座標そのもの。
- `--draco`、`-D` — Draco メッシュ圧縮を有効にします。
- `--quantize-position NUM`、`-QpNUM` — Draco の位置量子化ビット数（2～30）。既定値: 16。
- `--quantize-normal NUM`、`-QnNUM` — Draco の法線量子化ビット数（2～30）。既定値: 7。
- `--quantize-texcoord NUM`、`-QtNUM` — Draco のテクスチャ座標量子化ビット数（2～30）。既定値: 10。
- `--quantize-color NUM`、`-QcNUM` — Draco の色量子化ビット数（2～30）。既定値: 8。
- `--html`、`-W` — HTML ビューアページ `ROOT.html` も書き出します。
- `--verbose`、`-V` — グリッドの読み込みとメモリ使用の詳細を表示します。
- `--help`、`-H` — ヘルプを表示して終了します。

## 使用例

既定の設定でグリッドをテキスト glTF モデルに変換します（`survey_bathy.gltf` が作成されます）。

    mbgrd2gltf --input survey_bathy.grd

鉛直誇張 5 倍、グリッド中心をローカル原点として、コンパクトなバイナリモデルを書き出します。

    mbgrd2gltf --input survey_bathy.grd --binary --exaggeration 5 --geoorigin

位置精度を高めた Draco 圧縮バイナリモデルを別のフォルダに書き出します。

    mbgrd2gltf --input survey_bathy.grd --binary --draco --quantize-position 20 --output "D:\Models\survey area"

ローカル原点を明示的に指定し、HTML ビューアページも作成します。

    mbgrd2gltf --input monterey.grd --geoorigin=-122.0,36.8,-1000 --binary --html

## 注意

- 原点の値は `=` を付けて指定してください（`--geoorigin=-122.0,36.8,-1000`、旧形式では `-G-122.0,36.8,-1000`）。このオプションでは、別の引数として書いた値は読み込まれません。
- 旧形式の書式では、`-D` を `-Q` オプションより前に指定する必要があります。
- グリッドは地理座標で作成してください（`mbgrid` で `-J` を使わない）。空のセルは NaN にしておくことを推奨します（`mbgrid -N`）。
- 既定の出力名は入力名の最初のドットまでの部分なので、`area.v2.grd` からは `area.gltf` が作られます。別の名前にするには `--output` を使います。
- Web ブラウザは、直接開いたローカルの `.html` ファイルからモデルを読み込むことを通常拒否します。ローカル Web サーバー（例: `python -m http.server 8000`）でフォルダを公開し、`http://localhost:8000/` 経由でページを開いてください。

## 関連項目

`mbgrid`、`mbinfo`、`gmt mbgrdtiff`。完全なリファレンスは MB-System のマニュアルページ mbgrd2gltf(1)（英語）です。
