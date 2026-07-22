import 'package:flutter/material.dart';
import '../models/device.dart';
import '../services/websocket_service.dart';
import 'chat_screen.dart';

class DevicesScreen extends StatefulWidget {
  final WebSocketService webSocketService;

  const DevicesScreen({
    Key? key,
    required this.webSocketService,
  }) : super(key: key);

  @override
  State<DevicesScreen> createState() => _DevicesScreenState();
}

class _DevicesScreenState extends State<DevicesScreen> {
  final List<Device> _devices = [
    Device(
      deviceID: '1',
      deviceName: 'Device 1',
      isOnline: true,
      rssi: -50,
      mode: 'connectivity',
    ),
    Device(
      deviceID: '2',
      deviceName: 'Device 2',
      isOnline: true,
      rssi: -75,
      mode: 'blackout',
    ),
    Device(
      deviceID: '3',
      deviceName: 'Device 3',
      isOnline: false,
      rssi: -120,
      mode: 'connectivity',
    ),
  ];

  @override
  void initState() {
    super.initState();
    _loadDevices();
  }

  void _loadDevices() {
    try {
      widget.webSocketService.requestDeviceList();
    } catch (e) {
      print('Error requesting devices: $e');
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('BlackoutBuddy - Devices'),
        backgroundColor: Colors.deepPurple,
      ),
      body: _devices.isEmpty
          ? const Center(
              child: Text('No devices available'),
            )
          : ListView.builder(
              itemCount: _devices.length,
              itemBuilder: (context, index) {
                final device = _devices[index];
                return ListTile(
                  leading: Icon(
                    device.isOnline ? Icons.radio_button_on : Icons.radio_button_off,
                    color: device.isOnline ? Colors.green : Colors.grey,
                  ),
                  title: Text(device.deviceName),
                  subtitle: Text(
                    'ID: ${device.deviceID} | Mode: ${device.mode} | RSSI: ${device.rssi} dBm',
                  ),
                  trailing: Icon(
                    device.mode == 'blackout'
                        ? Icons.cloud_off
                        : Icons.cloud_done,
                    color: device.mode == 'blackout'
                        ? Colors.orange
                        : Colors.blue,
                  ),
                  onTap: () {
                    Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (context) => ChatScreen(
                          webSocketService: widget.webSocketService,
                          selectedDevice: device,
                        ),
                      ),
                    );
                  },
                );
              },
            ),
      floatingActionButton: FloatingActionButton(
        onPressed: _loadDevices,
        child: const Icon(Icons.refresh),
      ),
    );
  }
}
