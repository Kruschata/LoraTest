import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:blackout_buddy/main.dart';
import 'package:blackout_buddy/screens/blackoutMode_connection_screen.dart';
import 'package:blackout_buddy/services/bluetooth_service.dart';

void main() {
  testWidgets('shows connection mode entry screen', (WidgetTester tester) async {
    await tester.pumpWidget(const MyApp());

    expect(find.text('BlackoutBuddy - Connection Mode'), findsOneWidget);
    expect(find.text('Choose Connection Mode'), findsOneWidget);
    expect(find.text('Blackout Mode'), findsOneWidget);
  });

  testWidgets('blackout mode card is tappable', (
    WidgetTester tester,
  ) async {
    await tester.pumpWidget(const MyApp());

    final blackoutCard = find.byWidgetPredicate(
      (widget) => widget is GestureDetector && widget.onTap != null,
    );
    expect(blackoutCard, findsOneWidget);
    final gesture = tester.widget<GestureDetector>(blackoutCard.first);
    expect(gesture.onTap, isNotNull);
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
