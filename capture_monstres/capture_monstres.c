name: "FAP: Build"
on: [push, pull_request]
jobs:
  ufbt-build-action:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      - uses: flipperdevices/flipperzero-ufbt-action@v0.1
        id: build-app
        with:
          app-dir: capture_monstres
          sdk-url: https://github.com/Next-Flip/Momentum-Firmware.git
          sdk-hw-target: f7
      - uses: actions/upload-artifact@v4
        with:
          name: capture_monstres-fap
          path: ${{ steps.build-app.outputs.fap-artifacts }}
