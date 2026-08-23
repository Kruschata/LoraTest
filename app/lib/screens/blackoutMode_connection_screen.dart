import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial_plus/flutter_bluetooth_serial_plus.dart';
import 'dart:async';
import '../services/bluetooth_service.dart';
import '../services/node_registry_service.dart';
import 'device_map_screen.dart';
import 'device_status_screen.dart';

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
  final List<_BluetoothStatusLine> _statusMessages = [];
  final NodeRegistryService _nodeRegistry = NodeRegistryService();
  StreamSubscription<String>? _dataSubscription;

  List<_BluetoothStatusLine> get unreadStatusMessages =>
      _statusMessages.where((message) => !message.isRead).toList();

  List<_BluetoothStatusLine> get allStatusMessages => _statusMessages;

  @override
  void initState() {
    super.initState();
    _setupDataListener();
  }

  void _upsertStatusMessage(_BluetoothStatusLine statusLine) {
    final index = _statusMessages.indexWhere(
      (existing) => existing.category == statusLine.category,
    );

    if (index >= 0) {
      _statusMessages[index] = statusLine;
      return;
    }

    _statusMessages.insert(0, statusLine);
  }

  void _markStatusMessagesRead() {
    if (!mounted) return;
    setState(() {
      for (final message in _statusMessages) {
        message.isRead = true;
      }
    });
  }

  void _setupDataListener() {
    _dataSubscription = widget.bluetoothService.getDataStream().listen(
      (data) {
        if (!mounted) return;
        _nodeRegistry.ingestProtocolLine(data);

        final statusLine = _BluetoothStatusLine.parse(data);
        if (statusLine != null) {
          setState(() {
            _upsertStatusMessage(statusLine);
          });
        }

        final line = _BluetoothChatLine.parse(data);
        if (line != null) {
          setState(() => _messages.insert(0, line));
        }
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
    final statusCount = allStatusMessages.length;

    return Scaffold(
      appBar: AppBar(
        title: Text('Bluetooth: ${widget.deviceName}'),
        backgroundColor: Colors.deepPurple.shade700,
        foregroundColor: Colors.white,
        actions: [
          Stack(
            alignment: Alignment.center,
            clipBehavior: Clip.none,
            children: [
              IconButton(
                tooltip: 'Status messages',
                icon: const Icon(Icons.info_outline),
                onPressed: () {
                  _markStatusMessagesRead();
                  showModalBottomSheet(
                    context: context,
                    isScrollControlled: true,
                    builder: (context) => _StatusMessagesSheet(
                      messages: allStatusMessages,
                    ),
                  );
                },
              ),
              if (statusCount > 0)
                Positioned(
                  right: -3,
                  top: -2,
                  child: Container(
                    padding: const EdgeInsets.symmetric(horizontal: 5, vertical: 2),
                    constraints: const BoxConstraints(minWidth: 16, minHeight: 16),
                    decoration: const BoxDecoration(
                      color: Colors.orange,
                      shape: BoxShape.circle,
                    ),
                    child: Text(
                      statusCount.toString(),
                      style: const TextStyle(
                        color: Colors.white,
                        fontSize: 9,
                        fontWeight: FontWeight.bold,
                      ),
                      textAlign: TextAlign.center,
                    ),
                  ),
                ),
            ],
          ),
          IconButton(
            tooltip: 'Device list',
            icon: const Icon(Icons.devices),
            onPressed: () {
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => const DeviceStatusScreen(),
                ),
              );
            },
          ),
          IconButton(
            tooltip: 'Device map',
            icon: const Icon(Icons.map),
            onPressed: () {
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => const DeviceMapScreen(),
                ),
              );
            },
          ),
        ],
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

class _BluetoothStatusLine {
  final String category;
  final String message;
  final DateTime timestamp;
  bool isRead;

  _BluetoothStatusLine(this.category, this.message, this.timestamp, {this.isRead = false});

  static _BluetoothStatusLine? parse(String line) {
    final parts = line.split('|');
    if (parts.isEmpty || parts.first != 'STATUS') return null;
    if (parts.length < 3) return null;
    return _BluetoothStatusLine(
      parts[1],
      parts.sublist(2).join(' | '),
      DateTime.now(),
    );
  }
}

class _BluetoothChatLine {
  final String label;
  final String text;

  const _BluetoothChatLine(this.label, this.text);

  static _BluetoothChatLine? parse(String line) {
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
    if (parts.length >= 5 && parts.first == 'LOC') {
      // Location frames update the registry and are not shown in chat.
      return null;
    }
    if (parts.isNotEmpty && parts.first == 'STATUS') {
      return null;
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

class _StatusMessagesSheet extends StatelessWidget {
  final List<_BluetoothStatusLine> messages;

  const _StatusMessagesSheet({required this.messages});

  @override
  Widget build(BuildContext context) {
    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Expanded(
                  child: Text(
                    'Status messages',
                    style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                  ),
                ),
                IconButton(
                  icon: const Icon(Icons.close),
                  onPressed: () => Navigator.pop(context),
                ),
              ],
            ),
            const SizedBox(height: 8),
            if (messages.isEmpty)
              const Padding(
                padding: EdgeInsets.symmetric(vertical: 12),
                child: Text('No status messages yet.'),
              )
            else
              Flexible(
                child: ListView.builder(
                  shrinkWrap: true,
                  itemCount: messages.length,
                  itemBuilder: (context, index) {
                    final message = messages[index];
                    return ListTile(
                      leading: const Icon(Icons.info_outline, color: Colors.deepOrange),
                      title: Text(message.category),
                      subtitle: Text(message.message),
                      trailing: Text(
                        '${message.timestamp.hour.toString().padLeft(2, '0')}:${message.timestamp.minute.toString().padLeft(2, '0')}',
                      ),
                    );
                  },
                ),
              ),
          ],
        ),
      ),
    );
  }
}
