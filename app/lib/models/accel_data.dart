/// Accelerometer data model
class AccelData {
  final int x;
  final int y;
  final int z;
  final DateTime timestamp;

  AccelData({
    required this.x,
    required this.y,
    required this.z,
    DateTime? timestamp,
  }) : timestamp = timestamp ?? DateTime.now();

  factory AccelData.fromBytes(List<int> bytes) {
    if (bytes.length < 6) {
      throw ArgumentError('Invalid data length');
    }

    // Little-endian conversion
    int x = (bytes[1] << 8) | bytes[0];
    int y = (bytes[3] << 8) | bytes[2];
    int z = (bytes[5] << 8) | bytes[4];

    // Sign extend if negative
    if (x > 32767) x -= 65536;
    if (y > 32767) y -= 65536;
    if (z > 32767) z -= 65536;

    return AccelData(x: x, y: y, z: z);
  }

  double get magnitude => 
      (x * x + y * y + z * z) / (1000.0 * 1000.0);

  @override
  String toString() => 'AccelData(x: $x, y: $y, z: $z)';
}

