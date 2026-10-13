# mbroutetime — 測量データが計画ルートの各点に達した時刻を出力

`mbroutetime` は、スワスデータの航法を計画測線ルートと比較し、各測線の開始点・終了点（ウェイポイント）を通過した位置と時刻を書き出します。作成したウェイポイント時刻リストは `mbextractsegy`（オプション `-Q`）で使用し、サブボトムデータを計画測線ごとのファイルに分割できます。

## 書式

    mbroutetime  -Rroutefile [-Fformat -Ifile -Owaypointtimefile -Urangethreshold -H -V]

        --format=format_id {-Fformat_id}
        --help {-H}
        --input=file {-Ifile}
        --output=waypointtimefile {-Owaypointtimefile}
        --range-threshold=rangethreshold {-Urangethreshold}
        --route-file=routefile {-Rroutefile}
        --verbose {-V}

## 説明

ルートファイルは MBgrdviz 形式のテキストのルートファイル（`.rte`）です。バージョンヘッダのあるルートファイルでは、測線の開始点と終了点のウェイポイントだけが使われ、移動用のウェイポイントは無視されます。このようなウェイポイントが 2 つ以上必要です。バージョンヘッダのない古いルートファイルでは、すべての点が使われます。

本プログラムはスワスデータ（単一ファイルまたはデータリスト、既定値 `datalist.mb-1`）を順に読み込み、各スワスファイルの `.fnv` 航法ファイルがあればそれを使います。ウェイポイントは 1 つずつ順番に探索されます。航法がしきい値距離（既定値 25 m）以内に近づき、その後距離が増加し始めた時点でウェイポイントに到達したとみなされ、最接近時の値が記録されます。最後のウェイポイントに達する前にデータが終わった場合は、最後の航法位置に最終点が追加されます。

出力テキストファイルには見つかったウェイポイントごとに 1 行が書かれ、列は通し番号、ウェイポイントの種類（3 = 測線開始、4 = 測線終了）、経度、緯度、針路、10 進のエポック秒（1970-01-01 UTC からの秒数）による時刻です。既定のファイル名は入力名に `_wpttime_d.txt` を付けたもの（例: `datalist.mb-1_wpttime_d.txt`）です。

## 主なオプション

- `-Rroutefile` — 計画ウェイポイントを含む MBgrdviz ルートファイル（ロング形式 `--route-file=routefile`）。必須です。
- `-Ifile` — 確認するスワスファイルまたはデータリスト（ロング形式 `--input=file`）。既定値は `datalist.mb-1` です。
- `-Fformat_id` — 入力の MB-System フォーマット。通常はファイル名から判別され、データリストでは負の値です（ロング形式 `--format=format_id`）。
- `-Owaypointtimefile` — 出力するウェイポイント時刻リスト（ロング形式 `--output=waypointtimefile`）。既定値は入力名に `_wpttime_d.txt` を付けた名前です。
- `-Urangethreshold` — ウェイポイントに到達したとみなす距離（m）（ロング形式 `--range-threshold=rangethreshold`）。既定値は 25 です。
- `-V` — 状況メッセージを表示します。ping ごとの対象ウェイポイントまでの距離も表示されます（ロング形式 `--verbose`）。
- `-H` — 説明と使い方を表示して終了します（ロング形式 `--help`）。

## 使用例

`datalist.mb-1` の測量がルートのウェイポイントを通過した時刻を求めます：

    mbroutetime -R Line_plan.rte -I datalist.mb-1 -O Line_plan_wpttime.txt

ウェイポイントから 50 m 以内の通過を許容します：

    mbroutetime -R Line_plan.rte -I datalist.mb-1 -U50 -O Line_plan_wpttime.txt

ウェイポイント時刻を使って、サブボトムデータを計画測線ごとの SEGY ファイル（名前は `Line_plan` から作成、番号は 1 から）に抽出します：

    mbextractsegy -I datalist.mb-1 -Q Line_plan_wpttime.txt -L1/Line_plan

## 注意

- ウェイポイントはルートの順に通過する必要があります。あるウェイポイントを見逃す（しきい値以内に近づかない）と、それ以降のウェイポイントも見つかりません。その場合は `-U` を大きくしてください。
- ルートファイルは通常 `mbgrdviz` で作成しますが、`mbgrdviz` は Windows では利用できません。他のシステムで作成したルートファイルを使用できます。
- スペースを含むパスは引用符で囲んでください。

## 関連項目

`mbextractsegy`, `mbnavlist`, `mbinfo`, `mbdatalist`。完全なリファレンスは MB-System のマニュアルページ mbroutetime(1)（英語）です。
