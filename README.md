# BandHeartBeat

Самостоятельная сборка без OBS: `dist/standalone/BandHeartBeat.exe`. Нативное меню камер, установка/откат и ограничения описаны в [инструкции](directshow-camera/STANDALONE-RU.md). Сборка пакета: `cmd /c directshow-camera\package-msvc.cmd`. Ниже сохранён запасной OBS-путь.

Lightweight native Windows heart-rate monitor. Use a device broadcasting the standard Bluetooth SIG Heart Rate Service (0x180D), keep Windows Bluetooth on, then open `target/release/band-heart-beat.exe`.

The compact dark window shows real BLE heart-rate measurements. Closing or minimizing hides it in the notification area. Left-click its tray icon to show it; right-click for **Показать**, **Повторить подключение**, or **Выйти**. Retry starts a fresh scan after a failure; an active scan/connection is never duplicated. No readings for 20 seconds ends the connection and clears the value.

For an explicitly labeled UI preview without hardware: `band-heart-beat.exe --demo`. Demo shows a fixed example of 72 BPM and never claims a live connection. Normal mode never generates readings.

If no device is found, enable standard heart-rate broadcasting on a supported device and check that another host is not connected. **Huawei Band 10 compatibility and the availability of a broadcast setting are unconfirmed.**

## Build

Use Rust and Visual Studio C++ Build Tools: `cargo build --release`.
With the GNU Rust toolchain and MinGW installed: `rustup run stable-x86_64-pc-windows-gnu cargo build --release --offline`.
Both write `target/release/band-heart-beat.exe`. Run `cargo fmt --check` and `rustup run stable-x86_64-pc-windows-gnu cargo test --offline` for local checks.

No Huawei hardware was available for validation. Huawei Band 10 compatibility remains unconfirmed.

## Оверлей рядом с головой в Discord

1. Запустите приложение. Для проверки без устройства: `band-heart-beat.exe --demo` (показания явно помечены «ДЕМО»).
2. В OBS Studio добавьте источник **Устройство захвата видео** и выберите свою веб-камеру.
3. Добавьте поверх камеры источник **Браузер**: URL `http://127.0.0.1:8765/overlay`, ширина 480, высота 240. Фон страницы прозрачный. В трее приложения есть пункт «Открыть оверлей OBS» для просмотра.
4. Перетащите оверлей рядом с головой, измените размер при необходимости. Положение задаётся вручную в OBS; отслеживания лица нет.
5. Нажмите **Запустить виртуальную камеру** в OBS. В настройках видео Discord выберите **OBS Virtual Camera**. Камеру использует OBS, а Discord получает готовую композицию.

Держите OBS и BandHeartBeat запущенными. При отключении браслета показание меняется на тире, а анимация останавливается. Повтор подключения — через трей. Движение сердца рассчитывается по BPM и служит визуализацией частоты, а не измеренным временем отдельных ударов. В демо сердце анимировано по фиксированному примеру 72 BPM.

Сервер слушает только `127.0.0.1:8765`, отдаёт встроенную страницу и SSE `/events`, не читает файлы компьютера. Не более 8 подключений одновременно. Если порт занят, приложение показывает предупреждение: закройте другой экземпляр/программу на этом порту и перезапустите BandHeartBeat. В OBS можно отключить источник, когда он не виден, чтобы остановить браузерную анимацию.
