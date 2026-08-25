import 'dart:async';

import 'package:flutter/foundation.dart';

enum NodePresenceStatus { online, inactive, offline }

class NodePresence {
  final String nodeId;
  final String name;
  final DateTime lastSeen;
  final int? rssi;
  final double? snr;
  final double? latitude;
  final double? longitude;
  final double? accuracyMeters;
  final int? batteryPercentage;
  final double? batteryVoltage;

  const NodePresence({
    required this.nodeId,
    required this.name,
    required this.lastSeen,
    this.rssi,
    this.snr,
    this.latitude,
    this.longitude,
    this.accuracyMeters,
    this.batteryPercentage,
    this.batteryVoltage,
  });

  NodePresence copyWith({
    String? nodeId,
    String? name,
    DateTime? lastSeen,
    int? rssi,
    double? snr,
    double? latitude,
    double? longitude,
    double? accuracyMeters,
    int? batteryPercentage,
    double? batteryVoltage,
  }) {
    return NodePresence(
      nodeId: nodeId ?? this.nodeId,
      name: name ?? this.name,
      lastSeen: lastSeen ?? this.lastSeen,
      rssi: rssi ?? this.rssi,
      snr: snr ?? this.snr,
      latitude: latitude ?? this.latitude,
      longitude: longitude ?? this.longitude,
      accuracyMeters: accuracyMeters ?? this.accuracyMeters,
      batteryPercentage: batteryPercentage ?? this.batteryPercentage,
      batteryVoltage: batteryVoltage ?? this.batteryVoltage,
    );
  }

  NodePresenceStatus statusAt(DateTime now) {
    final seconds = now.difference(lastSeen).inSeconds;
    if (seconds < 30) return NodePresenceStatus.online;
    if (seconds <= 120) return NodePresenceStatus.inactive;
    return NodePresenceStatus.offline;
  }

  bool get hasLocation => latitude != null && longitude != null;
}

class NodeRegistryService {
  static final NodeRegistryService _instance = NodeRegistryService._internal();

  factory NodeRegistryService() => _instance;

  NodeRegistryService._internal();

  final Map<String, NodePresence> _nodes = <String, NodePresence>{};
  final ValueNotifier<List<NodePresence>> _nodesNotifier =
      ValueNotifier<List<NodePresence>>(const <NodePresence>[]);
  Timer? _refreshTimer;

  ValueListenable<List<NodePresence>> get nodesListenable => _nodesNotifier;

  List<NodePresence> get nodesSnapshot => _nodesNotifier.value;

  void clear() {
    _nodes.clear();
    _cancelRefreshTimer();
    _publishSnapshot();
  }

  void _startRefreshTimerIfNeeded() {
    if (_refreshTimer != null && _refreshTimer!.isActive) return;
    _refreshTimer = Timer.periodic(
      const Duration(seconds: 5),
      (_) => _publishSnapshot(),
    );
  }

  void _cancelRefreshTimer() {
    _refreshTimer?.cancel();
    _refreshTimer = null;
  }

  void ingestProtocolLine(String line) {
    final parts = line.trim().split('|');
    if (parts.isEmpty) return;

    if (parts.first == 'RX' && parts.length >= 6) {
      final nodeId = parts[1];
      _upsertNode(
        nodeId: nodeId,
        name: 'LilyGO-$nodeId',
        lastSeen: DateTime.now(),
        rssi: int.tryParse(parts[3]),
        snr: double.tryParse(parts[4]),
      );
      return;
    }

    if (parts.first == 'LOC' && parts.length >= 4) {
      final nodeId = parts[1];
      final lat = double.tryParse(parts[2]);
      final lon = double.tryParse(parts[3]);
      final accuracy = parts.length >= 5 ? double.tryParse(parts[4]) : null;
      final timestamp =
          parts.length >= 6 ? DateTime.tryParse(parts[5]) : DateTime.now();

      if (lat == null || lon == null) return;

      _upsertNode(
        nodeId: nodeId,
        name: 'LilyGO-$nodeId',
        lastSeen: timestamp ?? DateTime.now(),
        latitude: lat,
        longitude: lon,
        accuracyMeters: accuracy,
      );
      return;
    }

    if (parts.first == 'BAT' && parts.length >= 4) {
      final nodeId = parts[1];
      final percentage = int.tryParse(parts[2]);
      final voltage = double.tryParse(parts[3]);

      if (percentage == null || voltage == null) return;

      _upsertNode(
        nodeId: nodeId,
        name: 'LilyGO-$nodeId',
        lastSeen: DateTime.now(),
        batteryPercentage: percentage,
        batteryVoltage: voltage,
      );
    }
  }

  void _upsertNode({
    required String nodeId,
    required String name,
    required DateTime lastSeen,
    int? rssi,
    double? snr,
    double? latitude,
    double? longitude,
    double? accuracyMeters,
    int? batteryPercentage,
    double? batteryVoltage,
  }) {
    final existing = _nodes[nodeId];
    if (existing == null) {
      _nodes[nodeId] = NodePresence(
        nodeId: nodeId,
        name: name,
        lastSeen: lastSeen,
        rssi: rssi,
        snr: snr,
        latitude: latitude,
        longitude: longitude,
        accuracyMeters: accuracyMeters,
        batteryPercentage: batteryPercentage,
        batteryVoltage: batteryVoltage,
      );
    } else {
      _nodes[nodeId] = existing.copyWith(
        name: name,
        lastSeen: lastSeen,
        rssi: rssi ?? existing.rssi,
        snr: snr ?? existing.snr,
        latitude: latitude ?? existing.latitude,
        longitude: longitude ?? existing.longitude,
        accuracyMeters: accuracyMeters ?? existing.accuracyMeters,
        batteryPercentage: batteryPercentage ?? existing.batteryPercentage,
        batteryVoltage: batteryVoltage ?? existing.batteryVoltage,
      );
    }

    _startRefreshTimerIfNeeded();
    _publishSnapshot();
  }

  void _publishSnapshot() {
    final snapshot = _nodes.values.toList()
      ..sort((a, b) => a.nodeId.compareTo(b.nodeId));
    _nodesNotifier.value = snapshot;
    if (snapshot.isEmpty) {
      _cancelRefreshTimer();
    }
  }
}
