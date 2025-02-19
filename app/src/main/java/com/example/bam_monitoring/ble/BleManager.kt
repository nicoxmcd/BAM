package com.example.bam_monitoring.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.le.*
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log

@SuppressLint("MissingPermission")
class BleManager(private val context: Context) {

    interface DiagnosticListener {
        fun onDiagnostic(message: String)
    }
    var diagnosticListener: DiagnosticListener? = null

    private val TAG = "BleManager"
    private val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private val bluetoothLeScanner: BluetoothLeScanner? = bluetoothAdapter?.bluetoothLeScanner
    private val handler = Handler(Looper.getMainLooper())
    private var scanning = false
    private val SCAN_PERIOD: Long = 300  // You can adjust this duration if desired

    // A set to store discovered devices.
    private val discoveredDevices = mutableSetOf<String>()

    // Scan callback adapted from your original working code.
    private val leScanCallback: ScanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            result?.device?.let { device ->
                val deviceInfo = "${device.name ?: "Unknown"} - ${device.address}"
                if (discoveredDevices.add(deviceInfo)) {
                    diagnosticListener?.onDiagnostic("Found device: $deviceInfo")
                    Log.d(TAG, "Found device: $deviceInfo")
                }
            }
        }
        override fun onBatchScanResults(results: MutableList<ScanResult>?) {
            results?.forEach { result ->
                onScanResult(ScanSettings.CALLBACK_TYPE_ALL_MATCHES, result)
            }
        }
        override fun onScanFailed(errorCode: Int) {
            val errorMsg = "BLE scan failed with error: $errorCode"
            diagnosticListener?.onDiagnostic(errorMsg)
            Log.e(TAG, errorMsg)
        }
    }

    fun startScan() {
        if (bluetoothAdapter == null || bluetoothLeScanner == null) {
            diagnosticListener?.onDiagnostic("Bluetooth adapter or scanner is null!")
            Log.e(TAG, "Bluetooth adapter or scanner is null!")
            return
        }
        if (scanning) return
        scanning = true
        discoveredDevices.clear()
        diagnosticListener?.onDiagnostic("Starting BLE scan...")
        bluetoothLeScanner.startScan(leScanCallback)
        handler.postDelayed({
            stopScan()
            if (discoveredDevices.isEmpty()) {
                diagnosticListener?.onDiagnostic("No devices found")
            } else {
                diagnosticListener?.onDiagnostic("Scan complete. Devices found:")
                discoveredDevices.forEach { device ->
                    diagnosticListener?.onDiagnostic(device)
                }
            }
        }, SCAN_PERIOD)
    }

    fun stopScan() {
        if (!scanning) return
        scanning = false
        bluetoothLeScanner?.stopScan(leScanCallback)
        diagnosticListener?.onDiagnostic("BLE scan stopped")
        Log.d(TAG, "Stopped scan")
    }
}
