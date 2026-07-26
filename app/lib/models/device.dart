class Device {
  final String deviceID;
  final String deviceName;
  final bool isOnline;
  final int rssi;
  final String mode; // 'blackout' or 'connectivity'

  Device({
    required this.deviceID,
    required this.deviceName,
    required this.isOnline,
    required this.rssi,
    required this.mode,
  });

  factory Device.fromJson(Map<String, dynamic> json) {
    return Device(
      deviceID: json['deviceID'] ?? '',
      deviceName: json['deviceName'] ?? 'Unknown Device',
      isOnline: json['isOnline'] ?? false,
      rssi: json['rssi'] ?? 0,
      mode: json['mode'] ?? 'connectivity',
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'deviceID': deviceID,
      'deviceName': deviceName,
      'isOnline': isOnline,
      'rssi': rssi,
      'mode': mode,
    };
  }
}
