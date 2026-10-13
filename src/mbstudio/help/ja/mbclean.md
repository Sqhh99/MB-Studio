# mbclean — スワス水深の不良ビームを自動フラグ付け

`mbclean` はスワスソナーの水深データ内のアーティファクトを検出し、該当するビームにフラグを付けます。複数の判定方法（過大な傾斜、スパイク、水深の上下限、局所中央値からの偏差、外側ビーム、横断方向の距離または角度、船速、位置など）があり、1 回の実行で任意に組み合わせて適用できます。通常は自動的な一次編集として、前処理済みのデータに対して `mbprocess` の前に実行します。

## 書式

    mbclean
        --bounds=west/east/south/north {-Wwest/east/south/north}
        --depth-range=low/high {-Blow/high}
        --deviation-max=max {-Amax}
        --distance-range=min/max {-Dmin/max}
        --flag-distance-angle=distanceleft/distanceright[/mode] {-Ydistanceleft/distanceright[/mode]}
        --format=format {-Fformat}
        --fraction-range=fraction_low/fraction_high {-Gfraction_low/fraction_high}
        --help {-H}
        --input=infile {-Iinfile}
        --lonflip=lonflip {-Llonflip}
        --max-acrosstrack=max_acrosstrack {-Emax_acrosstrack}
        --max-heading-rate=maxheadingrate {-Rmaxheadingrate}
        --max-slope=slope/unit {-Cslope/unit}
        --min-good-beams=num_good_min {-Unum_good_min}
        --mode=mode {-Mmode}
        --ping-deviation-tolerance=tolerance {-Ntolerance}
        --range-min=range_min {-Krange_min}
        --speed-range=min_speed/max_speed {-Pmin_speed/max_speed}
        --spike-max=spike_slope/mode/format {-Sspike_slope/mode/format}
        --timestamp-tolerance=tolerance {-Ttolerance}
        --verbose {-V}
        --zap-beams=beamsleft/beamsright {-Xbeamsleft/beamsright}
        --zap-rails=backup_dist {-Qbackup_dist}
        --zero-position {-Z}

## 説明

`mbclean` は 1 つのスワスファイル、またはデータリスト内のすべてのファイル（既定は `datalist.mb-1`）を読み込みます。データファイル自体は変更しません。ビームフラグは、各スワスファイルの編集保存ファイル（スワスファイル名に `.esf` を付けたもの）に編集イベントとして書き込まれます。これは対話型編集や `mbareaclean` が作成するものと同じ種類のファイルです。編集保存ファイルがすでに存在する場合は、その編集を読み込んで先に適用し、新しいフラグを追加します。また `mbclean` は `mbprocess` のパラメータファイルを更新し、`mbprocess` が処理済みファイルを書き出すときに編集が適用されるようにします。

判定は次の順序で適用されます: 外側ビーム（`-X`）、横断方向の距離または角度（`-Y`）、ピングの速度（`-P`）、位置範囲（`-W`）、ゼロ位置（`-Z`）、絶対水深範囲（`-B`）、最小レンジ（`-K`）、方位変化率（`-R`）、レール（`-Q`）、最大横断距離（`-E`）、局所中央水深に対する比率（`-G`）、局所中央水深からの偏差（`-A`）、スパイク（`-S`）、過大な傾斜（`-C`）、良好ビーム数の不足（`-U`）、前後のピングからのピング全体の偏差（`-N`）。

傾斜判定（`-C`）は最もよく使われる方法で、既定の判定です。他の判定が指定されていない場合に適用されます。他の判定オプションを 1 つでも指定すると、`-C` も指定しない限り傾斜判定は行われません。局所中央水深は、現在のピングとその前後のピングから、`-D` の最大距離より近いビームを使って計算されます。

`mbclean` は各ファイルの終わりと実行の最後に、各判定でフラグを付けたビーム数を表示します。自動クリーニングですべてのアーティファクトを検出できるわけではないので、その後で必ずデータを目視で確認してください。

## 主なオプション

- `-Iinfile` — 入力スワスファイルまたはデータリスト。既定値: `datalist.mb-1`。
- `-Fformat` — MBIO フォーマット ID。負の値は入力がデータリストであることを意味します。既定: ファイル名の拡張子から推定。
- `-Cslope/unit` — slope より大きい傾斜に関係するビームにフラグを付けます。unit: `0` 正接（既定）、`1` ラジアン、`2` 度。slope の既定値: `1.0`。
- `-Mmode` — 過大な傾斜の扱い: `1` 局所中央水深から最も離れた 1 本のビームにフラグ（既定）、`2` 両方のビームにフラグ。
- `-Dmin/max` — 傾斜判定と中央値判定で使うビーム間の最小・最大距離（局所中央水深に対する比率）。既定値: `0.01/0.25`。
- `-Sspike_slope/mode/format` — スパイクにフラグを付けます。スパイクとは両側の傾斜が spike_slope を超え、かつ符号が逆のビームです。mode: `1` 横断方向（既定）、`2` 航走方向、`3` 両方。3 番目の値は `-C` と同じ傾斜の単位です。
- `-Blow/high` — low より浅い、または high より深い水深（m）にフラグを付けます。
- `-Gfraction_low/fraction_high` — 局所中央水深の fraction_low 倍未満、または fraction_high 倍を超える水深にフラグを付けます（例: `-G0.8/1.2`）。
- `-Amax` — 局所中央水深からの偏差が max メートルを超える水深にフラグを付けます。
- `-Xbeamsleft/beamsright` — 左舷側と右舷側の最も外側のビームを指定本数だけフラグ付けします（値が 1 つなら両側に適用）。
- `-Ydistanceleft/distanceright/mode` — 横断方向の位置でフラグ付け・解除します。mode: `1`（既定）距離範囲（m）外のビームにフラグ、`2` 範囲内のビームのフラグを解除、`3` ビーム角範囲（度）外のビームにフラグ、`4` 範囲内のビームのフラグを解除。左舷側の値は負です。
- `-Emax_acrosstrack` — 横断距離がこの値（m）を超えるビームにフラグを付けます。
- `-Qbackup_dist` — 「レール」にフラグを付けます。より内側のビームよりも backup_dist メートル以上内側にある外側ビームが対象です。
- `-Unum_good_min` — 良好なビームがこの数より少ないピングの左舷側または右舷側半分の全ビームにフラグを付けます。
- `-Ntolerance` — 前後のピングとの RMS 偏差が tolerance を超えるピングの全ビームにフラグを付けます（他のソナーからの干渉など）。
- `-Pmin_speed/max_speed` — 速度がこの範囲（km/h）外のピングにフラグを付けます。
- `-Wwest/east/south/north` — 航法がこの範囲外のピングにフラグを付けます。
- `-Z` — 経度・緯度がゼロのピングにフラグを付けます。
- `-Rmaxheadingrate` — この変化率（度/秒）より速く方位が変化している間のピングにフラグを付けます。
- `-Krange_min` — レンジが range_min（m）未満のビームにフラグを付けます。
- `-Ttolerance` — 既存の編集の時刻を、tolerance 秒以内で一致するピングの時刻に合わせ直します（`mbgetesf` で移した編集用）。
- `-V` — 詳細表示。

## 使用例

データリスト内のすべてのファイルに既定の傾斜判定を適用します。

    mbclean -I datalist.mb-1

傾斜が 3.5（約 74 度）を超えるビーム、または局所中央水深から 20 % 以上外れるビームにフラグを付けます。

    mbclean -I datalist.mb-1 -M1 -C3.5 -D0.01/0.20 -G0.80/1.20

傾斜判定を残したまま、両側の最も外側の 5 本のビームと、5〜250 m の範囲外の水深にフラグを付けます。

    mbclean -I datalist.mb-1 -X5 -B5/250 -C1.0

鉛直から 65 度を超えるビームにフラグを付け、その後すべての編集を適用します。

    mbclean -I datalist.mb-1 -Y-65/65/3
    mbprocess -I datalist.mb-1

## 注意

- フラグが処理済みデータに反映されるのは、`mbprocess` を実行したときです。
- `mbclean` を再度実行すると、既存の編集に追加されます。最初からやり直す場合は、先に `.esf` ファイルを削除するか名前を変更してください。
- 水深は下向きを正とするメートル単位です。`-B` と `-A` の値はメートルで指定します。
- 空白を含むファイル名は `-I "my survey\datalist.mb-1"` のように引用符で囲んでください。

## 関連項目

`mbareaclean`、`mbvoxelclean`、`mbprocess`、`mbdumpesf`、`mbgetesf`、`mbinfo`、`mbpreprocess`。完全なリファレンスは MB-System のマニュアルページ mbclean(1)（英語）です。
