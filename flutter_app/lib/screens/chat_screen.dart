import 'package:flutter/material.dart';
import '../models/message.dart';
import '../models/device.dart';
import '../services/websocket_service.dart';
import '../widgets/message_bubble.dart';

class ChatScreen extends StatefulWidget {
  final WebSocketService webSocketService;
  final Device? selectedDevice;

  const ChatScreen({
    Key? key,
    required this.webSocketService,
    this.selectedDevice,
  }) : super(key: key);

  @override
  State<ChatScreen> createState() => _ChatScreenState();
}

class _ChatScreenState extends State<ChatScreen> {
  final TextEditingController _messageController = TextEditingController();
  final List<Message> _messages = [];
  final String _currentDeviceID = '1'; // Replace with actual device ID
  final String _currentDeviceName = 'My Device'; // Replace with actual name

  @override
  void initState() {
    super.initState();
    _setupMessageListener();
  }

  void _setupMessageListener() {
    if (widget.webSocketService.isConnected) {
      widget.webSocketService.getMessageStream().listen(
        (data) {
          print('Received: $data');
          if (data is String && data.contains('message')) {
            try {
              // Parse incoming message and add to list
              // This is a simplified parser
              if (data.contains('sender') && data.contains('content')) {
                setState(() {
                  _messages.insert(
                    0,
                    Message(
                      id: DateTime.now().millisecondsSinceEpoch.toString(),
                      sender: 'Other Device',
                      senderID: '2',
                      content: 'Incoming message',
                      timestamp: DateTime.now(),
                      isMe: false,
                    ),
                  );
                });
              }
            } catch (e) {
              print('Error parsing message: $e');
            }
          }
        },
        onError: (error) {
          print('WebSocket error: $error');
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text('Connection error: $error')),
          );
        },
        onDone: () {
          print('WebSocket connection closed');
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Disconnected from server')),
          );
        },
      );
    }
  }

  void _sendMessage() {
    if (_messageController.text.isEmpty) return;

    final message = Message(
      id: DateTime.now().millisecondsSinceEpoch.toString(),
      sender: _currentDeviceName,
      senderID: _currentDeviceID,
      content: _messageController.text,
      timestamp: DateTime.now(),
      receiver: widget.selectedDevice?.deviceID,
      isMe: true,
    );

    try {
      widget.webSocketService.sendMessage(
        _currentDeviceName,
        _currentDeviceID,
        _messageController.text,
        receiver: widget.selectedDevice?.deviceID,
      );

      setState(() {
        _messages.insert(0, message);
      });

      _messageController.clear();
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Failed to send message: $e')),
      );
    }
  }

  @override
  void dispose() {
    _messageController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(
          widget.selectedDevice != null
              ? 'Chat with ${widget.selectedDevice!.deviceName}'
              : 'Select a device',
        ),
        backgroundColor: Colors.deepPurple,
      ),
      body: Column(
        children: [
          if (widget.selectedDevice != null)
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: Row(
                children: [
                  Expanded(
                    child: Text(
                      '${widget.selectedDevice!.deviceName} - ${widget.selectedDevice!.isOnline ? 'Online' : 'Offline'} (RSSI: ${widget.selectedDevice!.rssi})',
                      style: const TextStyle(fontSize: 12, color: Colors.grey),
                    ),
                  ),
                ],
              ),
            ),
          Expanded(
            child: _messages.isEmpty
                ? const Center(
                    child: Text('No messages yet. Send one to start!'),
                  )
                : ListView.builder(
                    reverse: true,
                    itemCount: _messages.length,
                    itemBuilder: (context, index) {
                      return MessageBubble(
                        message: _messages[index],
                      );
                    },
                  ),
          ),
          if (widget.selectedDevice != null)
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
                        contentPadding: const EdgeInsets.symmetric(
                          horizontal: 16,
                          vertical: 12,
                        ),
                      ),
                      maxLines: null,
                    ),
                  ),
                  const SizedBox(width: 8),
                  FloatingActionButton(
                    onPressed: _sendMessage,
                    child: const Icon(Icons.send),
                  ),
                ],
              ),
            )
          else
            const Padding(
              padding: EdgeInsets.all(16.0),
              child: Text('Select a device to start chatting'),
            ),
        ],
      ),
    );
  }
}
