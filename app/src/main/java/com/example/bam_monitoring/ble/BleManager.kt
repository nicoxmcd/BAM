package com.example.bam_monitoring.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import java.util.UUID

@SuppressLint("MissingPermission")
class BleManager(private val context: Context) {

    interface DiagnosticListener {
        fun onDiagnostic(message: String)
    }
    var diagnosticListener: DiagnosticListener? = null

    private val bluetoothManager: BluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private var bluetoothGatt: BluetoothGatt? = null

    // This UUID matches the sensor's EMG characteristic ("2A58")
    private val MUSCLE_STRAIN_CHAR_UUID: UUID =
        UUID.fromString("00002A58-0000-1000-8000-00805f9b34fb")

    // Specific MAC address to connect to.
    private val deviceMac = "CA:84:B5:D4:DF:2F"

    // Store received BLE data for further processing if needed.
    val receivedData = mutableListOf<ByteArray>()

    fun startScan() {
        bluetoothAdapter?.bluetoothLeScanner?.startScan(scanCallback)
        diagnosticListener?.onDiagnostic("BLE scan started")
    }

    fun stopScan() {
        bluetoothAdapter?.bluetoothLeScanner?.stopScan(scanCallback)
        diagnosticListener?.onDiagnostic("BLE scan stopped")
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            result?.device?.let { device ->
                // Only connect if the device MAC matches our target.
                if (device.address.equals(deviceMac, ignoreCase = true)) {
                    diagnosticListener?.onDiagnostic("Found target device: ${device.name ?: "Unknown"} - ${device.address}")
                    if (bluetoothGatt == null) {
                        connectToDevice(device)
                    }
                }
            }
        }

        override fun onScanFailed(errorCode: Int) {
            diagnosticListener?.onDiagnostic("BLE scan failed with error code: $errorCode")
        }
    }

    private fun connectToDevice(device: BluetoothDevice) {
        diagnosticListener?.onDiagnostic("Connecting to device: ${device.address}")
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
    }

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                diagnosticListener?.onDiagnostic("Connected. Discovering services...")
                bluetoothGatt?.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                diagnosticListener?.onDiagnostic("Disconnected from GATT server.")
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
                        }
                    }
                }
            } else {
                diagnosticListener?.onDiagnostic("Service discovery failed with status: $status")
            }
        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt?, characteristic: BluetoothGattCharacteristic?) {
            if (characteristic?.uuid == MUSCLE_STRAIN_CHAR_UUID) {
                characteristic.value?.let { data ->
                    receivedData.add(data)
                    val rawMessage = String(data, Charsets.UTF_8).trim()
                    val parsed = parseClassificationData(rawMessage)
                    diagnosticListener?.onDiagnostic("Muscle strain data: $parsed")
                }
            }
        }
    }

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
            diagnosticListener?.onDiagnostic("Subscribed to notifications for characteristic ${characteristic.uuid}")
        } else {
            diagnosticListener?.onDiagnostic("Descriptor not found for ${characteristic.uuid}")
        }
    }

    /**
     * Parses a classification string expected in the format:
     * "label1:XX% label2:YY% ..." and returns a formatted summary.
     */
    private fun parseClassificationData(rawMessage: String): String {
        // Split by whitespace; each token should be "label:XX%"
        val tokens = rawMessage.split(Regex("\\s+"))
        val parsedResults = tokens.mapNotNull { token ->
            val parts = token.split(":")
            if (parts.size == 2) {
                val label = parts[0]
                // Remove any trailing '%' and try to parse the number.
                val percentStr = parts[1].removeSuffix("%")
                val percent = percentStr.toIntOrNull()
                if (percent != null) {
                    "$label: ${percent}%"
                } else null
            } else null
        }
        return if (parsedResults.isNotEmpty())
            parsedResults.joinToString(separator = ", ")
        else
            rawMessage
    }
}
