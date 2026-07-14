# zmk-input-processor-gesture（試作・実機未検証）

トラックボールを一定方向に一定距離「弾く」動作を検知して、指定したキー動作（例: `&kp C_VOL_UP`）
を1回送るための、ZMK用カスタムinput-processorです。

**2026-07-14時点でビルド・書き込み・実機動作の確認は一切行っていません。** ZMK公式の
scaler / behaviors input-processor のソースを参考に書いた試作コードです。実際に
`west build` してこの機種に書き込み、動作を確認してから使ってください。

## これは何をするものか

Skinner39 Studio（設定アプリ）のトラックボール設定画面にある「ジェスチャーショートカット」
（上下左右のキー・発火するレイヤー・発火距離）は、今まで `.keymap` ファイルの中の**コメント**
としてしか保存されておらず、実際にはボールを弾いても何も起きませんでした。このモジュールは、
その設定を実際に動く形にするためのものです。

## 仕組み

- X軸用・Y軸用、それぞれ1つずつインスタンスを作る（`zip_gesture_x` / `zip_gesture_y`）
- ボールの動きを累積し、`threshold`（既定40）を超えたら、負方向/正方向に割り当てた
  `bindings` のどちらかを1回だけ押して離す
- その軸のイベントは常に「消費」する（＝このプロセッサを通した軸はカーソル移動としては
  伝わらない）。専用のジェスチャーレイヤーで使うことを想定

Skinner39 Studioの「⤓」でエクスポートした `trackball.overlay` には、以下のような内容が
自動生成されます（ジェスチャーの方向キーを1つ以上設定した場合のみ）:

```dts
zip_gesture_x: zip_gesture_x {
    compatible = "zmk,input-processor-gesture";
    #input-processor-cells = <0>;
    type = <INPUT_EV_REL>;
    code = <INPUT_REL_X>;
    threshold = <40>;
    bindings = <&kp C_LEFT>, <&kp C_RIGHT>;
};

zip_gesture_y: zip_gesture_y {
    compatible = "zmk,input-processor-gesture";
    #input-processor-cells = <0>;
    type = <INPUT_EV_REL>;
    code = <INPUT_REL_Y>;
    threshold = <40>;
    bindings = <&kp C_UP>, <&kp C_DOWN>;
};

trackball_listener_gesture {
    compatible = "zmk,input-listener";
    device = <&trackball>;
    layers = <4>;  // ConfigPanelの「ジェスチャーショートカット」Layer欄の値
    input-processors = <&zip_gesture_x>, <&zip_gesture_y>;
};
```

## 組み込み方

このフォルダ（`config/modules/zmk-input-processor-gesture/`）と
`config/zephyr/module.yml` を追加するだけで、GitHub Actionsのビルドに自動で
組み込まれます（`config/` 自体がwestの `self` プロジェクトとしてモジュール登録される仕組み）。
`west.yml` の変更は不要です。

## 実機検証でチェックしてほしいこと

- ビルドが通るか（Kconfig・devicetreeの構文ミスがないか）
- ボールを弾いた時に本当にキーが送信されるか
- 意図しない方向で誤発火しないか（`threshold` の値を調整する余地あり）
- OS側でキー入力として正しく認識されるか（もし反応しない場合、
  `src/input_processor_gesture.c` の press/release invoke の間に
  短い `k_msleep()` を挟む必要があるかもしれません）

## 関連

- ZMK公式の [Input Processors](https://zmk.dev/docs/keymaps/input-processors) ドキュメント
- 同じ考え方の公式プロセッサ: [Scaler](https://zmk.dev/docs/keymaps/input-processors/scaler)
  （remainderの累積方式を参考にした）、
  [Behaviors](https://zmk.dev/docs/keymaps/input-processors/behaviors)
  （bindings呼び出し方式を参考にした）
