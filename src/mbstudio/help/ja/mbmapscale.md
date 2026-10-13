# mbmapscale — 経緯度 1 度あたりのメートル数を表示

`mbmapscale` は、指定した緯度における地理座標（経度・緯度）と東方向・北方向のメートルとの間の局所的な縮尺を表示します。局所的な直交座標近似で度とメートルを換算する場合、たとえばメートル単位の間隔に相当する度単位のグリッドセルサイズを決める場合や、調査海域でのオフセットを解釈する場合に使用します。

## 書式

    mbmapscale
        --alvinxy {-A}
        --help {-H}
        --latitude=latitude {-Llatitude}
        --verbose {-V}

## 説明

縮尺は `-L` で与えた緯度（10 進度、北が正、南が負）で計算されます。緯度を指定しない場合は赤道（緯度 0.0）で計算されます。

既定では、他の MB-System プログラムの局所縮尺計算と同じく WGS72 楕円体を使用します。`-A` を指定すると、AlvinXY 局所座標系で用いられる 1866 年 Clarke 楕円体で計算します。

結果は標準出力に 4 つの値として書き出されます：経度 1 度あたりのメートル数、緯度 1 度あたりのメートル数、およびその逆数（「Meters to degree longitude」「Meters to degree latitude」と表示）で、後者は東方向・北方向 1 メートルに相当する度数です。

## 主なオプション

- `-Llatitude` — 縮尺を計算する緯度（10 進度）（ロング形式 `--latitude=latitude`）。既定値：0.0（赤道）。
- `-A` — WGS72 楕円体の代わりに AlvinXY 方式の 1866 年 Clarke 楕円体を使用します（ロング形式 `--alvinxy`）。
- `-V` — 詳細モード（ロング形式 `--verbose`）。プログラム名と MB-System のバージョンも表示します。
- `-H` — 簡単な説明と使い方を表示して終了します（ロング形式 `--help`）。

## 使用例

既定の WGS72 楕円体を使い、北緯 36.8 度での縮尺を表示します：

    mbmapscale -L36.8

出力は次のような形式です：

    Local scaling between degrees longitude and latitude and meters east and north:
        Using WGS72 ellipsoid
        Meters per degree longitude: 89244.230
        Meters per degree latitude:  110973.870
        Meters to degree longitude:  0.000011205
        Meters to degree latitude:   0.000009011

同じ緯度で AlvinXY（1866 年 Clarke 楕円体）方式の縮尺を表示します：

    mbmapscale -L36.8 -A

南半球の緯度（負の値）での縮尺を表示します：

    mbmapscale --latitude=-41.5

## 注意

- 結果に影響するのは緯度だけで、経度は不要です。
- オプションは小文字（`-l`、`-a`、`-v`、`-h`）でも指定できます。

## 関連項目

`mbtime`、`mbgrid`、`mbmosaic`、`mbinfo`。完全なリファレンスは MB-System のマニュアルページ mbmapscale(1)（英語）です。
