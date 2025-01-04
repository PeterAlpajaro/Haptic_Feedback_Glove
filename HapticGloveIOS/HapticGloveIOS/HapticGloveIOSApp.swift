//
//  HapticGloveIOSApp.swift
//  HapticGloveIOS
//
//  Created by Peter Alpajaro on 11/22/24.
//

import SwiftUI
import CoreBluetooth
import Combine

// Manages variables between and through different classes / objects.
class ViewManager: ObservableObject {
    @Published var bluetoothManager: BluetoothManager
    @Published var isConnected: Bool = false
    @Published var visibleDevices: [ListItem] = []
    @Published var temperature: Int = 10
    
    private var cancellables = Set<AnyCancellable>()

    init() {
        bluetoothManager = BluetoothManager()
        bluetoothManager.viewManager = self

        bluetoothManager.$visible_devices
            .receive(on: RunLoop.main)
            .sink { [weak self] devices in
                self?.visibleDevices = devices
            }
            .store(in: &cancellables)
    }
}

@main
struct HapticGloveIOSApp: App {
    @StateObject private var viewManager = ViewManager()
    
    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(viewManager)
        }
    }
}
