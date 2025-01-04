//
//  GridView.swift
//  HapticGloveIOS
//
//  Created by Peter Alpajaro on 11/27/24.
//

// This shows the depth values returned from the depthAnything model.
// 2x2 grid for each vibration motor.

import AVFoundation
import SwiftUI

struct GridView: View {
    
    @ObservedObject var model: DataModel
    

    var body: some View {
        GeometryReader {geometry in
            VStack (spacing: 1) {
                HStack (spacing: 1) {
                    depthCell(region: 0, size: geometry.size.width / 2)
                    depthCell(region: 0, size: geometry.size.width / 2)
                    
                }
                HStack (spacing: 1) {
                    depthCell(region: 2, size: geometry.size.width / 2)
                    depthCell(region: 3, size: geometry.size.width / 2)
                }
                
                
            }
            
            
        }
    }
    
    private func depthCell(region: Int, size: CGFloat) -> some View {
            Text(String(format: "%.2f", model.regionDepths[region]))
                .frame(width: size, height: size)
                .background(Color.black.opacity(Double(model.regionDepths[region])))
                .foregroundColor(.white);
    }
    
}
//
//#Preview {
//    GridView(image: .constant(Image(systemName: "circle.rectangle.filled.pattern.diagonalline")))
//}
