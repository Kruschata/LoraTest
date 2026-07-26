# Non-Blackout Mode + TTN Plan

This document defines the planned architecture and rollout for adding a dedicated Non-Blackout mode to BlackoutBuddy while preserving existing Blackout functionality.

## Goal

Add a second operational mode alongside current Blackout mode:
- Blackout mode: existing Bluetooth -> ESP -> LoRa flow
- Non-Blackout mode: utility-first experience with TTN-assisted connectivity
- Auto mode: switches between both based on connectivity and gateway health

## Scope

Included:
- Flutter app updates
- ESP firmware updates
- TTN-oriented integration path
- Manual mode switching and Auto mode
- Backward compatibility with existing protocol frames

Excluded in first release:
- Full multi-hop mesh redesign
- End-to-end guaranteed delivery across mixed transports
- Large new backend stack

## Implementation Phases

1. Define mode architecture
- Introduce canonical mode values: blackout, non_blackout, auto
- Centralize mode state in app provider/service
- Define behavior contract for each mode

2. App navigation and UX
- Extend mode selection screen with manual + auto controls
- Keep existing blackout screens unchanged
- Add a new non-blackout home with utility cards
- Add a shared status strip for transport and network health

3. Protocol and service abstraction
- Refactor app command layer to be transport-aware
- Keep TX, RX, STATUS compatible
- Add additive protocol frames:
  - MODE
  - CAPS
  - NET
- Extend parser to handle new frame families safely

4. ESP firmware mode support
- Add mode state machine with safe default blackout
- Add MODE command handling and transition status reports
- Add TTN/gateway transport layer behind a clean interface
- Add retry/fallback policy for auto mode

5. Non-Blackout utility features
- Gateway monitor
- Node health dashboard
- Incident alerts
- Message journal with search/export
- TTN event history and diagnostics

6. Auto mode safeguards
- Prefer non-blackout when gateway and internet are healthy
- Fall back to blackout when unstable/unavailable
- Add hysteresis/cooldown to prevent mode flapping
- Add manual override lock window

7. Validation and rollout
- Unit tests for parser, resolver, transport selection
- Integration tests for protocol compatibility and fallback
- Bench testing for connectivity edge cases
- Release behind feature flag, keep blackout default initially

## Recommended Non-Blackout Features

High-value additions for first release:
- Gateway status and packet-rate visibility
- Node battery and signal trend health panels
- Alerting for offline/low battery/poor link
- Searchable message timeline with export
- TTN uplink/downlink event history

## File Touchpoints (Planned)

- app/lib/screens/connection_mode_screen.dart
- app/lib/screens/blackoutMode_connection_screen.dart
- app/lib/services/bluetooth_service.dart
- app/lib/models/device.dart
- app/lib/main.dart
- esp/src/main.cpp
- esp/README.md
- BLACKOUTBUDDY_IMPLEMENTATION_PLAN.md

## Verification Checklist

- flutter analyze
- flutter test
- Old app <-> new firmware compatibility check
- New app <-> old firmware compatibility check
- MODE transition verification over serial/Bluetooth
- TTN visibility and utility panel reflection checks
- Auto-mode stability under network loss/recovery

## Recommended Decisions

- TTN integration depth: Option B (uplink + controlled downlink)
- Auth strategy: per-device tokens
- Data retention: local history + periodic cloud sync