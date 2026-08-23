import 'package:blackout_buddy/services/node_registry_service.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('NodeRegistryService protocol parsing', () {
    late NodeRegistryService registry;

    setUp(() {
      registry = NodeRegistryService();
      registry.clear();
    });

    test('ingests RX frame with signal metrics', () {
      registry.ingestProtocolLine('RX|2|17|-98|7.25|hello');

      final nodes = registry.nodesSnapshot;
      expect(nodes, hasLength(1));
      expect(nodes.first.nodeId, '2');
      expect(nodes.first.name, 'LilyGO-2');
      expect(nodes.first.rssi, -98);
      expect(nodes.first.snr, 7.25);
      expect(nodes.first.hasLocation, isFalse);
    });

    test('ingests LOC frame with ISO-8601 UTC timestamp', () {
      registry.ingestProtocolLine('LOC|2|48.137154|11.576124|3.5|2026-07-30T14:23:05Z');

      final nodes = registry.nodesSnapshot;
      expect(nodes, hasLength(1));
      expect(nodes.first.nodeId, '2');
      expect(nodes.first.latitude, closeTo(48.137154, 0.000001));
      expect(nodes.first.longitude, closeTo(11.576124, 0.000001));
      expect(nodes.first.accuracyMeters, 3.5);
      expect(
        nodes.first.lastSeen.toUtc().toIso8601String(),
        '2026-07-30T14:23:05.000Z',
      );
    });

    test('ignores invalid LOC frame coordinates', () {
      registry.ingestProtocolLine('LOC|2|invalid|11.576124|3.5|2026-07-30T14:23:05Z');

      expect(registry.nodesSnapshot, isEmpty);
    });
  });

  group('NodePresence status tiers', () {
    test('online when last seen under 30 seconds', () {
      final now = DateTime.utc(2026, 7, 30, 14, 30, 0);
      final node = NodePresence(
        nodeId: '1',
        name: 'LilyGO-1',
        lastSeen: now.subtract(const Duration(seconds: 10)),
      );

      expect(node.statusAt(now), NodePresenceStatus.online);
    });

    test('inactive between 30 and 120 seconds', () {
      final now = DateTime.utc(2026, 7, 30, 14, 30, 0);
      final node = NodePresence(
        nodeId: '1',
        name: 'LilyGO-1',
        lastSeen: now.subtract(const Duration(seconds: 45)),
      );

      expect(node.statusAt(now), NodePresenceStatus.inactive);
    });

    test('offline over 120 seconds', () {
      final now = DateTime.utc(2026, 7, 30, 14, 30, 0);
      final node = NodePresence(
        nodeId: '1',
        name: 'LilyGO-1',
        lastSeen: now.subtract(const Duration(seconds: 150)),
      );

      expect(node.statusAt(now), NodePresenceStatus.offline);
    });
  });
}
