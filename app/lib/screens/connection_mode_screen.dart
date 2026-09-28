import 'package:flutter/material.dart';
import '../services/bluetooth_service.dart';
import 'blackoutMode_connection_screen.dart';
import 'non_blackout_home_screen.dart';

class ConnectionModeScreen extends StatelessWidget {
  final BluetoothService bluetoothService;
  final String deviceName;

  const ConnectionModeScreen({
    Key? key,
    required this.bluetoothService,
    required this.deviceName,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Connected: $deviceName'),
        backgroundColor: Colors.deepPurple,
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Icon(
              Icons.router,
              size: 64,
              color: Colors.deepPurple,
            ),
            const SizedBox(height: 32),
            const Text(
              'Choose Connection Mode',
              style: TextStyle(
                fontSize: 24,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 48),
            _buildConnectionCard(
              context,
              icon: Icons.bluetooth,
              title: 'Blackout Mode',
              subtitle: 'Communication only over LORA',
              description: 'Blackout mode - no internet needed',
              color: Colors.deepOrange,
              onTap: () => _openBlackoutMode(context),
            ),
            const SizedBox(height: 16),
            _buildConnectionCard(
              context,
              icon: Icons.cloud_outlined,
              title: 'Non-Blackout Mode',
              subtitle: 'Gateway-assisted connectivity',
              description: 'Use network services when a gateway is available',
              color: Colors.indigo,
              onTap: () => _openTtnMode(context),
            ),
          ],
        ),
      ),
    );
  }

  Future<void> _openBlackoutMode(BuildContext context) async {
    await _openMode(
      context,
      command: 'MODE|BLACKOUT',
      screen: BluetoothChatScreen(
        bluetoothService: bluetoothService,
        deviceName: deviceName,
      ),
    );
  }

  Future<void> _openTtnMode(BuildContext context) async {
    await _openMode(
      context,
      command: 'MODE|TTN',
      screen: NonBlackoutHomeScreen(
        bluetoothService: bluetoothService,
        deviceName: deviceName,
      ),
    );
  }

  Future<void> _openMode(
    BuildContext context, {
    required String command,
    required Widget screen,
  }) async {
    try {
      await bluetoothService.sendCommand(command);
      if (!context.mounted) return;
      Navigator.push(
        context,
        MaterialPageRoute(builder: (_) => screen),
      );
    } catch (error) {
      if (!context.mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Could not select mode: $error')),
      );
    }
  }

  Widget _buildConnectionCard(
    BuildContext context, {
    required IconData icon,
    required String title,
    required String subtitle,
    required String description,
    required Color color,
    required VoidCallback onTap,
  }) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: MediaQuery.of(context).size.width * 0.8,
        padding: const EdgeInsets.all(20),
        decoration: BoxDecoration(
          color: color.withValues(alpha: 0.1),
          border: Border.all(color: color, width: 2),
          borderRadius: BorderRadius.circular(12),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, size: 40, color: color),
                const SizedBox(width: 16),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        title,
                        style: TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.bold,
                          color: color,
                        ),
                      ),
                      Text(
                        subtitle,
                        style: const TextStyle(
                          fontSize: 12,
                          color: Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
            const SizedBox(height: 12),
            Text(
              description,
              style: const TextStyle(fontSize: 13),
            ),
          ],
        ),
      ),
    );
  }
}
