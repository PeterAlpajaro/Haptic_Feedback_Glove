//
//  ContentView.swift
//  HapticGloveApp
//
//  Created by Peter Alpajaro on 11/21/24.
//

import SwiftUI
import CoreBluetooth

struct ContentView: View {
    
    @EnvironmentObject var viewManager: ViewManager
    
    var body: some View {
        
        if viewManager.isConnected == true {
            
            // View and processing with the camera and grid output.
            ProcessingView(viewManager: viewManager)
            
        } else {
            
            // List of comaptible devices to connect to.
            ConnectionView()
            
        }
        
    }
}

struct ListItem: Identifiable, Hashable ,Equatable {
    let id = UUID()
    let title: String
    let data: CBPeripheral
    
    func hash(into hasher: inout Hasher) {
        hasher.combine(id)
        
    }
    
    static func ==(lhs: ListItem, rhs: ListItem) -> Bool {
            lhs.id == rhs.id
    }
    
    
}

// View of connection options
struct ConnectionView: View {
    
    @EnvironmentObject var viewManager: ViewManager
    
    // TODO: list devices that can be connected to.
    var body: some View {
        
       
        
        @State var connection_status = false
        VStack {
            
            Text("Devices Found: \(viewManager.bluetoothManager.visible_devices.count)")
            
            let background_color = Color(hue: 0.1, saturation: 0.066, brightness: 1)
            
            VStack {
                
                // -- Connection Status Rectangle
                
                Rectangle()
                    .fill(background_color)
                    .frame(height: 60)  // I want to make the height equal to width so it's square
                    .overlay(
                        
                        // Inlaid Connection Text
                        HStack(alignment: .center) {
                            
                            // Text and image conditional on connection status
                            Image(systemName: "bolt.horizontal.circle.fill")
                                .resizable()
                                .frame(width: 30, height: 30) // Adjust the size
                                .foregroundColor(connection_status ? .blue : .gray)
                            
                            // --
                            
                            Text(connection_status ? "Bluetooth Connected" : "No Connection")
                                .foregroundColor(connection_status ? .blue : .gray) // Adjust text color
                                .font(.headline) // Adjust font style
                            
                        }
                            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
                    )
                
                // -- List of connected objects
                
            
                
                ScrollView {
                    LazyVStack(spacing: 10) {
                        
                        ForEach(viewManager.visibleDevices, id: \.self) { item in
                            let name = item.data.name ?? "Unknown Device"
                            ItemRow(text: name)
                                .onTapGesture {
                                    viewManager.bluetoothManager.connect(to: item.data);
                                }
                            
                            
                        }
                        
                    }
                    
                }
                
                // --
                
                Spacer()
                
                // -- Connection Button
                
                Button(action: {
                    connection_status = !connection_status
                    viewManager.bluetoothManager.writeData(timestamp: 1, float_grid:[[1.0, 2.0], [3.0, 4.0]])
                }) {
                    Rectangle()
                        .fill(background_color)
                        .frame(width: 100, height: 60)
                        .cornerRadius(20)
                        .overlay(
                            
                            HStack (alignment: .center) {
                                
                                Text("Connect")
                                
                                
                            }
                            
                            
                                .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
                        )
                }
                
                // --
                
                
                .padding()
            }
            
        }
        
    }
    
    
}

struct ItemRow: View {
    
    let text: String
        
    var body: some View {
        
        HStack {
            
            Text(text)
                .font(.system(.body))
            Spacer()
            
        }
        .padding()
        .background(Color.gray.opacity(0.1))
        .cornerRadius(8)
        
    }
    
}

// View of processing options
struct ProcessingView: View {
    
    
    @EnvironmentObject var viewManager: ViewManager
    
    @StateObject private var model: DataModel
    
    init(viewManager: ViewManager) {
        _model = StateObject(wrappedValue: DataModel(viewM: viewManager))
    }
    
    var body: some View {
            
            VStack {
                
                
                ViewfinderView(session: model.camera.captureSession)
                    .frame(height: UIScreen.main.bounds.height / 2)
                    .clipped()
//                DepthView(image: $model.depthImage).background(.black)
//                    .frame(height: UIScreen.main.bounds.height / 2)
//                    .clipped()
                GridView(model: model)
                    .frame(height: UIScreen.main.bounds.height / 2)
                    .clipped()
                
                
                
            }
            .edgesIgnoringSafeArea(.all)
        .task {
                        await model.camera.start()
                    }
                    .navigationTitle("Camera")
                    .navigationBarTitleDisplayMode(.inline)
                    .navigationBarHidden(true)
                    .statusBar(hidden: true)
                    .ignoresSafeArea()
        
        
        
    }
    
}


#Preview {
    ContentView()
        .environmentObject(ViewManager())
}
