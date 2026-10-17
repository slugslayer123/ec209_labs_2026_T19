import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';

void main() => runApp(const EnergyMonitorApp());

class EnergyMonitorApp extends StatelessWidget {
  const EnergyMonitorApp({super.key});

  @override
  Widget build(BuildContext context) => MaterialApp(
        title: 'EE209 Energy Monitor',
        debugShowCheckedModeBanner: false,
        theme: ThemeData(useMaterial3: true, colorSchemeSeed: Colors.teal, brightness: Brightness.dark),
        home: const MonitorHome(),
      );
}

class MonitorHome extends StatefulWidget {
  const MonitorHome({super.key});
  @override
  State<MonitorHome> createState() => _MonitorHomeState();
}

class _MonitorHomeState extends State<MonitorHome> {
  final _hostController = TextEditingController(text: '192.168.4.1');
  final HttpClient _http = HttpClient()..connectionTimeout = const Duration(seconds: 2);
  Timer? _timer;
  Map<String, dynamic>? _data;
  String? _error;
  bool _busy = false;
  String _host = '192.168.4.1';
  int _lastUpdate = 0;

  @override
  void initState() {
    super.initState();
    _fetch();
    _timer = Timer.periodic(const Duration(milliseconds: 500), (_) => _fetch());
  }

  Future<void> _fetch() async {
    if (_busy) return;
    _busy = true;
    try {
      final request = await _http.getUrl(Uri.parse('http://$_host/api/v1/state'))
          .timeout(const Duration(seconds: 2));
      final response = await request.close().timeout(const Duration(seconds: 2));
      if (response.statusCode != 200) throw HttpException('HTTP ${response.statusCode}');
      final body = await utf8.decoder.bind(response).join().timeout(const Duration(seconds: 2));
      final parsed = jsonDecode(body);
      if (parsed is! Map<String, dynamic> || parsed['schema'] != 'ee209.v1') {
        throw const FormatException('Unexpected API response');
      }
      if (mounted) setState(() {
        _data = parsed;
        _error = null;
        _lastUpdate = DateTime.now().millisecondsSinceEpoch;
      });
    } catch (e) {
      if (mounted) setState(() => _error = e.toString());
    } finally {
      _busy = false;
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    _http.close(force: true);
    _hostController.dispose();
    super.dispose();
  }

  Widget _measurement(String label, String value, String unit, IconData icon) {
    return Card(
      clipBehavior: Clip.antiAlias,
      child: Padding(
        padding: const EdgeInsets.all(18),
        child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
          Row(children: [Icon(icon, size: 20), const SizedBox(width: 8),
            Expanded(child: Text(label, style: Theme.of(context).textTheme.titleSmall))]),
          const SizedBox(height: 14),
          FittedBox(alignment: Alignment.centerLeft,
            child: Text('$value $unit', style: Theme.of(context).textTheme.headlineMedium)),
        ]),
      ),
    );
  }

  String _fmt(String field, int decimals) {
    final v = _data?[field];
    return v is num ? v.toStringAsFixed(decimals) : '—';
  }

  @override
  Widget build(BuildContext context) {
    final isDemo = _data?['demo'] == true;
    final online = _data?['online'] == true &&
        DateTime.now().millisecondsSinceEpoch - _lastUpdate < 2500 && _error == null;
    final calibrated = _data?['calibrated'] == true;
    final loss = _data?['sampleLoss'] == true;
    final clipping = _data?['adcClipping'] == true;
    final size = MediaQuery.of(context).size.width;
    return Scaffold(
      appBar: AppBar(title: const Text('EE209 Smart Energy Monitor')),
      body: ListView(padding: const EdgeInsets.all(16), children: [
        Row(children: [
          Icon(online ? Icons.check_circle : Icons.wifi_off,
              color: online ? Colors.greenAccent : Colors.orangeAccent),
          const SizedBox(width: 8),
          Expanded(child: Text(online ? (isDemo ? 'Connected — DEMO data' : 'Connected — ATmega live data')
               : 'Disconnected / stale data')),
        ]),
        const SizedBox(height: 12),
        if (isDemo) const _Notice(
          'DEMO MODE: values are simulated by the ESP32, not physical measurements.', Icons.science),
        if (!isDemo && !calibrated) const _Notice(
          'UNCALIBRATED: electrical values are provisional and must not be treated as accurate.', Icons.warning_amber),
        if (loss || clipping) _Notice(
          'Data quality warning: ${loss ? 'sample loss ' : ''}${clipping ? 'ADC clipping' : ''}', Icons.error_outline),
        if (_error != null) _Notice('Communication: $_error', Icons.wifi_off),
        const SizedBox(height: 12),
        LayoutBuilder(builder: (context, constraints) {
          final width = constraints.maxWidth;
          return GridView.count(
            shrinkWrap: true,
            physics: const NeverScrollableScrollPhysics(),
            crossAxisCount: width > 680 ? 4 : 2,
            childAspectRatio: width > 680 ? 1.35 : (size < 370 ? 1.05 : 1.20),
            children: [
              _measurement('RMS voltage', _fmt('voltageVrms', 2), 'V', Icons.bolt),
              _measurement('Peak current', _fmt('peakCurrentMa', 0), 'mA', Icons.electrical_services),
              _measurement('Real power', _fmt('realPowerW', 2), 'W', Icons.electric_meter),
              _measurement('Energy', _fmt('energyWh', 3), 'Wh', Icons.battery_charging_full),
            ],
          );
        }),
        const SizedBox(height: 14),
        Text('ESP32 address', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 8),
        Row(children: [
          Expanded(child: TextField(controller: _hostController,
            decoration: const InputDecoration(border: OutlineInputBorder(), labelText: 'IP address'),
            keyboardType: TextInputType.url)),
          const SizedBox(width: 8),
          FilledButton(onPressed: () { setState(() { _host = _hostController.text.trim(); _data = null; }); _fetch(); },
            child: const Text('Connect')),
        ]),
        const SizedBox(height: 16),
        Text('Diagnostics', style: Theme.of(context).textTheme.titleMedium),
        const SizedBox(height: 8),
        Text('Packets accepted: ${_data?['validFrames'] ?? '—'}  •  Invalid: ${_data?['invalidFrames'] ?? '—'}'),
        Text('Sequence: ${_data?['sequence'] ?? '—'}  •  Age: ${_data?['ageMs'] ?? '—'} ms'),
        Text('ADC0 bias: ${_data?['adc0Bias'] ?? '—'}  •  ADC1 bias: ${_data?['adc1Bias'] ?? '—'}'),
        Text('ADC2 at boot: ${_data?['adc2Boot'] ?? '—'}'),
        const SizedBox(height: 22),
        const Text('Connect your phone/computer to Wi-Fi "EE209-Monitor" (password: ee209demo123). '
          'The ESP32 hosts a local API at 192.168.4.1; internet is not required.'),
      ]),
    );
  }
}

class _Notice extends StatelessWidget {
  final String message;
  final IconData icon;
  const _Notice(this.message, this.icon);
  @override
  Widget build(BuildContext context) => Padding(
    padding: const EdgeInsets.only(bottom: 7),
    child: Card(child: Padding(padding: const EdgeInsets.all(12),
      child: Row(children: [Icon(icon), const SizedBox(width: 10), Expanded(child: Text(message))]))),
  );
}
