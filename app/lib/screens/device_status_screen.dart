import 'package:flutter/material.dart';

import '../services/node_registry_service.dart';

class DeviceStatusScreen extends StatefulWidget {
  const DeviceStatusScreen({super.key});

  @override
  State<DeviceStatusScreen> createState() => _DeviceStatusScreenState();
}

class _DeviceStatusScreenState extends State<DeviceStatusScreen> {
  final NodeRegistryService _nodeRegistry = NodeRegistryService();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('LilyGO Device Status'),
        backgroundColor: Colors.deepOrange,
      ),
      body: AnimatedBuilder(
        animation: _nodeRegistry.nodesListenable,
        builder: (context, _) {
          final now = DateTime.now();
          final nodes = _nodeRegistry.nodesSnapshot;

          final online = nodes
              .where((node) => node.statusAt(now) == NodePresenceStatus.online)
              .toList();
          final inactive = nodes
              .where(
                  (node) => node.statusAt(now) == NodePresenceStatus.inactive)
              .toList();
          final offline = nodes
              .where((node) => node.statusAt(now) == NodePresenceStatus.offline)
              .toList();

          if (nodes.isEmpty) {
            return const Center(
              child: Padding(
                padding: EdgeInsets.all(24),
                child: Text(
                  'No node activity yet. Once RX or LOC frames arrive, LilyGO modules appear here.',
                  textAlign: TextAlign.center,
                ),
              ),
            );
          }

          return ListView(
            padding: const EdgeInsets.all(12),
            children: [
              _buildHeaderCounts(
                  online.length, inactive.length, offline.length),
              const SizedBox(height: 12),
              _buildSection(
                title: 'Online',
                color: Colors.green,
                items: online,
                now: now,
              ),
              _buildSection(
                title: 'Inactive',
                color: Colors.orange,
                items: inactive,
                now: now,
              ),
              _buildSection(
                title: 'Offline',
                color: Colors.grey,
                items: offline,
                now: now,
              ),
            ],
          );
        },
      ),
    );
  }

  Widget _buildHeaderCounts(int online, int inactive, int offline) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Wrap(
          runSpacing: 8,
          spacing: 12,
          children: [
            _buildCountBadge('Online', online, Colors.green),
            _buildCountBadge('Inactive', inactive, Colors.orange),
            _buildCountBadge('Offline', offline, Colors.grey),
          ],
        ),
      ),
    );
  }

  Widget _buildCountBadge(String label, int count, Color color) {
    return Chip(
      avatar: CircleAvatar(
        backgroundColor: color,
        child: Text(
          '$count',
          style: const TextStyle(color: Colors.white, fontSize: 12),
        ),
      ),
      label: Text(label),
    );
  }

  Widget _buildSection({
    required String title,
    required Color color,
    required List<NodePresence> items,
    required DateTime now,
  }) {
    return Card(
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(Icons.circle, color: color, size: 12),
                const SizedBox(width: 8),
                Text(
                  '$title (${items.length})',
                  style: const TextStyle(fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const SizedBox(height: 8),
            if (items.isEmpty)
              const Text('No devices')
            else
              ...items.map((node) => _buildNodeTile(node, now)),
          ],
        ),
      ),
    );
  }

  Widget _buildNodeTile(NodePresence node, DateTime now) {
    return ListTile(
      contentPadding: EdgeInsets.zero,
      title: Text('${node.name} (${node.nodeId})'),
      subtitle: Text(
        'Last seen ${_formatAge(now.difference(node.lastSeen))} ago\n'
        'RSSI: ${node.rssi?.toString() ?? '-'} dBm | '
        'SNR: ${node.snr?.toStringAsFixed(1) ?? '-'} | '
        'Location: ${node.hasLocation ? 'available' : 'missing'}\n'
        'Battery: ${node.batteryPercentage?.toString() ?? '-'}%'
        '${node.batteryVoltage != null ? ' (${node.batteryVoltage!.toStringAsFixed(2)} V)' : ''}',
      ),
      isThreeLine: true,
      leading: const Icon(Icons.memory),
    );
  }

  String _formatAge(Duration age) {
    if (age.inSeconds < 60) return '${age.inSeconds}s';
    if (age.inMinutes < 60) return '${age.inMinutes}m';
    return '${age.inHours}h';
  }
}
