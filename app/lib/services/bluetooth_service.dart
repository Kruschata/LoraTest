import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_bluetooth_serial_plus/flutter_bluetooth_serial_plus.dart';
import 'package:permission_handler/permission_handler.dart';

/// Bluetooth Classic / SPP bridge for the ESP32's BluetoothSerial service.
class BluetoothService {
  static final BluetoothService _instance = BluetoothService._internal();
  final FlutterBluetoothSerial _bluetooth = FlutterBluetoothSerial.instance;
  final StreamController<String> _lines =
      StreamController<String>.broadcast();

  BluetoothConnection? _connection;
  StreamSubscription<Uint8List>? _dataSubscription;
  String _inputBuffer = '';
  bool _isConnected = false;
  String? _lastError;

  factory BluetoothService() => _instance;

  BluetoothService._internal();

  bool get isConnected => _isConnected;
  String? get lastError => _lastError;

  Future<List<BluetoothDevice>> getAvailableDevices() async {
    await _ensureBluetoothPermissions();
    if (!await isBluetoothAvailable()) return [];
    if (!await isBluetoothEnabled()) {
      final enabled = await _bluetooth.requestEnable();
      if (enabled != true) return [];
    }
    return _bluetooth.getBondedDevices();
  }

  /// Android 12+ requires these permissions at runtime, not only in the
  /// manifest. Without CONNECT, getBondedDevices() silently returns nothing
  /// or throws on many phones.
  Future<void> _ensureBluetoothPermissions() async {
    final results = await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
    ].request();

    final denied = results.entries
        .where((entry) => !entry.value.isGranted)
        .map((entry) => entry.key.toString())
        .join(', ');
    if (denied.isNotEmpty) {
      throw StateError('Bluetooth permission denied: $denied');
    }
  }

  Future<bool> connect(BluetoothDevice device) async {
    try {
      _lastError = null;
      await _ensureBluetoothPermissions();
      await disconnect();

      for (var attempt = 0; attempt < 2; attempt++) {
        _connection = await BluetoothConnection.toAddress(device.address);

        // Some Android builds report a connect() success before the socket is
        // actually stable enough to keep the RFCOMM channel open.
        if (!_connection!.isConnected) {
          await _connection!.close();
          _connection = null;
          if (attempt == 0) {
            await Future<void>.delayed(const Duration(milliseconds: 250));
            continue;
          }
          throw StateError('Bluetooth socket disconnected immediately after connect');
        }

        _inputBuffer = '';
        _dataSubscription = _connection!.input.listen(
          _onData,
          onError: _lines.addError,
          onDone: () {
            _isConnected = false;
            _lines.add('STATUS|INFO|Bluetooth connection closed');
          },
        );

        _isConnected = true;
        return true;
      }

      return false;
    } catch (error) {
      _isConnected = false;
      _lastError = error.toString();
      return false;
    }
  }

  Future<void> disconnect() async {
    await _dataSubscription?.cancel();
    _dataSubscription = null;
    await _connection?.close();
    _connection = null;
    _isConnected = false;
  }

  Future<void> sendMessage(String message) async {
    if (_connection == null || !_isConnected) {
      throw StateError('Bluetooth not connected');
    }
    final text = message.trim();
    if (text.isEmpty) return;

    // ESP firmware accepts one CHAT command per newline.
    _connection!.output.add(Uint8List.fromList(utf8.encode('CHAT|$text\n')));
    await _connection!.output.allSent;
  }

  void _onData(Uint8List data) {
    _inputBuffer += utf8.decode(data, allowMalformed: true);
    final lines = _inputBuffer.split('\n');
    _inputBuffer = lines.removeLast();
    for (final line in lines) {
      final trimmed = line.trim();
      if (trimmed.isNotEmpty) _lines.add(trimmed);
    }
  }

  /// Complete protocol lines emitted by the ESP, never arbitrary chunks.
  Stream<String> getDataStream() => _lines.stream;

  Future<bool> isBluetoothAvailable() async =>
      await _bluetooth.isAvailable ?? false;

  Future<bool> isBluetoothEnabled() async =>
      await _bluetooth.isEnabled ?? false;
}
