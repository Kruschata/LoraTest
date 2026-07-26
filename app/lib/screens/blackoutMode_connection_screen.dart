import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial_plus/flutter_bluetooth_serial_plus.dart';
import 'dart:async';
import '../services/bluetooth_service.dart';

class BluetoothConnectionScreen extends StatefulWidget {
  const BluetoothConnectionScreen({Key? key}) : super(key: key);

  @override
  State<BluetoothConnectionScreen> createState() =>
      _BluetoothConnectionScreenState();
}

class _BluetoothConnectionScreenState extends State<BluetoothConnectionScreen> {
  final BluetoothService _bluetoothService = BluetoothService();
  List<BluetoothDevice> _devices = [];
  bool _isLoading = false;

  @override
  void initState() {
    super.initState();
    _loadDevices();
  }

  void _loadDevices() async {
    if (!mounted) return;
    setState(() {
      _isLoading = true;
    });

    try {
      final devices = await _bluetoothService.getAvailableDevices();
      if (mounted) {
        setState(() {
          // Show every paired device. Some Android versions initially return
          // no friendly name, so filtering on "LoRa" hid a valid ESP.
          _devices = devices;
        });
      }
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error loading devices: $e')),
      );
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  void _connectToDevice(BluetoothDevice device) async {
    if (!mounted) return;
    setState(() {
      _isLoading = true;
    });

    try {
      final success = await _bluetoothService.connect(device);

      if (!mounted) return;

      if (success) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Connected to ${device.name}')),
        );

        // Navigate to chat screen with Bluetooth connection
        Navigator.pushReplacement(
          context,
          MaterialPageRoute(
            builder: (context) => BluetoothChatScreen(
              bluetoothService: _bluetoothService,
              deviceName: device.name ?? 'Unknown Device',
            ),
          ),
        );
      } else {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              'Failed to connect: ${_bluetoothService.lastError ?? 'unknown error'}',
            ),
          ),
        );
      }
    } catch (e) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Error: $e')),
      );
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Connect via Bluetooth'),
        backgroundColor: Colors.deepPurple,
      ),
      body: _isLoading
          ? const Center(child: CircularProgressIndicator())
          : Column(
              children: [
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Text(
                    'Paired Bluetooth Devices',
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                ),
                if (_devices.isEmpty)
                  const Padding(
                    padding: EdgeInsets.all(16.0),
                    child: Text(
                      'No paired devices found. Pair LoRaChat-2 first in the Android Bluetooth settings, then tap Refresh.',
                    ),
                  )
                else
                  Expanded(
                    child: ListView.builder(
                      itemCount: _devices.length,
                      itemBuilder: (context, index) {
                        final device = _devices[index];
                        return ListTile(
                          leading: const Icon(Icons.bluetooth),
                          title: Text(device.name ?? 'Unknown'),
                          subtitle: Text(device.address),
                          trailing: IconButton(
                            icon: const Icon(Icons.connect_without_contact),
                            onPressed: () => _connectToDevice(device),
                          ),
                          onTap: () => _connectToDevice(device),
                        );
                      },
                    ),
                  ),
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: ElevatedButton.icon(
                    onPressed: _loadDevices,
                    icon: const Icon(Icons.refresh),
                    label: const Text('Refresh Devices'),
                  ),
                ),
              ],
            ),
    );
  }
}

/// Chat Screen für Bluetooth-Verbindung
class BluetoothChatScreen extends StatefulWidget {
  final BluetoothService bluetoothService;
  final String deviceName;

  const BluetoothChatScreen({
    Key? key,
    required this.bluetoothService,
    required this.deviceName,
  }) : super(key: key);

  @override
  State<BluetoothChatScreen> createState() => _BluetoothChatScreenState();
}

class _BluetoothChatScreenState extends State<BluetoothChatScreen> {
  final TextEditingController _messageController = TextEditingController();
  final List<_BluetoothChatLine> _messages = [];
  StreamSubscription<String>? _dataSubscription;

  @override
  void initState() {
    super.initState();
    _setupDataListener();
  }

  void _setupDataListener() {
    _dataSubscription = widget.bluetoothService.getDataStream().listen(
      (data) {
        if (!mounted) return;
        setState(() => _messages.insert(0, _BluetoothChatLine.parse(data)));
      },
      onError: (error) {
        if (!mounted) return;
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Bluetooth error: $error')),
        );
      },
      onDone: () {
        if (!mounted) return;
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Bluetooth connection closed')),
        );
      },
    );
  }

  void _sendMessage() {
    if (_messageController.text.isEmpty) return;

    final text = _messageController.text;
    _messageController.clear();
    widget.bluetoothService.sendMessage(text).catchError((error) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Send failed: $error')),
        );
      }
    });
  }

  @override
  void dispose() {
    _dataSubscription?.cancel();
    _messageController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Bluetooth: ${widget.deviceName}'),
        backgroundColor: Colors.deepOrange,
      ),
      body: Column(
        children: [
          Expanded(
            child: _messages.isEmpty
                ? const Center(child: Text('No messages'))
                : ListView.builder(
                    reverse: true,
                    itemCount: _messages.length,
                    itemBuilder: (context, index) {
                      return Padding(
                        padding: const EdgeInsets.all(8.0),
                        child: Container(
                          padding: const EdgeInsets.all(12),
                          decoration: BoxDecoration(
                            color: Colors.grey[200],
                            borderRadius: BorderRadius.circular(8),
                          ),
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                _messages[index].label,
                                style: const TextStyle(
                                  fontWeight: FontWeight.bold,
                                  fontSize: 12,
                                ),
                              ),
                              const SizedBox(height: 3),
                              Text(_messages[index].text),
                            ],
                          ),
                        ),
                      );
                    },
                  ),
          ),
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 8),
            child: Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _messageController,
                    decoration: InputDecoration(
                      hintText: 'Type a message...',
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(20),
                      ),
                      contentPadding:
                          const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                    ),
                  ),
                ),
                const SizedBox(width: 8),
                FloatingActionButton(
                  onPressed: _sendMessage,
                  child: const Icon(Icons.send),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

class _BluetoothChatLine {
  final String label;
  final String text;

  const _BluetoothChatLine(this.label, this.text);

  factory _BluetoothChatLine.parse(String line) {
    final parts = line.split('|');
    if (parts.length >= 4 && parts.first == 'TX') {
      return _BluetoothChatLine('You', _unescape(parts.sublist(3).join('|')));
    }
    if (parts.length >= 6 && parts.first == 'RX') {
      return _BluetoothChatLine(
        'LoRa node ${parts[1]} (${parts[3]} dBm)',
        _unescape(parts.sublist(5).join('|')),
      );
    }
    if (parts.isNotEmpty && parts.first == 'STATUS') {
      return _BluetoothChatLine('Status', parts.skip(2).join(' | '));
    }
    return _BluetoothChatLine('ESP', line);
  }

  static String _unescape(String value) {
    final output = StringBuffer();
    var escaped = false;
    for (final rune in value.runes) {
      final char = String.fromCharCode(rune);
      if (escaped) {
        output.write(char);
        escaped = false;
      } else if (char == '\\') {
        escaped = true;
      } else {
        output.write(char);
      }
    }
    if (escaped) output.write('\\');
    return output.toString();
  }
}
