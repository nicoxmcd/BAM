package com.example.bam_monitoring.ble

import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.util.Log
import java.util.UUID

class BleManager(private val context: Context) {
    private val bluetoothManager: BluetoothManager =
        context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private var bluetoothGatt: BluetoothGatt? = null

    // In-memory store for received BLE data
    val receivedData = mutableListOf<ByteArray>()

    // Start scanning for BLE devices
    fun startScan() {
        val scanner = bluetoothAdapter?.bluetoothLeScanner
        scanner?.startScan(scanCallback)
        Log.d("BleManager", "BLE scan started")
    }

    // Stop scanning when no longer needed
    fun stopScan() {
        val scanner = bluetoothAdapter?.bluetoothLeScanner
        scanner?.stopScan(scanCallback)
        Log.d("BleManager", "BLE scan stopped")
    }

    // Scan callback that handles discovered devices
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            result?.device?.let { device ->
                Log.d("BleManager", "Found device: ${device.name} - ${device.address}")
                // Connect to the first discovered device (add filtering as needed)
                if (bluetoothGatt == null) {
                    connectToDevice(device)
                }
            }
        }

        override fun onScanFailed(errorCode: Int) {
            Log.e("BleManager", "BLE scan failed with error code: $errorCode")
        }
    }

    // Connect to the discovered BLE device
    private fun connectToDevice(device: BluetoothDevice) {
        Log.d("BleManager", "Connecting to device: ${device.address}")
        bluetoothGatt = device.connectGatt(context, false, gattCallback)
    }

    // Handle connection events, service discovery, and data reception
    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt?, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d("BleManager", "Connected to GATT server. Discovering services...")
                bluetoothGatt?.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.d("BleManager", "Disconnected from GATT server.")
                bluetoothGatt?.close()
                bluetoothGatt = null
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                gatt?.services?.forEach { service ->
                    service.characteristics.forEach { characteristic ->
                        // Enable notifications if supported
                        if (characteristic.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY != 0) {
                            setCharacteristicNotification(gatt, characteristic, true)
                        }
                        // Also perform an initial read of the characteristic
                        gatt.readCharacteristic(characteristic)
                    }
                }
            } else {
                Log.w("BleManager", "onServicesDiscovered received: $status")
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
                    Log.d("BleManager", "Characteristic read: ${data.contentToString()}")
                }
            }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?
        ) {
            characteristic?.value?.let { data ->
                receivedData.add(data)
                Log.d("BleManager", "Characteristic changed: ${data.contentToString()}")
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
        } else {
            Log.w("BleManager", "Descriptor not found for characteristic ${characteristic.uuid}")
        }
    }
}
