import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:blackout_buddy/main.dart';
import 'package:blackout_buddy/screens/blackoutMode_connection_screen.dart';
import 'package:blackout_buddy/services/bluetooth_service.dart';

void main() {
  testWidgets('connects to a device before choosing a mode',
      (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());

    expect(find.text('Connect via Bluetooth'), findsOneWidget);
    expect(find.text('Choose Connection Mode'), findsNothing);
  });

  testWidgets('chat screen exposes a dedicated status messages action', (
    WidgetTester tester,
  ) async {
    await tester.pumpWidget(
      MaterialApp(
        home: BluetoothChatScreen(
          bluetoothService: BluetoothService(),
          deviceName: 'LoRaChat-2',
        ),
      ),
    );

    expect(find.byTooltip('Status messages'), findsOneWidget);
  });
}
