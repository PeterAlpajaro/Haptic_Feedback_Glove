//
//  BluetoothManager.swift
//  HapticGloveApp
//
//  Created by Peter Alpajaro on 11/21/24.
//

import CoreBluetooth
import SwiftUICore

// Description of functionality:

//Sets up the bluetooth manager, which can be declared in any other class in the codebase.
//The bluetooth manager sets up the phone as a central manager, and treats the glove as a peripheral.
// Current functionality currently uses the writeData() function, which writes the values of a float array to the glove (the strength of each vibration motor)

class BluetoothManager: NSObject, ObservableObject {
    // MARK: Properties
    
    // For us to update parts of the UI
    weak var viewManager: ViewManager?
    
    private var centralManager: CBCentralManager!
    private var connectedPeripheral: CBPeripheral?
    public var readCharacteristic: CBCharacteristic?
    private var writeCharacteristic: CBCharacteristic?
    
    private var device_count = 0
    
    @Published private(set) var visible_devices: [ListItem] = [] {
        didSet {
            print("Visible devices updated \(visible_devices.map { $0.title })")
        }
    }
        
    
    // Service and characteristic UUIDs
    
    // TODO: Find and replace.
    private let serviceUUID = CBUUID(string: "00000000-0002-11e1-9ab4-0002a5d5c51b")
    private let readCharacteristicUUID = CBUUID(string: "00000001-0001-11e1-ac36-0002a5d5c51b")
    private let writeCharacteristicUUID = CBUUID(string: "00000001-0001-11e1-ac36-0002a5d5c51b")
    
    // Callbacks
    var onDataReceived: ((String) -> Void)?
    var onWriteComplete: ((Bool) -> Void)?
    var onConnectionChanged: ((Bool) -> Void)?
        
    
    override init() {
        super.init()
        // Q: What is the queue here?
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }
    
    func startScanning() {
        guard centralManager.state == .poweredOn else {
            print("Bluetooth is not available")
            return
        }
        
        visible_devices.removeAll()
        centralManager.scanForPeripherals(withServices: nil, options: nil)
        print("Scanning for peripherals...")
        
    }
    
    
    func stopScanning() {
        centralManager.stopScan()
        print("Scanning stopped")
        
    }
    
    func connect(to peripheral: CBPeripheral) {
        
        // Cancel our existing connection
        if let currentPeripheral = connectedPeripheral {
            centralManager.cancelPeripheralConnection(currentPeripheral)
        }
        
        // Connect to our new peripheral
        peripheral.delegate = self
        centralManager.connect(peripheral, options: nil)
        print("Connecting to: \(peripheral.name ?? "Unknown")")
    }
    
    func disconnect() {
        if let peripheral = connectedPeripheral {
            centralManager.cancelPeripheralConnection(peripheral)
        }
        
        
    }
    
    func printRawBits<T: FixedWidthInteger>(_ value: T) {
        let binaryString = String(value, radix: 2)
        let paddedString = binaryString.padding(toLength: 10, withPad: " ", startingAt: 0)
        print(paddedString)
    }
    
    func writeData(timestamp: UInt16, float_grid: [[Float]]) {
        guard let peripheral = connectedPeripheral,
              let characteristic = writeCharacteristic,
              float_grid.count == 2, float_grid[0].count == 2 else {
            print("Could not write: peripheral not ready or invalid data")
            return
        }
        
        var combinedData = Data()
        combinedData.append(UInt8(timestamp & 0xFF))
        combinedData.append(UInt8((timestamp >> 8) & 0xFF))
        
        for row in float_grid {
            for value in row {
                var littleEndianValue = value.bitPattern.littleEndian
                withUnsafeBytes(of: &littleEndianValue) { bytes in
                    combinedData.append(contentsOf: bytes)
                }
            }
            
        }
        
        peripheral.writeValue(combinedData, for: characteristic, type: .withResponse)
        
        print("Sending timestamp: \(timestamp), grid: \(float_grid)")
        print("Total data length: \(combinedData.count) bytes")
    }
    
    func readData() {
        guard let peripheral = connectedPeripheral,
              let characteristic = readCharacteristic else {
                print ("Cannot read: peripheral not ready")
                return
            }
        
        peripheral.readValue(for: characteristic)
        
    }
    
    
}


// These are the required methods to scan for peripherals when activated
extension BluetoothManager: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        
        if central.state == .poweredOn {
            print ("Beginning scanning for devices...")
            startScanning()
        } else {
            print ("Bluetooth manager state \(central.state)")
            
        }
        
        
    }
    
    // When a periopheral is discovered, add it to the list of devices seen by the app.
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {

        var contains = false
        for item in visible_devices {
            if item.data.name == peripheral.name {
                contains = true;
            }
        }
        
        
        DispatchQueue.main.async {
            
            if !contains {
                    
                    let new_device: ListItem = ListItem(title: peripheral.name ?? "Unknown", data: peripheral)
                    self.device_count += 1
                    self.visible_devices.append(new_device)
                    print("Found peripheral: \(new_device.data.name ?? "Unknown")")
                    // We want to check what device we have
                    print("Advertising as : \(advertisementData)\n\n")
            }
            
            
        }
        
        
        
        
        // Planned logic for this section:
        
        // Check advertisement data, if the device is the type that we expect, then display it on a list. Tapping that device will call the connection function.
        // TODO: Error handling here.
//        if (advertisementData["Type"] as! String == "HGlove") {
//            
//            
//            
//        }
        
    }
    
    // If a connection succeeds to a peripheral
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
            connectedPeripheral = peripheral
            peripheral.discoverServices([serviceUUID])
            print("Connected to peripheral")
            stopScanning()
            viewManager?.isConnected = true
        
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
            connectedPeripheral = nil
            writeCharacteristic = nil
            readCharacteristic = nil
            print("Disconnected from peripheral")
            startScanning() // Optionally restart scanning
    }
    
}

// Contains the required methods for the device to handle peripheral actions and characteristics
extension BluetoothManager: CBPeripheralDelegate {
    
    // Once connected, find the services. We can just auto choose this since we know exactly what this device should do.
    // TODO: Create and prepare the correct service and characteristics and match their UUIDs to what we have here.
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?)
    {
        guard let services = peripheral.services else { return }
        for service in services {
            // TODO: add write functionality
            peripheral.discoverCharacteristics([readCharacteristicUUID], for: service)
        }
    }
    
    // Once a service is found, characteristics are also found. We can also auto handle this.
    // TODO: Find the characteristics for the video data and the grid representing our haptic feedback
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error? ) {
        guard let characteristics = service.characteristics else {
            print ("Service has no characteristics")
            return
        }
        
        for characteristic in characteristics {
            if characteristic.uuid == readCharacteristicUUID {
                readCharacteristic = characteristic
                
                print("Camera data characteristic found")
                
//                let stringInt = String.init(data: readCharacteristic?.value ?? Data(), encoding: String.Encoding.utf8)
//                let int = Int.init(stringInt ?? "")
//                viewManager?.temperature = int ?? -5
            }
            
            if characteristic.uuid == writeCharacteristicUUID {
                writeCharacteristic = characteristic
                print ("Haptic Grid characteristic found")
                
                //peripheral.setNotifyValue(true, for: characteristic)
            }
        }
        
        
    }
    
    
    // TODO: This is where we process the camera data. How do we handle concurrency here?
    // If a characteristic has been updated by the peripheral, then run our callback sending the data.
    func peripheral (_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil, let data = characteristic.value else {
                print("Error reading characteristic value: \(error?.localizedDescription ?? "Unknown error")")
                return
        }
            
        // Assuming the data is a single float
        if data.count >= MemoryLayout<Float>.size {
                let floatValue = data.withUnsafeBytes { $0.load(as: Float.self) }
                print("Received float value: \(floatValue)")
                // Process the float value as needed
        } else {
                print("Received data is too short to be a float")
        }
        
        //onDataReceived?(message)
        
    }
    
    
    // TODO: This is where we can check if we've successfully written our feedback values to the glove.
    // If a characteristic has been sucessfully written, then run our callback sending the status.
    func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        let success = error == nil
        onWriteComplete?(success)
        
        
        if let error = error {
            print ("Write failed: (\(error.localizedDescription))")
            
            
            
        }
        
        
    }
}
