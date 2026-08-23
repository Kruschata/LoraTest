import 'package:flutter/material.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';

import '../services/node_registry_service.dart';

class DeviceMapScreen extends StatefulWidget {
  const DeviceMapScreen({super.key});

  @override
  State<DeviceMapScreen> createState() => _DeviceMapScreenState();
}

class _DeviceMapScreenState extends State<DeviceMapScreen> {
  final NodeRegistryService _nodeRegistry = NodeRegistryService();
  final MapController _mapController = MapController();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('LilyGO Map'),
        backgroundColor: Colors.deepOrange,
      ),
      body: AnimatedBuilder(
        animation: _nodeRegistry.nodesListenable,
        builder: (context, _) {
          final now = DateTime.now();
          final nodesWithLocation = _nodeRegistry.nodesSnapshot
              .where((node) => node.hasLocation)
              .toList();

          if (nodesWithLocation.isEmpty) {
            return const Center(
              child: Padding(
                padding: EdgeInsets.all(24),
                child: Text(
                  'No node locations yet. Waiting for LOC frames from LilyGO modules.',
                  textAlign: TextAlign.center,
                ),
              ),
            );
          }

          final center = LatLng(
            nodesWithLocation.first.latitude!,
            nodesWithLocation.first.longitude!,
          );

          final markers = nodesWithLocation.map((node) {
            final status = node.statusAt(now);
            return Marker(
              point: LatLng(node.latitude!, node.longitude!),
              width: 42,
              height: 42,
              child: GestureDetector(
                onTap: () => _showNodeBottomSheet(context, node, status),
                child: Icon(
                  Icons.location_on,
                  size: 38,
                  color: _statusColor(status),
                ),
              ),
            );
          }).toList();

          return FlutterMap(
            mapController: _mapController,
            options: MapOptions(
              initialCenter: center,
              initialZoom: 14,
              interactionOptions:
                  const InteractionOptions(flags: InteractiveFlag.all),
            ),
            children: [
              TileLayer(
                urlTemplate: 'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
                userAgentPackageName: 'com.blackoutbuddy.app',
              ),
              MarkerLayer(markers: markers),
            ],
          );
        },
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _zoomToAll,
        icon: const Icon(Icons.center_focus_strong),
        label: const Text('Center'),
      ),
    );
  }

  Color _statusColor(NodePresenceStatus status) {
    switch (status) {
      case NodePresenceStatus.online:
        return Colors.green;
      case NodePresenceStatus.inactive:
        return Colors.orange;
      case NodePresenceStatus.offline:
        return Colors.grey;
    }
  }

  void _showNodeBottomSheet(
    BuildContext context,
    NodePresence node,
    NodePresenceStatus status,
  ) {
    showModalBottomSheet<void>(
      context: context,
      builder: (context) {
        return Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                '${node.name} (${node.nodeId})',
                style: Theme.of(context).textTheme.titleMedium,
              ),
              const SizedBox(height: 8),
              Text('Status: ${status.name}'),
              Text('Last seen: ${node.lastSeen.toLocal()}'),
              Text('Lat/Lon: ${node.latitude}, ${node.longitude}'),
              Text('Accuracy: ${node.accuracyMeters?.toStringAsFixed(1) ?? '-'} m'),
              Text('RSSI: ${node.rssi?.toString() ?? '-'} dBm'),
              Text('SNR: ${node.snr?.toStringAsFixed(1) ?? '-'}'),
            ],
          ),
        );
      },
    );
  }

  void _zoomToAll() {
    final withLocation = _nodeRegistry.nodesSnapshot
        .where((node) => node.hasLocation)
        .toList();
    if (withLocation.isEmpty) return;

    if (withLocation.length == 1) {
      _mapController.move(
        LatLng(withLocation.first.latitude!, withLocation.first.longitude!),
        15,
      );
      return;
    }

    final lats = withLocation.map((node) => node.latitude!).toList();
    final lons = withLocation.map((node) => node.longitude!).toList();

    final bounds = LatLngBounds(
      LatLng(lats.reduce((a, b) => a < b ? a : b), lons.reduce((a, b) => a < b ? a : b)),
      LatLng(lats.reduce((a, b) => a > b ? a : b), lons.reduce((a, b) => a > b ? a : b)),
    );

    _mapController.fitCamera(
      CameraFit.bounds(
        bounds: bounds,
        padding: const EdgeInsets.all(36),
      ),
    );
  }
}
