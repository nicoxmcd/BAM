package com.example.bam_monitoring.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.util.Log
import java.util.UUID

@SuppressLint("MissingPermission")
class BleManager(private val context: Context) {

    // Diagnostic listener interface
    interface DiagnosticListener {
        fun onDiagnostic(message: String)
    }
    var diagnosticListener: DiagnosticListener? = null

    private val bluetoothManager: BluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private var bluetoothGatt: BluetoothGatt? = null
    private val MUSCLE_STRAIN_CHAR_UUID: UUID = UUID.fromString("00002A58-0000-1000-8000-00805f9b34fb")

    // In-memory store for received BLE data
    val receivedData = mutableListOf<ByteArray>()

    // Start scanning for BLE devices
    fun startScan() {
        val scanner = bluetoothAdapter?.bluetoothLeScanner
        scanner?.startScan(scanCallback)
        val msg = "BLE scan started"
        Log.d("BleManager", msg)
        diagnosticListener?.onDiagnostic(msg)
    }

    // Stop scanning when no longer needed
    fun stopScan() {
        val scanner = bluetoothAdapter?.bluetoothLeScanner
        scanner?.stopScan(scanCallback)
        val msg = "BLE scan stopped"
        Log.d("BleManager", msg)
        diagnosticListener?.onDiagnostic(msg)
    }

    // Scan callback that handles discovered devices
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            result?.device?.let { device ->
                val msg = "Found device: ${device.name} - ${device.address}"
                Log.d("BleManager", msg)
                diagnosticListener?.onDiagnostic(msg)
                // Connect to the first discovered device (add filtering as needed)
                if (bluetoothGatt == null) {
                    connectToDevice(device)
                }
            }
        }

        override fun onScanFailed(errorCode: Int) {
            val msg = "BLE scan failed with error code: $errorCode"
            Log.e("BleManager", msg)
            diagnosticListener?.onDiagnostic(msg)
        }
    }

    // Connect to the discovered BLE device
    private fun connectToDevice(device: BluetoothDevice) {
        val msg = "Connecting to device: ${device.address}"
        Log.d("BleManager", msg)
        diagnosticListener?.onDiagnostic(msg)
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
    }

    // Handle connection events, service discovery, and data reception
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                val msg = "Connected to GATT server. Discovering services..."
                Log.d("BleManager", msg)
                diagnosticListener?.onDiagnostic(msg)
                bluetoothGatt?.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                val msg = "Disconnected from GATT server."
                Log.d("BleManager", msg)
                diagnosticListener?.onDiagnostic(msg)
                bluetoothGatt?.close()
                bluetoothGatt = null
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                gatt?.services?.forEach { service ->
                    service.characteristics.forEach { characteristic ->
                        if (characteristic.uuid == MUSCLE_STRAIN_CHAR_UUID) {
                            if (characteristic.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY != 0) {
                                setCharacteristicNotification(gatt, characteristic, true)
                            }
                            gatt.readCharacteristic(characteristic)
                        }
                    }
                }
            } else {
                val msg = "onServicesDiscovered received: $status"
                Log.w("BleManager", msg)
                diagnosticListener?.onDiagnostic(msg)
            }
        }

        override fun onCharacteristicRead(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?,
            status: Int
        ) {
            if (status == BluetoothGatt.GATT_SUCCESS && characteristic != null) {
                characteristic.value?.let { data ->
                    receivedData.add(data)
                    val msg = "Characteristic read: ${data.contentToString()}"
                    Log.d("BleManager", msg)
                    diagnosticListener?.onDiagnostic(msg)
                }
            }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?
        ) {
            characteristic?.value?.let { data ->
                receivedData.add(data)
                val msg = "Characteristic changed: ${data.contentToString()}"
                Log.d("BleManager", msg)
                diagnosticListener?.onDiagnostic(msg)
            }
        }
    }

    // Enable or disable notifications for a characteristic by writing to its descriptor
    private fun setCharacteristicNotification(
        gatt: BluetoothGatt,
        characteristic: BluetoothGattCharacteristic,
        enabled: Boolean
    ) {
        gatt.setCharacteristicNotification(characteristic, enabled)
        val descriptor = characteristic.getDescriptor(
            UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
        )
        if (descriptor != null) {
            descriptor.value = if (enabled)
                BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
            else
                BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE
            gatt.writeDescriptor(descriptor)
            val msg = "Characteristic ${characteristic.uuid} notifications ${if (enabled) "enabled" else "disabled"}"
            Log.d("BleManager", msg)
            diagnosticListener?.onDiagnostic(msg)
        } else {
            val msg = "Descriptor not found for characteristic ${characteristic.uuid}"
            Log.w("BleManager", msg)
            diagnosticListener?.onDiagnostic(msg)
        }
    }
}
