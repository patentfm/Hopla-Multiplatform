import 'dart:async';
import 'dart:math';
import 'dart:ui';

import 'package:flame/components.dart';
import 'package:flame/game.dart';
import 'package:flame/text.dart';
import 'package:flutter/material.dart';
import 'package:sensors_plus/sensors_plus.dart';

class MoleSmashRhythmScreen extends StatefulWidget {
  const MoleSmashRhythmScreen({super.key});

  @override
  State<MoleSmashRhythmScreen> createState() => _MoleSmashRhythmScreenState();
}

class _MoleSmashRhythmScreenState extends State<MoleSmashRhythmScreen> {
  late final MoleSmashRhythmGame _game;
  StreamSubscription<AccelerometerEvent>? _accelerometerSub;

  @override
  void initState() {
    super.initState();
    _game = MoleSmashRhythmGame();
    _accelerometerSub = accelerometerEvents.listen(_game.handleAccelerometer);
  }

  @override
  void dispose() {
    _accelerometerSub?.cancel();
    _game.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: GameWidget(
          game: _game,
          overlayBuilderMap: {
            'hud': (context, game) => _HudOverlay(game as MoleSmashRhythmGame),
            'gameOver': (context, game) => _GameOverOverlay(
                  game: game as MoleSmashRhythmGame,
                ),
          },
          initialActiveOverlays: const ['hud'],
        ),
      ),
    );
  }
}

class MoleSmashRhythmGame extends FlameGame {
  MoleSmashRhythmGame();

  static const int _gridSize = 3;
  static const double _impactDeltaThreshold = 5.2;
  static const double _impactAbsoluteThreshold = 15.0;
  static const double _tiltRange = 8.0;

  final Map<int, _Mole> _activeMoles = {};
  final Random _random = Random();
  final Duration _initialSpawnInterval = const Duration(milliseconds: 1500);
  final Duration _minSpawnInterval = const Duration(milliseconds: 550);
  final Duration _moleLifetime = const Duration(milliseconds: 1300);

  final ValueNotifier<_HudState> hud = ValueNotifier(
    const _HudState(score: 0, streak: 0, lives: 3, difficulty: 1, tempoMs: 1500, paused: false),
  );

  double _elapsed = 0;
  double _nextSpawnAt = 1.5;
  int _score = 0;
  int _streak = 0;
  int _lives = 3;
  bool _isGameOver = false;
  bool _isPaused = false;
  double _difficulty = 1.0;
  double _tiltX = 0;
  double _tiltY = 0;
  double _lastZ = 0;
  DateTime _lastImpactAt = DateTime.fromMillisecondsSinceEpoch(0);

  @override
  Future<void> onLoad() async {
    camera.viewport = FixedResolutionViewport(Vector2(1080, 1920));
  }

  @override
  void update(double dt) {
    super.update(dt);
    if (_isPaused || _isGameOver) {
      return;
    }

    _elapsed += dt;
    _spawnMolesIfNeeded();
    _expireMoles();
    _adjustDifficulty();
    _updateHud();
  }

  @override
  void render(Canvas canvas) {
    super.render(canvas);
    final bgPaint = Paint()..color = const Color(0xFF0E0F17);
    canvas.drawRect(Rect.fromLTWH(0, 0, size.x, size.y), bgPaint);

    final boardSize = min(size.x, size.y) * 0.9;
    final offset = Offset((size.x - boardSize) / 2, (size.y - boardSize) / 2 + 40);
    final cellSize = boardSize / _gridSize;

    final inactivePaint = Paint()..color = const Color(0xFF1F2230);
    final activePaint = Paint()..color = const Color(0xFF4CAF50);
    final borderPaint = Paint()
      ..color = const Color(0xFF3C4DD0)
      ..style = PaintingStyle.stroke
      ..strokeWidth = 5;
    final timerPaint = Paint()
      ..color = const Color(0xFFFFC107)
      ..strokeCap = StrokeCap.round
      ..strokeWidth = 6;

    final textPaint = TextPaint(
      style: const TextStyle(
        color: Colors.white,
        fontSize: 28,
        fontWeight: FontWeight.bold,
      ),
    );

    for (int row = 0; row < _gridSize; row++) {
      for (int col = 0; col < _gridSize; col++) {
        final index = row * _gridSize + col;
        final rect = Rect.fromLTWH(
          offset.dx + col * cellSize,
          offset.dy + row * cellSize,
          cellSize,
          cellSize,
        );

        final mole = _activeMoles[index];
        final isSelected = index == _selectedHoleIndex;
        final basePaint = mole != null ? activePaint : inactivePaint;

        canvas.drawRRect(
          RRect.fromRectAndRadius(rect.deflate(10), const Radius.circular(18)),
          basePaint,
        );

        if (mole != null) {
          final progress = ((elapsedSeconds - mole.spawnedAt) /
                  (_moleLifetime.inMilliseconds / 1000))
              .clamp(0.0, 1.0);
          final barStart = Offset(rect.left + 14, rect.bottom - 18);
          final barEnd = Offset(rect.left + 14 + (rect.width - 28) * (1 - progress), rect.bottom - 18);
          canvas.drawLine(barStart, barEnd, timerPaint);
          textPaint.render(
            canvas,
            '${((1 - progress) * 100).clamp(0, 99).round()}%',
            Vector2(rect.center.dx - 24, rect.center.dy - 12),
          );
        }

        if (isSelected) {
          canvas.drawRRect(
            RRect.fromRectAndRadius(rect.deflate(6), const Radius.circular(22)),
            borderPaint,
          );
        }
      }
    }
  }

  void handleAccelerometer(AccelerometerEvent event) {
    if (_isPaused || _isGameOver) return;

    _tiltX = event.x;
    _tiltY = event.y;

    final deltaZ = (event.z - _lastZ).abs();
    final now = DateTime.now();
    if (deltaZ > _impactDeltaThreshold &&
        event.z > _impactAbsoluteThreshold &&
        now.difference(_lastImpactAt) > const Duration(milliseconds: 300)) {
      _registerImpact();
      _lastImpactAt = now;
    }

    _lastZ = event.z;
  }

  void togglePause() {
    if (_isGameOver) return;
    _isPaused = !_isPaused;
    if (_isPaused) {
      pauseEngine();
    } else {
      resumeEngine();
    }
    _updateHud();
  }

  void restart() {
    _activeMoles.clear();
    _elapsed = 0;
    _nextSpawnAt = _initialSpawnInterval.inMilliseconds / 1000;
    _score = 0;
    _streak = 0;
    _lives = 3;
    _isPaused = false;
    _isGameOver = false;
    _difficulty = 1.0;
    _tiltX = 0;
    _tiltY = 0;
    _lastZ = 0;
    _lastImpactAt = DateTime.fromMillisecondsSinceEpoch(0);
    overlays.remove('gameOver');
    resumeEngine();
    _updateHud();
  }

  void _spawnMolesIfNeeded() {
    if (elapsedSeconds < _nextSpawnAt) return;

    final freeHoles = List.generate(_gridSize * _gridSize, (i) => i)
        .where((index) => !_activeMoles.containsKey(index))
        .toList();
    if (freeHoles.isEmpty) {
      _nextSpawnAt += _currentSpawnInterval;
      return;
    }

    final holeIndex = freeHoles[_random.nextInt(freeHoles.length)];
    _activeMoles[holeIndex] = _Mole(spawnedAt: elapsedSeconds);
    _nextSpawnAt = elapsedSeconds + _currentSpawnInterval;
  }

  void _expireMoles() {
    final expired = _activeMoles.entries
        .where((entry) => elapsedSeconds - entry.value.spawnedAt > _moleLifetime.inMilliseconds / 1000)
        .map((entry) => entry.key)
        .toList();
    for (final index in expired) {
      _activeMoles.remove(index);
      _registerMiss();
    }
  }

  void _registerImpact() {
    final targetIndex = _selectedHoleIndex;
    final mole = _activeMoles[targetIndex];
    if (mole != null) {
      final reaction = elapsedSeconds - mole.spawnedAt;
      final timingBonus = (1.0 - (reaction / (_moleLifetime.inMilliseconds / 1000))).clamp(0.3, 1.0);
      final baseScore = 100 + (_difficulty * 25).round();
      _score += (baseScore * timingBonus).round();
      _streak += 1;
      _activeMoles.remove(targetIndex);
    } else {
      _registerMiss();
    }
  }

  void _registerMiss() {
    _streak = 0;
    _lives = max(0, _lives - 1);
    if (_lives == 0) {
      _endGame();
    }
  }

  void _adjustDifficulty() {
    final timeFactor = (_elapsed ~/ 20) + 1;
    final streakFactor = 1 + (_streak ~/ 5) * 0.05;
    _difficulty = (timeFactor * streakFactor).clamp(1.0, 3.5).toDouble();
  }

  void _updateHud() {
    hud.value = _HudState(
      score: _score,
      streak: _streak,
      lives: _lives,
      difficulty: _difficulty,
      tempoMs: (_currentSpawnInterval * 1000).round(),
      paused: _isPaused,
    );
  }

  void _endGame() {
    _isGameOver = true;
    pauseEngine();
    overlays.add('gameOver');
  }

  int get _selectedHoleIndex {
    final normX = ((_tiltX / _tiltRange) + 1) / 2;
    final normY = ((-_tiltY / _tiltRange) + 1) / 2;
    final col = (normX * _gridSize).clamp(0, _gridSize - 0.001).floor();
    final row = (normY * _gridSize).clamp(0, _gridSize - 0.001).floor();
    return row * _gridSize + col;
  }

  double get elapsedSeconds => _elapsed;

  double get _currentSpawnInterval {
    final multiplier = max(0.35, 1 - ((_difficulty - 1) * 0.18));
    final next = (_initialSpawnInterval.inMilliseconds * multiplier).round();
    final candidate = next / 1000;
    return candidate < _minSpawnInterval.inMilliseconds / 1000
        ? _minSpawnInterval.inMilliseconds / 1000
        : candidate;
  }
}

class _HudState {
  const _HudState({
    required this.score,
    required this.streak,
    required this.lives,
    required this.difficulty,
    required this.tempoMs,
    required this.paused,
  });

  final int score;
  final int streak;
  final int lives;
  final double difficulty;
  final int tempoMs;
  final bool paused;
}

class _HudOverlay extends StatelessWidget {
  const _HudOverlay(this.game);

  final MoleSmashRhythmGame game;

  @override
  Widget build(BuildContext context) {
    return Align(
      alignment: Alignment.topCenter,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: DecoratedBox(
          decoration: BoxDecoration(
            color: Colors.black.withOpacity(0.45),
            borderRadius: BorderRadius.circular(16),
          ),
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
            child: ValueListenableBuilder<_HudState>(
              valueListenable: game.hud,
              builder: (context, state, _) {
                return Column(
                  mainAxisSize: MainAxisSize.min,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        _statTile('Wynik', state.score.toString(), Icons.bolt),
                        _statTile('Streak', 'x${state.streak}', Icons.trending_up),
                        _statTile('Życia', state.lives.toString(), Icons.favorite),
                      ],
                    ),
                    const SizedBox(height: 12),
                    Row(
                      children: [
                        Text('Poziom: ${state.difficulty.toStringAsFixed(1)}', style: _labelStyle),
                        const SizedBox(width: 12),
                        Text('Tempo: ${state.tempoMs} ms', style: _labelStyle),
                        const Spacer(),
                        IconButton(
                          onPressed: game.togglePause,
                          icon: Icon(state.paused ? Icons.play_arrow : Icons.pause),
                          tooltip: state.paused ? 'Wznów' : 'Pauza',
                          color: Colors.white,
                        ),
                        IconButton(
                          onPressed: game.restart,
                          icon: const Icon(Icons.refresh),
                          tooltip: 'Restart',
                          color: Colors.white,
                        ),
                      ],
                    ),
                  ],
                );
              },
            ),
          ),
        ),
      ),
    );
  }

  static Widget _statTile(String label, String value, IconData icon) {
    return Row(
      children: [
        Icon(icon, color: Colors.white),
        const SizedBox(width: 6),
        Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(label, style: _labelStyle),
            Text(value, style: _valueStyle),
          ],
        ),
      ],
    );
  }

  static const TextStyle _labelStyle = TextStyle(
    color: Colors.white70,
    fontSize: 13,
  );

  static const TextStyle _valueStyle = TextStyle(
    color: Colors.white,
    fontSize: 18,
    fontWeight: FontWeight.bold,
  );
}

class _GameOverOverlay extends StatelessWidget {
  const _GameOverOverlay({required this.game});

  final MoleSmashRhythmGame game;

  @override
  Widget build(BuildContext context) {
    return Container(
      color: Colors.black.withOpacity(0.6),
      child: Center(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text(
              'Koniec gry!'
              '\nWychyl platformę, aby celować, i uderz, aby trafić krecika.',
              textAlign: TextAlign.center,
              style: TextStyle(
                color: Colors.white,
                fontSize: 22,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 20),
            FilledButton.icon(
              onPressed: game.restart,
              icon: const Icon(Icons.refresh),
              label: const Text('Zagraj ponownie'),
            ),
          ],
        ),
      ),
    );
  }
}

class _Mole {
  _Mole({required this.spawnedAt});

  final double spawnedAt;
}
