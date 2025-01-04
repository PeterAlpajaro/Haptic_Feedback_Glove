import CoreImage
import CoreML
import SwiftUI
import os

fileprivate let targetSize = CGSize(width: 518, height: 392)

final class DataModel: ObservableObject {
    let camera = Camera()
    let context = CIContext()

    weak var viewManager: ViewManager?
    
    /// The depth model.
    var model: DepthAnythingV2SmallF16P6?

    /// A pixel buffer used as input to the model.
    let inputPixelBuffer: CVPixelBuffer

    /// The last image captured from the camera.
    var lastImage = OSAllocatedUnfairLock<CIImage?>(uncheckedState: nil)

    /// The resulting depth image.
    @Published var depthImage: Image?
    
    // The resulting depth data
    @Published var depthData: CVPixelBuffer?
    
    // The resulting depthregions
    @Published var regionDepths: [Float] = [0, 0, 0, 0]
    
    init(viewM:    ViewManager) {
        // Create a reusable buffer to avoid allocating memory for every model invocation
        viewManager = viewM
        var buffer: CVPixelBuffer!
        let status = CVPixelBufferCreate(
            kCFAllocatorDefault,
            Int(targetSize.width),
            Int(targetSize.height),
            kCVPixelFormatType_32ARGB,
            nil,
            &buffer
        )
        guard status == kCVReturnSuccess else {
            fatalError("Failed to create pixel buffer")
        }
        inputPixelBuffer = buffer

        // Decouple running the model from the camera feed since the model will run slower
        Task.detached(priority: .userInitiated) {
            await self.runModel()
        }
        Task {
            await handleCameraFeed()
        }
    }
    
    func calculateRegionDepths() {
        guard let depthBuffer = depthData else { return }
        
        let width = CVPixelBufferGetWidth(depthBuffer)
        let height = CVPixelBufferGetHeight(depthBuffer)
        let quadWidth = width / 2
        let quadHeight = height / 2
        
        // Define the size of the center area to analyze (e.g., 25% of each quadrant)
        let centerWidth = quadWidth / 4
        let centerHeight = quadHeight / 4
        
        CVPixelBufferLockBaseAddress(depthBuffer, .readOnly)
        defer { CVPixelBufferUnlockBaseAddress(depthBuffer, .readOnly) }
        
        guard let baseAddress = CVPixelBufferGetBaseAddress(depthBuffer) else { return }
        
        let bytesPerRow = CVPixelBufferGetBytesPerRow(depthBuffer)
        
        // Downsample by processing every 2nd pixel
        let downsampleFactor = 2
        
        var minDepth: Float = Float.greatestFiniteMagnitude
        var maxDepth: Float = -Float.greatestFiniteMagnitude
        
        // Find min and max depths in the center areas
        for region in 0..<4 {
            let startX = (region % 2) * quadWidth + (quadWidth - centerWidth) / 2
            let startY = (region / 2) * quadHeight + (quadHeight - centerHeight) / 2
            
            for y in stride(from: startY, to: startY + centerHeight, by: downsampleFactor) {
                let rowOffset = y * bytesPerRow
                for x in stride(from: startX, to: startX + centerWidth, by: downsampleFactor) {
                    let offset = rowOffset + x * MemoryLayout<Float>.size
                    let depth = baseAddress.load(fromByteOffset: offset, as: Float.self)
                    minDepth = min(minDepth, depth)
                    maxDepth = max(maxDepth, depth)
                }
            }
        }
        
        let depthRange = max(maxDepth - minDepth, Float.ulpOfOne)
        
        var newRegionDepths: [Float] = [0, 0, 0, 0]
        
        DispatchQueue.concurrentPerform(iterations: 4) { region in
            var sum: Float = 0
            var count = 0
            let startX = (region % 2) * quadWidth + (quadWidth - centerWidth) / 2
            let startY = (region / 2) * quadHeight + (quadHeight - centerHeight) / 2
            
            for y in stride(from: startY, to: startY + centerHeight, by: downsampleFactor) {
                let rowOffset = y * bytesPerRow
                for x in stride(from: startX, to: startX + centerWidth, by: downsampleFactor) {
                    let offset = rowOffset + x * MemoryLayout<Float>.size
                    let depth = baseAddress.load(fromByteOffset: offset, as: Float.self)
                    sum += depth
                    count += 1
                }
            }
            
            let average = sum / Float(count)
            let normalizedAverage = (average - minDepth) / depthRange
            newRegionDepths[region] = normalizedAverage
        }
        
        self.viewManager?.bluetoothManager.writeData(timestamp: 1, float_grid: [[newRegionDepths[0], newRegionDepths[1]], [newRegionDepths[2], newRegionDepths[3]]])
        
        DispatchQueue.main.async {
            self.regionDepths = newRegionDepths
            print(self.regionDepths[0])
        }
    }

    
    func handleCameraFeed() async {
        let imageStream = camera.previewStream
        for await image in imageStream {
            lastImage.withLock({ $0 = image })
        }
    }

    func runModel() async {
        try! loadModel()

        let clock = ContinuousClock()
        var durations = [ContinuousClock.Duration]()

        while !Task.isCancelled {
            let image = lastImage.withLock({ $0 })
            if let pixelBuffer = image?.pixelBuffer {
                let duration = await clock.measure {
                    try? await performInference(pixelBuffer)
                }
                durations.append(duration)
            }

            let measureInterval = 100
            if durations.count == measureInterval {
                let total = durations.reduce(Duration(secondsComponent: 0, attosecondsComponent: 0), +)
                let average = total / measureInterval
                print("Average model runtime: \(average.formatted(.units(allowed: [.milliseconds])))")
                durations.removeAll(keepingCapacity: true)
            }

            // Slow down inference to prevent freezing the UI
            try? await Task.sleep(for: .milliseconds(200))
        }

    }

    func loadModel() throws {
        print("Loading model...")

        let clock = ContinuousClock()
        let start = clock.now

        model = try DepthAnythingV2SmallF16P6()

        let duration = clock.now - start
        print("Model loaded (took \(duration.formatted(.units(allowed: [.seconds, .milliseconds]))))")
    }

    func performInference(_ pixelBuffer: CVPixelBuffer) async throws {
        guard let model else {
            return
        }

        let originalSize = CGSize(width: CVPixelBufferGetWidth(pixelBuffer), height: CVPixelBufferGetHeight(pixelBuffer))
        let inputImage = CIImage(cvPixelBuffer: pixelBuffer).resized(to: targetSize)
        context.render(inputImage, to: inputPixelBuffer)
        let result = try model.prediction(image: inputPixelBuffer)
        let outputImage = CIImage(cvPixelBuffer: result.depth)
            .resized(to: originalSize)
            .image

        // TODO: Should this be here?
        Task { @MainActor in
            depthImage = outputImage
            depthData = result.depth
            calculateRegionDepths()
        }
    }
}

fileprivate extension CIImage {
    var image: Image? {
        let ciContext = CIContext()
        guard let cgImage = ciContext.createCGImage(self, from: self.extent) else { return nil }
        return Image(decorative: cgImage, scale: 1, orientation: .up)
    }
}
