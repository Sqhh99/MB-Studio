# mbauvloglist — MBARI AUV ログから指定項目を一覧出力

`mbauvloglist` は、MBARI AUV のバイナリ形式のミッションログファイル（例: `navigation.log`、`fastcatlog.log`、`kearfott.log`）を読み込み、選択した項目をテキストまたはバイナリの表として出力します。AUV ログから航法、姿勢、CTD、音速などの時系列を取り出すときに使います。たとえば音速プロファイルの作成や、前処理用の補助航法ファイルの作成に利用できます。

## 書式

    mbauvloglist
        --altitude-file=file {-Afile}
        --angles-degrees {-S}
        --clip-to-nav {-C}
        --decimate=value {-Dvalue}
        --help {-H}
        --input=file {-Ifile}
        --lonflip=lonflip {-Llonflip}
        --nav-file=file {-Nfile}
        --output-field=name {-Oname}
        --output-mode=mode {-Mmode}
        --print-format=format {-Fformat}
        --print-header {-P}
        --recalculate-ctd=id {-Rid}
        --scale=value {-Xvalue}
        --verbose {-V}

## 説明

MBARI AUV のミッションログは、ASCII ヘッダーで始まるバイナリファイルです。ヘッダーの各行は固定長バイナリレコード内の 1 つの値について、型、名前、既定の出力書式、説明、単位を記述します。例: `# double calculated_sound_velocity %8.8e ,Calculated sound velocity ,M/s`。

ログに含まれる値を確認するには、`-P` だけを指定して実行します。ヘッダーを表示して終了します。次に、出力したい列の順に値ごとに `-O` を 1 つずつ指定して再度実行します。`-O` を 1 つも指定しない場合は、ログ内のすべての項目が出力されます。出力は標準出力に書き出されるので、`>` でファイルにリダイレクトしてください。

値は、`-F` で printf 形式の書式を指定しない限り、ログヘッダーの書式で出力されます。`-F` は、再度変更されるまで後続のすべての `-O` 項目に適用されます（`-Fdefault` でヘッダーの書式に戻ります）。レコードの時刻項目（ログ内の名前は `time`）では、`-Ftime_i` で年・月・日・時・分・秒を、`-Ftime_j` で年・通日・時・分・秒を出力します。同様に、`-X` は後続の浮動小数点の `-O` 項目に掛けるスケール係数を設定します。

ヘッダーで定義された項目のほかに、次の特殊な値を指定できます: `zero`、`timeTag`、`timeInterval`（直前のレコードからの時間）、`-N` で指定した外部航法ファイルから内挿した値（`mergeLon`、`mergeLat`、`mergeHeading`、`mergeSpeed`、`mergeDraft`、`mergeSensordepth`、`mergeRoll`、`mergePitch`、`mergeHeave`）、`-A` で指定した高度ファイルからの `mergeAltitude`、`fastcatlog.log` から計算する CTD 値（`calcConductivity`、`calcTemperature`、`calcPressure`、`calcSalinity`、`calcSoundspeed`、`calcPotentialTemperature`、`calcDensity`）、および Kearfott/Phins から導出する値（`calcKTime`、`calcKSpeed`、`calcPSpeed`）。

## 主なオプション

- `-Ifile` — 入力する MBARI AUV ミッションログファイル。
- `-P` — ログファイルのヘッダーを表示します。単独で使うと、利用できる項目を一覧表示して終了します。
- `-Oname` — 指定した項目を出力します。列ごとに、出力したい順序で繰り返し指定します。
- `-Fformat` — 後続の `-O` 項目に使う printf 形式の書式。時刻項目には `time_i` または `time_j`、`default` でヘッダーの書式に戻ります。
- `-Mmode` — 出力モード: `0` タブ区切りテキスト（既定）、`1` カンマ区切りテキスト、`2` 区切りのないバイナリ値。
- `-Nfile` — "fnv" 形式の航法ファイル。内挿した値を `merge...` 項目として出力できます。
- `-C` — `-N` と併用し、航法データの時間範囲外のレコードを出力しません。既定ではそのようなレコードも出力され、内挿値は 0 になります。
- `-Afile` — ASCII の高度ファイル（1 行に「時刻 高度」の組、単位は秒とメートル）。内挿した値を `mergeAltitude` として出力できます。
- `-Dvalue` — value 個ごとに 1 レコードだけを出力します。既定値: `1`（全レコード）。
- `-Xvalue` — 後続の浮動小数点の `-O` 項目に掛けるスケール係数。既定値: `1.0`。
- `-S` — Kearfott、Crossbow、Phins のログの角度項目をラジアンから度に変換します。
- `-Rid` — `fastcatlog.log` について、id で指定した校正係数を使って水温、電気伝導度、塩分、圧力を再計算します（定義されているのは `1` のみ）。
- `-Llonflip` — 経度の範囲: `-1` で -360〜0、`0` で -180〜180、`1` で 0〜360。既定値: `mbdefaults` で設定した値。
- `-V` — 詳細表示。
- `-H` — ヘルプを表示して終了します。

## 使用例

航法ログに保存されている項目を確認します。

    mbauvloglist -I navigation.log -P

CTD ログから圧力と音速を取り出してテキストファイルに保存します。

    mbauvloglist -I fastcatlog.log -F%f -O pressure -O calculated_sound_velocity > auv_ssv.txt

航法ログから日時、緯度、経度、深度を CSV で出力します。

    mbauvloglist -I navigation.log -M1 -Ftime_i -O time -Fdefault -O latitude -O longitude -O mDepth > auv_nav.csv

外部航法を Kearfott ログに内挿結合し、航法データの範囲内のレコードだけを出力します。

    mbauvloglist -I kearfott.log -N mission_nav.fnv -C -O time -O mergeLon -O mergeLat -O mHeadK -S

## 注意

- バッチファイル（`.bat`/`.cmd`）内では、書式中のパーセント記号を `%%` と書きます（例: `-F%%f`）。対話型のプロンプトでは上の例のとおり `-F%f` で動作します。
- 出力は収録順のままで、並べ替えはされません。CTD ログから音速プロファイルを作成するには、出力を圧力順に並べ替え、別のツールで再サンプリングしてください。
- `calc...` の CTD 値は `fastcatlog.log` に対してのみ、`calcKTime`/`calcKSpeed` は `kearfott.log` に対してのみ定義されています。

## 関連項目

`mbpreprocess`、`mbnavlist`、`mblist`、`mbctdlist`、`mbsvplist`。完全なリファレンスは MB-System のマニュアルページ mbauvloglist(1)（英語）です。
