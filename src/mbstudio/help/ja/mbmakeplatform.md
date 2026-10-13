# mbmakeplatform — プラットフォーム（センサーオフセット）ファイルの作成・修正

`mbmakeplatform` は MB-System のプラットフォームファイル（`.plf`）を作成または修正します。プラットフォームファイルは、調査プラットフォーム上のセンサーと、センサー間の位置オフセットおよび角度オフセットを記述したテキストファイルです。生データに完全で正しいセンサー配置が含まれていない場合（AUV、ROV、曳航体などの独自システムで多く見られます）に、`mbpreprocess`（オプション `--platform-file`）で使用します。

## 書式

    mbmakeplatform
        --output=plffile
        [
        --verbose
        --help
        --input=plffile
        --swath=datalist
        --swath=swathfile
        --swath-format=value
        --platform-type-surface-vessel | --platform-type-tow-body | --platform-type-rov
        --platform-type-auv | --platform-type-aircraft | --platform-type-satellite
        --platform-name=string
        --platform-organization=string
        --platform-documentation-url=string
        --platform-start-time=yyyy/mm/dd/hh/mm/ss.ssssss
        --platform-end-time=yyyy/mm/dd/hh/mm/ss.ssssss
        --add-sensor-<type>
        --modify-sensor=sensorid
        --modify-sensor-<source>
        --sensor-model=string
        --sensor-manufacturer=string
        --sensor-serialnumber=string
        --sensor-capability-<capability>
        --sensor-capability1=value
        --sensor-capability2=value
        --sensor-offsets=x/y/z/azimuth/roll/pitch
        --sensor-offset-positions=x/y/z
        --sensor-offset-angles=azimuth/roll/pitch
        --sensor-time-latency=value
        --sensor-time-latency-model=file
        --sensor-flipsign-heading
        --sensor-flipsign-roll
        --sensor-flipsign-pitch
        --sensor-source-<source>
        --modify-offsets=ioff/x/y/z/azimuth/roll/pitch
        --modify-offset-positions=ioff/x/y/z
        --modify-offset-angles=ioff/azimuth/roll/pitch
        --modify-time-latency=value
        --modify-time-latency-model=file
        --end-sensor
        --set-source-<source>=sensorid
        ]

`<source>` は `bathymetry`、`backscatter`、`subbottom`、`camera`、`position`、`depth`、`heading`、`rollpitch`、`heave` のいずれかで、末尾に `1`、`2`、`3` を付けることもできます（例: `bathymetry2`）。

## 説明

プラットフォームファイルには、プラットフォーム上のすべてのセンサー（種類、型式、メーカー、シリアル番号、計測機能）、各センサーの位置・角度オフセット、必要に応じて時間遅延（レイテンシ）、および各データ種別（水深、後方散乱、位置、深度、方位、ロール・ピッチ、ヒーブなど）の取得元となるセンサーが記録されます。通常は測位センサーをプラットフォーム原点とし、オフセットをゼロにします。

プラットフォームファイルは、通常 1 回の長い `mbmakeplatform` コマンドで作成します。オプションは指定した順に処理されます。`--add-sensor-...` で新しいセンサーの定義を開始し、続く `--sensor-...` オプションでそのセンサーを記述し、`--end-sensor` で終了します。同様に、`--modify-sensor=sensorid`（センサー ID は 0 から数えます）または `--modify-sensor-<source>` で既存のセンサーを開き、`--end-sensor` まで修正します。

オフセットの符号は、x が右舷方向、y が前方、z が上方を正とします。ロールは右舷側が上がる向き、ピッチは船首側が上がる向き、方位角（azimuth）は前方から時計回りを正とします。送信アレイと受信アレイが分かれたマルチビームやステレオカメラのように 2 つの要素を持つセンサーには `--sensor-offsets` を 2 回指定し、1 つ目を送信アレイ（または左カメラ）とします。

`--input` を指定すると既存のプラットフォームファイルを読み込み、後続のオプションで修正します。`--swath` を指定すると、スワスデータから初期のプラットフォーム（センサー一覧とオフセット）の抽出を試みます。これは最近の Kongsberg および Teledyne のフォーマット（例: フォーマット 58、59、88、89）でのみ有効です。出力ファイルを指定したのにセンサーが 1 つも定義されていない場合は、オフセットがゼロで全データの取得元となる汎用センサー 1 つが書き出されます。

記録データに完全なプラットフォーム記述が含まれている場合、プラットフォームファイルは不要です。たとえば Kongsberg のマルチビームデータでは通常必要ありません。

## 主なオプション

- `--output=plffile` — 書き出すプラットフォームファイル。このオプションがないと何も書き出されません。
- `--input=plffile` — 既存のプラットフォームファイルを読み込み、後続のオプションで修正します。
- `--swath=datalist` または `--swath=swathfile` — スワスデータ中で最初に見つかったプラットフォーム定義から初期化します（最近の Kongsberg/Teledyne フォーマットのみ）。
- `--swath-format=value` — `--swath` 入力のフォーマット。ファイル名から判別できない場合のみ必要で、`--swath` より前に指定します。
- `--platform-type-surface-vessel`、`--platform-type-tow-body`、`--platform-type-rov`、`--platform-type-auv`、`--platform-type-aircraft`、`--platform-type-satellite` — プラットフォームの種類を設定します。
- `--platform-name=string`、`--platform-organization=string` — プラットフォーム名と運用組織名。
- `--add-sensor-<type>` — 新しいセンサーの定義を開始します。種類: `sonar-echosounder`、`sonar-multiechosounder`、`sonar-sidescan`、`sonar-interferometry`、`sonar-multibeam`、`sonar-multibeam-twohead`、`sonar-subbottom`、`camera-mono`、`camera-stereo`、`camera-video`、`lidar-scan`、`lidar-swath`、`position`、`compass`、`vru`、`imu`、`ins`、`ins-with-pressure`、`ctd`、`pressure`、`soundspeed`。
- `--sensor-model=string`、`--sensor-manufacturer=string`、`--sensor-serialnumber=string` — 現在のセンサーの型式、メーカー、シリアル番号。
- `--sensor-capability-<capability>` — 現在のセンサーに計測機能を追加します。例: `position`、`depth`、`heading`、`rollpitch`、`soundspeed`、`topography-multibeam`、`backscatter-multibeam`、`backscatter-sidescan`、`topography-lidar`、`stereophotography`。`--sensor-capability1=value` と `--sensor-capability2=value` で 2 つの機能ビットマスクを直接設定することもできます。
- `--sensor-offsets=x/y/z/azimuth/roll/pitch` — 現在のセンサーの位置オフセット（m）と角度オフセット（度）。姿勢に依存しないセンサー（圧力計、CTD、音速計）には `--sensor-offset-positions=x/y/z`、姿勢のみを計測するセンサー（VRU）には `--sensor-offset-angles=azimuth/roll/pitch` を使います。
- `--sensor-time-latency=value` — 現在のセンサーの一定の時間遅延（秒）。`--sensor-time-latency-model=file` では時系列（エポック秒と遅延秒の 2 列）を読み込みます。
- `--sensor-flipsign-heading`、`--sensor-flipsign-roll`、`--sensor-flipsign-pitch` — センサーの方位、ロール、ピッチの符号が逆であることを指定します。
- `--sensor-source-<source>` — 現在のセンサーをそのデータ種別の取得元にします。例: `--sensor-source-bathymetry`、`--sensor-source-position`、`--sensor-source-heave`。
- `--end-sensor` — 現在のセンサーの定義または修正を終了します。
- `--modify-sensor=sensorid` — 既存のセンサー `sensorid`（0 から数えます）の修正を開始します。`--modify-sensor-bathymetry`、`--modify-sensor-position` などは、そのデータ種別の取得元になっているセンサーを選択します。
- `--modify-offsets=ioff/x/y/z/azimuth/roll/pitch` — 修正中のセンサーのオフセット組 `ioff`（0 または 1）を変更します。`--modify-offset-positions=ioff/x/y/z` と `--modify-offset-angles=ioff/azimuth/roll/pitch` は位置または角度のみを変更します。`--modify-time-latency=value` と `--modify-time-latency-model=file` は時間遅延を変更します。
- `--set-source-<source>=sensorid` — センサーを開かずに、センサー `sensorid` をそのデータ種別の取得元にします。
- `--verbose` — 読み込んだプラットフォームと書き出したプラットフォームを表示します。`--help` は使用法を表示します。

## 使用例

INS をプラットフォーム原点とし、送信アレイと受信アレイが分かれたマルチビームを搭載した調査船のプラットフォームファイルを作成します。

    mbmakeplatform --output=vessel.plf --platform-type-surface-vessel --platform-name="Survey Vessel" --add-sensor-ins --sensor-capability-position --sensor-capability-heading --sensor-capability-rollpitch --sensor-offsets=0/0/0/0/0/0 --sensor-source-position --sensor-source-heading --sensor-source-rollpitch --sensor-source-heave --end-sensor --add-sensor-sonar-multibeam --sensor-model="T50" --sensor-manufacturer="Reson" --sensor-capability-topography-multibeam --sensor-capability-backscatter-multibeam --sensor-offsets=0.0/1.20/-2.50/0.0/0.0/0.0 --sensor-offsets=0.0/1.40/-2.50/0.0/0.0/0.0 --sensor-source-bathymetry --sensor-source-backscatter --end-sensor

既存のプラットフォームファイルのセンサー 1 の受信アレイ（オフセット組 1）を修正し、新しいファイルに書き出します。

    mbmakeplatform --input=vessel.plf --output=vessel_v2.plf --modify-sensor=1 --modify-offsets=1/0.0/1.45/-2.48/0.0/0.10/0.0 --end-sensor

Teledyne 7k データに記録されたセンサー配置からプラットフォームファイルを作成します。

    mbmakeplatform --swath=0001_20250101_000000.mb88 --output=rov.plf --verbose

前処理でプラットフォームファイルを使用します。

    mbpreprocess --input=datalist.mb-1 --platform-file=vessel.plf

## 注意

- コマンドプロンプトではコマンド全体を 1 行で入力する必要があります。長い定義はバッチファイル（`.bat`）に書くと便利です。バッチファイルでは行末に `^` を付けると次の行に続けられます。
- オプションは順番に適用されます。`--input` や `--swath` は先頭に、`--swath-format` は `--swath` より前に指定してください。
- 空白を含む値は引用符で囲みます（例: `--platform-name="Survey Vessel"`）。
- 現在のソースでは、`--platform-start-time` と `--platform-end-time` の時刻は `yyyy/mm/dd hh:mm:ss.ssssss`（日付と時刻の間は空白。値を引用符で囲みます）として読み取られます。

## 関連項目

`mbpreprocess`、`mbprocess`、`mbinfo`、`mbsslayout`。完全なリファレンスは MB-System のマニュアルページ mbmakeplatform(1)（英語）です。
