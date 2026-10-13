# mbminirovnav — MiniROV のログを形式 165 の ROV 航法に統合

`mbminirovnav` は、MBARI MiniROV が潜航中に記録する 4 種類のログファイル（USBL 航法、CTD、DVL、ROV 姿勢）を読み込み、MBARI ROV 形式の 1 つの ROV 航法ファイルにまとめます。この形式は MB-System ではフォーマット 165（MBF_MBARIROV）として読み込めます。MiniROV の航法を編集・描画したり、スワスデータと組み合わせたりする準備に使用します。

## 書式

    mbminirovnav
        --help
        --input-ctd-file=file
        --input-dvl-file=file
        --input-nav-file=file
        --input-rov-file=file
        --interpolate-position
        --interval=seconds
        --output=file
        --rov-dive-start=yyyy/mo/da/hr/mn/sc
        --rov-dive-end=yyyy/mo/da/hr/mn/sc
        --utm-zone=zone_id/NorS
        --verbose

## 説明

MiniROV は UTC の 1 日ごとに、`CTD_*.txt`、`DVL_*.txt`、`NAV_*.txt`、`ROV_*.txt` という 4 つのカンマ区切りテキストファイルを記録します（この形式の記録は 2017 年 9 月の潜航 100 から）。いずれも行頭はエポック秒（UTC）のタイムスタンプです。本プログラムが使用するのは次の値です。

- NAV: Winfrog ソフトウェアから `$GPGLL` 文で渡される USBL 位置（緯度・経度は度と 10 進分）。
- CTD: CTD の圧力センサーによる深度。
- DVL: 海底からの高度（DVL がボトムロックしているときのみ 0 以外）。DVL の速度は使用しません。
- ROV: 針路、ピッチ、ロール。

各値は、`--rov-dive-start` から `--rov-dive-end` まで `--interval` 秒ごと（既定値 1 秒）の等間隔の出力時刻に内挿されます。これらのオプションを指定しない場合は、入力ファイル全体の時間範囲が使われます。各出力レコードには日時、緯度、経度、UTM の東距・北距、深度、針路、高度、ピッチ、ロール、有効フラグが含まれます。ファイルが指定されていない、またはその時刻のデータがないために欠けている値には無効フラグが付きます。

NAV ファイルと ROV ファイルは必須で、両方のデータがないと出力ファイルは書き出されません。CTD ファイルと DVL ファイルは省略可能です。UTM ゾーンは `--utm-zone` を指定しない限り入力の平均位置から計算されます。形式 165 のファイルには使用したゾーンは記録されません。

MiniROV のロガーは最新の USBL 測位値を約 2 秒ごとに繰り返し記録するため、位置が変わらないのは新しい測位がないことを意味します。既定ではこの繰り返し位置はそのまま残されます。`--interpolate-position` は繰り返し位置を捨て、異なる測位値だけから出力時刻の位置を求めます。USBL データに誤った測位（外れ値）が含まれる場合、それが測位間の位置にも影響するため推奨されません。測位値を編集してから内挿する方が適切です。

## 主なオプション

- `--input-nav-file=file` — MiniROV の NAV ログ（USBL 位置）。必須です。
- `--input-rov-file=file` — MiniROV の ROV ログ（針路、ピッチ、ロール）。必須です。
- `--input-ctd-file=file` — MiniROV の CTD ログ（圧力深度）。
- `--input-dvl-file=file` — MiniROV の DVL ログ（高度）。
- `--output=file` — 出力する形式 165 の航法ファイル。必須で、既定値はありません。
- `--interval=seconds` — 出力レコードの時間間隔。既定値は 1.0 です。
- `--rov-dive-start=yyyy/mo/da/hr/mn/sc` — 出力の開始時刻（UTC）。例: `2017/09/05/16/54/00`。
- `--rov-dive-end=yyyy/mo/da/hr/mn/sc` — 出力の終了時刻（UTC）。
- `--utm-zone=zone_id/NorS` — 東距・北距に使う UTM ゾーン。例: `8/N` または `8N`。既定値は平均位置から求めます。
- `--interpolate-position` — 繰り返しの USBL 位置を捨て、異なる測位値だけから出力を求めます（上記参照）。
- `--verbose` — パラメータと、入力レコード数・有効な出力レコード数の集計を表示します。
- `--help` — 使い方を表示して終了します。

## 使用例

UTC の 0 時をまたぐ潜航のデータがそろうよう、まず航海中の日ごとのファイルを連結します（Windows の `copy` は名前順にファイルを結合します）：

    copy /b NAV_*.txt MiniROV_NAV_all.txt
    copy /b CTD_*.txt MiniROV_CTD_all.txt
    copy /b DVL_*.txt MiniROV_DVL_all.txt
    copy /b ROV_*.txt MiniROV_ROV_all.txt

1 回の潜航の着底中の航法を UTM ゾーン 8（北半球）で作成します：

    mbminirovnav --input-nav-file=MiniROV_NAV_all.txt --input-ctd-file=MiniROV_CTD_all.txt --input-dvl-file=MiniROV_DVL_all.txt --input-rov-file=MiniROV_ROV_all.txt --output=MiniROVnav_100.mb165 --utm-zone=8N --rov-dive-start=2017/09/05/16/54/00 --rov-dive-end=2017/09/05/17/56/00 --verbose

同じ潜航を、NAV と ROV のデータだけを使って 0.5 秒間隔で書き出します：

    mbminirovnav --input-nav-file=MiniROV_NAV_all.txt --input-rov-file=MiniROV_ROV_all.txt --output=MiniROVnav_100_nav.mb165 --interval=0.5 --rov-dive-start=2017/09/05/16/54/00 --rov-dive-end=2017/09/05/17/56/00

## 注意

- ロングオプションのみ使用できます。組み込みの使い方表示には `yyyymmddhhmmss` と書かれていますが、時刻はスラッシュ区切り（`yyyy/mo/da/hr/mn/sc`）で指定してください。使い方表示にある `--input` オプションは効果がありません。
- パスを指定しない場合、出力ファイルはカレントディレクトリに書き出されます。スペースを含むパスは引用符で囲んでください。
- マニュアルページで紹介されている対話型の航法エディタ `mbnavedit` は Windows では利用できません。

## 関連項目

`mbusbl2fnv`, `mbnavlist`, `mbpreprocess`, `mbinfo`。完全なリファレンスは MB-System のマニュアルページ mbminirovnav(1)（英語）です。
