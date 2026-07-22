class Message {
  final String id;
  final String sender;
  final String senderID;
  final String content;
  final DateTime timestamp;
  final String? receiver;
  final bool isMe;

  Message({
    required this.id,
    required this.sender,
    required this.senderID,
    required this.content,
    required this.timestamp,
    this.receiver,
    required this.isMe,
  });

  factory Message.fromJson(Map<String, dynamic> json) {
    return Message(
      id: json['id'] ?? '',
      sender: json['sender'] ?? 'Unknown',
      senderID: json['senderID'] ?? '',
      content: json['content'] ?? '',
      timestamp: DateTime.parse(json['timestamp'] ?? DateTime.now().toIso8601String()),
      receiver: json['receiver'],
      isMe: json['isMe'] ?? false,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'sender': sender,
      'senderID': senderID,
      'content': content,
      'timestamp': timestamp.toIso8601String(),
      'receiver': receiver,
      'isMe': isMe,
    };
  }
}
