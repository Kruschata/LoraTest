import 'dart:async';

import 'package:flutter/material.dart';

import '../services/bluetooth_service.dart';
import '../services/node_registry_service.dart';
import 'device_map_screen.dart';
import 'device_status_screen.dart';

class NonBlackoutHomeScreen extends StatefulWidget {
  final BluetoothService bluetoothService;
  final String deviceName;

  const NonBlackoutHomeScreen({
    super.key,
    required this.bluetoothService,
    required this.deviceName,
  });

  @override
  State<NonBlackoutHomeScreen> createState() => _NonBlackoutHomeScreenState();
}

class _NonBlackoutHomeScreenState extends State<NonBlackoutHomeScreen> {
  final NodeRegistryService _nodeRegistry = NodeRegistryService();
  final TextEditingController _messageController = TextEditingController();
  StreamSubscription<String>? _dataSubscription;
  String _ttnStatus = 'WAITING';
  String? _lastTtnMessage;

  @override
  void initState() {
    super.initState();
    _dataSubscription = widget.bluetoothService.getDataStream().listen(
      _handleDeviceLine,
      onError: (Object error) {
        if (mounted) setState(() => _ttnStatus = 'DEVICE ERROR');
      },
    );
  }

  void _handleDeviceLine(String line) {
    _nodeRegistry.ingestProtocolLine(line);
    if (!line.startsWith('TTN|')) return;
    final fields = line.split('|');
    if (fields.length < 2 || !mounted) return;
    setState(() {
      _ttnStatus = fields[1].replaceAll('_', ' ');
      if (fields[1] == 'DOWNLINK' && fields.length > 2) {
        _lastTtnMessage = fields.skip(2).join('|');
      }
    });
  }

  Future<void> _sendTtnMessage() async {
    final text = _messageController.text.trim();
    if (text.isEmpty) return;
    try {
      await widget.bluetoothService.sendCommand('TTN|SEND|$text');
      _messageController.clear();
      if (mounted) setState(() => _ttnStatus = 'UPLINK QUEUED');
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('TTN message failed: $error')),
      );
    }
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
        title: Text('TTN · ${widget.deviceName}'),
        backgroundColor: Colors.indigo,
      ),
      body: AnimatedBuilder(
        animation: _nodeRegistry.nodesListenable,
        builder: (context, _) {
          final nodes = _nodeRegistry.nodesSnapshot;
          final now = DateTime.now();
          final online = nodes
              .where((node) => node.statusAt(now) == NodePresenceStatus.online)
              .length;

          return ListView(
            padding: const EdgeInsets.all(16),
            children: [
              _buildTransportStatus(context),
              const SizedBox(height: 12),
              _buildTtnMessageComposer(),
              const SizedBox(height: 16),
              Text(
                'Network workspace',
                style: Theme.of(context).textTheme.titleLarge,
              ),
              const SizedBox(height: 10),
              _buildUtilityCard(
                context,
                icon: Icons.memory,
                title: 'Node health',
                subtitle: nodes.isEmpty
                    ? 'No gateway telemetry received yet'
                    : '$online online of ${nodes.length} known nodes',
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => const DeviceStatusScreen(),
                  ),
                ),
              ),
              _buildUtilityCard(
                context,
                icon: Icons.map_outlined,
                title: 'Live map',
                subtitle: 'View reported node locations',
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (context) => const DeviceMapScreen(),
                  ),
                ),
              ),
              _buildUtilityCard(
                context,
                icon: Icons.history,
                title: 'Event history',
                subtitle: 'TTN event history will appear here',
                onTap: () => _showUnavailableMessage(context),
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildTransportStatus(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Icon(
              Icons.cloud_queue,
              color: _ttnStatus == 'JOINED' ? Colors.green : Colors.orange,
              size: 30,
            ),
            const SizedBox(width: 12),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'Gateway status',
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                  const SizedBox(height: 4),
                  Text('Device: ${widget.deviceName}'),
                ],
              ),
            ),
            Chip(
              label: Text(_ttnStatus),
              avatar: Icon(
                Icons.circle,
                color: _ttnStatus == 'JOINED' ? Colors.green : Colors.orange,
                size: 10,
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildTtnMessageComposer() {
    return Column(
      children: [
        Row(
          children: [
            Expanded(
              child: TextField(
                controller: _messageController,
                maxLength: 50,
                textInputAction: TextInputAction.send,
                onSubmitted: (_) => _sendTtnMessage(),
                decoration: const InputDecoration(
                  labelText: 'TTN uplink message',
                  border: OutlineInputBorder(),
                  counterText: '',
                ),
              ),
            ),
            const SizedBox(width: 8),
            IconButton.filled(
              tooltip: 'Send via TTN',
              onPressed: _sendTtnMessage,
              icon: const Icon(Icons.send),
            ),
          ],
        ),
        if (_lastTtnMessage != null)
          ListTile(
            leading: const Icon(Icons.south_west),
            title: const Text('Latest TTN downlink'),
            subtitle: Text(_lastTtnMessage!),
          ),
      ],
    );
  }

  Widget _buildUtilityCard(
    BuildContext context, {
    required IconData icon,
    required String title,
    required String subtitle,
    required VoidCallback onTap,
  }) {
    return Card(
      margin: const EdgeInsets.only(bottom: 10),
      child: ListTile(
        leading: Icon(icon, color: Colors.indigo),
        title: Text(title),
        subtitle: Text(subtitle),
        trailing: const Icon(Icons.chevron_right),
        onTap: onTap,
      ),
    );
  }

  void _showUnavailableMessage(BuildContext context) {
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content:
            Text('Event history is available once a gateway is connected.'),
      ),
    );
  }
}
