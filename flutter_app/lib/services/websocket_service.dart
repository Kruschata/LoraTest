import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:web_socket_channel/status.dart' as status;
import 'dart:convert';

class WebSocketService {
  late WebSocketChannel _channel;
  final String serverUrl;
  bool _isConnected = false;

  WebSocketService({required this.serverUrl});

  bool get isConnected => _isConnected;

  Future<void> connect() async {
    try {
      _channel = WebSocketChannel.connect(
        Uri.parse(serverUrl),
      );
      _isConnected = true;
      print('WebSocket connected to $serverUrl');
    } catch (e) {
      print('WebSocket connection error: $e');
      _isConnected = false;
      throw Exception('Failed to connect to server: $e');
    }
  }

  void disconnect() {
    if (_isConnected) {
      _channel.sink.close(status.goingAway);
      _isConnected = false;
    }
  }

  Stream<dynamic> getMessageStream() {
    return _channel.stream;
  }

  void sendMessage(String sender, String senderID, String content, {String? receiver}) {
    final message = {
      'type': 'message',
      'sender': sender,
      'senderID': senderID,
      'content': content,
      'receiver': receiver,
      'timestamp': DateTime.now().toIso8601String(),
    };

    if (_isConnected) {
      _channel.sink.add(jsonEncode(message));
      print('Message sent: $message');
    } else {
      throw Exception('WebSocket not connected');
    }
  }

  void sendDeviceInfo(String deviceID, String deviceName, String mode) {
    final message = {
      'type': 'device_info',
      'deviceID': deviceID,
      'deviceName': deviceName,
      'mode': mode,
      'timestamp': DateTime.now().toIso8601String(),
    };

    if (_isConnected) {
      _channel.sink.add(jsonEncode(message));
    }
  }

  void requestDeviceList() {
    final message = {
      'type': 'request_devices',
      'timestamp': DateTime.now().toIso8601String(),
    };

    if (_isConnected) {
      _channel.sink.add(jsonEncode(message));
    }
  }
}
