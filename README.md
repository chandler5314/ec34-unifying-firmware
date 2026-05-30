# EC34 Unifying / Bolt Firmware Research

This repository tracks EC34 keyboard firmware research and bring-up work.

Current focus:

- Logitech Unifying connectivity for the EC34 keyboard.
- nRF52840 proprietary 2.4 GHz radio bring-up.
- Host-side Logitech HID++ receiver inspection tools.

Bolt work is intentionally parked until a Bolt receiver is available for testing.

## Layout

- `unifying-prototype/`
  - Portable Unifying protocol core.
  - Pairing frame builders, response parsers, keep-alive and wake-up helpers.
  - Optional Zephyr/NCS ESB adapter.
- `firmware/ec34u_unifying_bringup/`
  - Minimal nRF Connect SDK application for nRF52840 radio bring-up.
- `tools/`
  - Windows PowerShell HID++ receiver inspection tools.
- `*.md`
  - Chinese development notes and feasibility reports.

## Safety Boundary

This work targets normal pairing with the owner's own Logitech Unifying receiver.
It does not implement forced pairing, key sniffing, or injection against unknown receivers.

## GitHub Actions

Two build paths are provided:

- Host protocol check: compiles the portable C protocol core on Ubuntu.
- Manual NCS bring-up build: checks the nRF52840 ESB application with nRF Connect SDK.

The NCS job is manual because it downloads a large SDK workspace.
