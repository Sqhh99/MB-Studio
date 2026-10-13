# mbpreprocess — 生スワスデータの前処理と補助データの結合

`mbpreprocess` は、MB-System における生スワスソナーデータの最初の処理ステップです。生ファイルを（通常はデータリスト経由で）読み込み、他のプログラムが必要とする補助ファイルとともに MB-System の処理用フォーマットで書き出します。また、航法、センサー深度、方位、姿勢、高度、音速を、データストリーム内または外部ファイルから結合できます。`mbmakedatalist` の後、編集（`mbclean`）や `mbprocess` の前に実行します。

## 書式

    mbpreprocess --input=datalist [--format=format_id]
        --verbose --help
        --output-directory=directory
        --platform-file=platform_file --platform-target-sensor=sensor_id
        --output-sensor-fnv --skip-existing
        --nav-file=file --nav-file-format=format_id
        --nav-async=record_kind --nav-sensor=sensor_id
        --sensordepth-file=file --sensordepth-file-format=format_id
        --sensordepth-async=record_kind --sensordepth-sensor=sensor_id
        --heading-file=file --heading-file-format=format_id
        --heading-async=record_kind --heading-sensor=sensor_id
        --altitude-file=file --altitude-file-format=format_id
        --altitude-async=record_kind --altitude-sensor=sensor_id
        --attitude-file=file --attitude-file-format=format_id
        --attitude-async=record_kind --attitude-sensor=sensor_id
        --attitude-zero-heave
        --soundspeed-file=file --soundspeed-file-format=format_id
        --soundspeed-async=record_kind --soundspeed-sensor=sensor_id
        --time-latency-file=file --time-latency-file-format=format_id
        --time-latency-constant=value
        --time-latency-apply-nav --time-latency-apply-sensordepth
        --time-latency-apply-heading --time-latency-apply-attitude
        --time-latency-apply-altitude --time-latency-apply-all-ancilliary
        --time-latency-apply-survey --time-latency-apply-all
        --filter=value
        --filter-apply-nav --filter-apply-sensordepth
        --filter-apply-heading --filter-apply-attitude
        --filter-apply-altitude --filter-apply-all-ancilliary
        --recalculate-bathymetry --no-change-survey
        --multibeam-sidescan-source=recordid
        --sounding-amplitude-filter=value --sounding-altitude-filter=value
        --ignore-water-column
        --head1-offsets=x/y/z/heading/roll/pitch
        --head2-offsets=x/y/z/heading/roll/pitch
        --kluge-time-jumps=threshold --kluge-fix-7k-timestamps=targetoffset
        --kluge-ancilliary-time-jumps=threshold
        --kluge-mbaripressure-time-jumps=threshold
        --kluge-beam-tweak=factor --kluge-soundspeed-tweak=factor
        --kluge-zero-attitude-correction --kluge-zero-alongtrack-angles
        --kluge-fix-wissl-timestamps --kluge-auv-sentry-sensordepth
        --kluge-ignore-snippets --kluge-sensordepth-from-heave
        --kluge-early-MBARI-Mapping-AUV
        --kluge-flipsign-roll --kluge-flipsign-pitch
        --kluge-set-beamwidths=beamwidth-acrosstrack/beamwidth-alongtrack
        --kluge-set-beamwidth-acrosstrack=beamwidth-acrosstrack
        --kluge-set-beamwidth-alongtrack=beamwidth-alongtrack
        --kluge-ignore-duplicate-pings
        --kluge-xducer-depth-from-heave
        --kluge-xducer-depth-from-sensordepth
        --kluge-xducer-depth-from-heave-and-sensordepth
        --kluge-rangescale=factor --kluge-fix-wissl2-ranges

## 説明

`mbpreprocess` は、旧バージョンの MB-System にあったフォーマットごとの前処理プログラムを置き換えるものです。入力ファイルごとに 2 パスで処理します。パス 1 では、データストリームと指定された外部ファイルから非同期の補助データ（航法、センサー深度、方位、姿勢、高度、音速）を読み込みます。パス 2 では、選択した補助データを各ピングに結合しながら調査データを新しいファイルに書き出し、後続のプログラムが使う補助ファイルを作成します。作成されるのは、`.inf` 統計ファイル（`mbinfo` を実行して作成）、`.fnv` 航法ファイル、通常は `.fbt` 水深ファイル、および姿勢とセンサー深度の時系列ファイルです。

出力ファイル名は、入力ファイル名の拡張子を除いた部分に `.mbXX`（XX は出力フォーマット ID）を付けたものです。一部のフォーマットは拡張された処理用フォーマットに変換されます。たとえば Kongsberg の `.all` データ（フォーマット 58）はフォーマット 59（`0001_20250101_000000.mb59`）に、旧 Kongsberg データ（56）はフォーマット 57 に、Imagenex 83P データ（191）はフォーマット 192 になります。その他のフォーマットはフォーマット ID が変わりません（例: `.kmall` データは `.mb261` になります）。出力名が入力名と同じになる場合は、拡張子の前に文字 `r` が付きます。`--output-directory` を指定しない限り、出力ファイルは入力ファイルと同じ場所に書き出されます。

既定では、データ内にすでにある補助データの値が使われます。データの種類ごとに別の取得元を選択できます。外部の時系列ファイル（`--nav-file` と `--nav-file-format`、センサー深度、方位、高度、姿勢、音速も同様）か、データストリーム内の別のレコード種別（`--nav-async=record_kind` など）です。航法、センサー深度、方位、姿勢、音速を結合する場合や、`...-apply-...` オプションを指定した場合は、水深の再計算も有効になります。Kongsberg の `.kmall` データでは、インストール情報のデータグラムを使って、使用中の航法・姿勢システムの選択とセンサー配置の構築が自動的に行われます。

プラットフォームファイル（`mbmakeplatform` で作成）は、センサーのオフセットがデータに適用されていない場合、主に AUV や ROV による調査でのみ必要です。対応しているのは 88、89、261、3D at Depth ライダーのフォーマットなど一部のフォーマットです。`--time-latency-...` オプションは補助データや調査データの時刻ずれを補正し、`--filter` は補助データをガウスフィルタで平滑化します。`...-apply-...` オプションを指定しない場合、どちらもすべての補助データに適用されます。`--kluge-...` オプションは特定のデータセットの問題に対する修正です。使用する前にマニュアルページを読んでください。

## 主なオプション

- `--input=file` — 前処理するスワスファイルまたはデータリスト。必ず指定してください。
- `--format=format_id` — 入力の MBIO フォーマット。既定: ファイル名の拡張子から推定。
- `--output-directory=directory` — 出力ファイルを入力ファイルと同じ場所ではなく、このディレクトリに書き出します。
- `--skip-existing` — 出力ファイルがすでに存在し、入力より新しく大きいファイルをスキップします。
- `--verbose` — 進行状況（両パスのレコード数）を表示します。
- `--nav-file=file` — 結合する外部航法。データ内の航法を置き換えます。
- `--nav-file-format=format_id` — 航法ファイルのフォーマット: `1` エポック秒 経度 緯度 [速度]、`2` 年 月 日 時 分 秒 経度 緯度、`3` 年 通日 時 分 秒 経度 緯度、`4` 年 通日 日内分 秒 経度 緯度、`5` L-DEO 航法、`6` または `7` NMEA 0183（ZDA、GLL、GGA など）、`8` Simrad 90、`9` MB-System の fnv 形式（日付と時刻、エポック秒、経度、緯度、方位、速度、センサー深度 [、ロール、ピッチ、ヒーブ]）、`10` R2R、`11` RVDAS、`12` Kongsberg Navlab の `navlab_smooth.bin`。
- `--nav-async=record_kind` — データ内で航法に使うレコード種別。例: `12`（航法）、`29`、`30`、`31`（航法システム 1〜3）、`1`（測深レコード）。
- `--sensordepth-file=file`、`--heading-file=file`、`--attitude-file=file`、`--altitude-file=file`、`--soundspeed-file=file` — これらの値を結合する外部時系列ファイル。対応する `...-file-format=` オプションには `1`（エポック秒と値）、`2`（年 月 日 時 分 秒と値）、`3`（年 通日 時 分 秒 ...）、`4`（年 通日 日内分 秒 ...）を指定でき、センサー深度、方位、姿勢では航法と同様に `9`、`11`、`12` も使えます。姿勢ファイルにはロール、ピッチ、ヒーブを記述します。
- `--sensordepth-async=`、`--heading-async=`、`--attitude-async=`、`--altitude-async=`、`--soundspeed-async=record_kind` — これらの値に使うレコード種別。例: `59` ソナー深度、`16` 高さ、`17` 方位、`18` 姿勢、`56`〜`58` 姿勢システム 1〜3、`60` 高度、`19` SSV、`34` CTD。ファイルに含まれるレコード種別は `mbinfo -N` で確認できます。
- `--attitude-zero-heave` — すべてのヒーブ値を 0 にします。
- `--platform-file=file` — このプラットフォームファイルを使ってレバーアームと角度オフセットを補正します。
- `--platform-target-sensor=sensor_id` — 調査データを取得したプラットフォーム上のセンサー（補助センサーには `--nav-sensor`、`--sensordepth-sensor`、`--heading-sensor`、`--attitude-sensor` を使用）。
- `--time-latency-constant=value` — 一定の時刻遅延（秒）。時間変化するモデルには `--time-latency-file=` と `--time-latency-file-format=`（1〜4）を使います。`--time-latency-apply-...` オプションで適用先を選びます。
- `--filter=value` — 補助データ用ガウス平滑化フィルタの半幅（秒）。`--filter-apply-...` オプションで適用先を選びます。
- `--recalculate-bathymetry` — 前処理中に水深を再計算します（フォーマットに依存）。
- `--no-change-survey` — 調査データを変更しません。
- `--multibeam-sidescan-source=recordid` — マルチビームのサイドスキャンに使う後方散乱（フォーマット 88、89、261）: `S` スニペット、`C` 校正済みスニペット、`B` ワイドビーム後方散乱、`W` 校正済みワイドビーム後方散乱。既定: 利用可能な最良のもの。
- `--ignore-water-column` — ウォーターカラムのレコードを出力にコピーしません（Reson/Teledyne 7k フォーマット）。
- `--help` — ヘルプを表示して終了します。

## 使用例

Kongsberg の生ファイルのデータリストを作成し、進行状況を表示しながら前処理します。

    mbmakedatalist -S .all
    mbpreprocess --input=datalist.mb-1 --verbose

増えていくデータセットに対して、前処理済みのファイルをスキップしながら再実行し、既存のフォルダー `preprocessed` に出力します。

    mbpreprocess --input=datalist.mb-1 --skip-existing --output-directory=preprocessed

航法を後処理済みの位置ファイル（エポック秒、経度、緯度の行）で置き換え、航法の一定遅延 0.05 秒を補正します。

    mbpreprocess --input=datalist.mb-1 --nav-file=ppk_nav.txt --nav-file-format=1 --time-latency-constant=0.05 --time-latency-apply-nav

センサー 2 がマルチビーム、センサー 0 が INS、センサー 1 が圧力センサーであるプラットフォームファイルを使い、AUV の Reson 7k データを前処理します。

    mbpreprocess --input=datalist.mb-1 --platform-file=auv.plf --platform-target-sensor=2 --nav-sensor=0 --heading-sensor=0 --attitude-sensor=0 --sensordepth-sensor=1

## 注意

- `mbpreprocess` には長い形式のオプション（`--name` または `--name=value`）しかなく、1 文字の形式はありません。
- `--output-directory` と `--platform-file` に指定するパスには空白を含めないでください。それ以外の空白を含むファイル名は `--input="my survey\datalist.mb-1"` のように引用符で囲んでください。
- 出力ディレクトリはあらかじめ作成しておく必要があります。
- 前処理の後、新しいファイルのデータリストを作成し（例: `mbmakedatalist -S .mb59 -P -O datalistp.mb-1`）、`mbclean`、`mbset`、`mbprocess` へ進みます。
- `.inf` ファイルは `mbinfo` を呼び出して作成されます。MB-System の `bin` フォルダーが PATH に含まれる MB-System Command Prompt または MB-Studio から実行してください。

## 関連項目

`mbmakedatalist`、`mbdatalist`、`mbinfo`、`mbmakeplatform`、`mbclean`、`mbset`、`mbprocess`、`mbnavlist`。完全なリファレンスは MB-System のマニュアルページ mbpreprocess(1)（英語）です。
