# KinKon #

https://youtu.be/i-j91lbzI2U?si=LUTt86hgS40Jx9ae
https://minkara.carview.co.jp/userid/3016335/car/2628711/8410751/note.aspx

KinKon (not KingKong) is a program that recreates a Japanese automotive safety feature from the 1980s.
Cars at that time sounded a warning chime when exceeding 105 km/h (Light motive was 85 km/h).

It runs on common RP2040 Zero compatible modules. (Including Raspberry Pi Pico, of course).

-Ja-

KinKon (not KingKong)は、1980年代の日本の自動車の安全装置を再現したプログラムです。
当時の車は、105km/h(85km/h)を超過すると、警告チャイムが鳴りました。

RP2040 Zeroの一般的な互換モジュールで動きます。(もちろん、Paspberry PI Picoも含みます)。

# Build and Installation #

(0)
Prepare the development environment for Raspberry Pi Pico.
I used Visual Studio Code.

(1)
In the libs directory,
git clone https://github.com/daschr/pico-ssd1306
to prepare the libraries.

(2)
Build to create the .uf2 binary.

(3)
Copy the .uf2 file to the target RP2040 Zero.

(4)
Copy resource/001.wav to the root directory of the microSD card for the DFPlayer mini.

(5)
Connect the module and assemble it to complete the setup.

-Ja-

(0)
Raspberry PI Picoの開発環境を準備します。
私は、VisualStudio Codeを使用しました。

(1)
libsで
git clone https://github.com/daschr/pico-ssd1306
してライブラリを準備してください。

(2)
ビルドして、.uf2バイナリを作ります。

(3)
ターゲットのRP2040 Zeroへ.uf2をコピーします。

(4)
DFPlayer mini 用のmicroSDカードへ、reource/001.wavをカードのルートへコピーします。

(5)
モジュールを接続して、組み立てれば完成です。

[EOT]
