// Writes the smoke test's video: 128x96 H.264 (15 light-gray frames at 15 fps) and one second of silent
// AAC in an MP4, made with macOS's own encoders so the file is independent of the decoders under test.
// Usage: swift scripts/make-test-video.swift <output.mp4>
import AVFoundation

guard CommandLine.arguments.count == 2 else {
    fputs("usage: make-test-video.swift <output.mp4>\n", stderr)
    exit(2)
}
let output = URL(fileURLWithPath: CommandLine.arguments[1])
try? FileManager.default.removeItem(at: output)
let writer = try AVAssetWriter(outputURL: output, fileType: .mp4)

let video = AVAssetWriterInput(mediaType: .video, outputSettings: [
    AVVideoCodecKey: AVVideoCodecType.h264, AVVideoWidthKey: 128, AVVideoHeightKey: 96])
let adaptor = AVAssetWriterInputPixelBufferAdaptor(assetWriterInput: video, sourcePixelBufferAttributes: [
    kCVPixelBufferPixelFormatTypeKey as String: kCVPixelFormatType_32BGRA,
    kCVPixelBufferWidthKey as String: 128, kCVPixelBufferHeightKey as String: 96])
let audio = AVAssetWriterInput(mediaType: .audio, outputSettings: [
    AVFormatIDKey: kAudioFormatMPEG4AAC, AVSampleRateKey: 44100, AVNumberOfChannelsKey: 2,
    AVEncoderBitRateKey: 64000])
writer.add(video)
writer.add(audio)
guard writer.startWriting() else {
    fputs("cannot write \(output.path): \(String(describing: writer.error))\n", stderr)
    exit(1)
}
writer.startSession(atSourceTime: .zero)

let group = DispatchGroup()
group.enter()
var frame: Int64 = 0
video.requestMediaDataWhenReady(on: DispatchQueue(label: "video")) {
    while video.isReadyForMoreMediaData && frame < 15 {
        var buffer: CVPixelBuffer?
        CVPixelBufferPoolCreatePixelBuffer(nil, adaptor.pixelBufferPool!, &buffer)
        let pixels = buffer!
        CVPixelBufferLockBaseAddress(pixels, [])
        memset(CVPixelBufferGetBaseAddress(pixels)!, 200, CVPixelBufferGetBytesPerRow(pixels) * 96)
        CVPixelBufferUnlockBaseAddress(pixels, [])
        adaptor.append(pixels, withPresentationTime: CMTime(value: frame, timescale: 15))
        frame += 1
    }
    if frame == 15 {
        video.markAsFinished()
        frame += 1
        group.leave()
    }
}

group.enter()
var audioDone = false
audio.requestMediaDataWhenReady(on: DispatchQueue(label: "audio")) {
    guard !audioDone, audio.isReadyForMoreMediaData else { return }
    let frames = 44100
    var format = AudioStreamBasicDescription(
        mSampleRate: 44100, mFormatID: kAudioFormatLinearPCM,
        mFormatFlags: kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked,
        mBytesPerPacket: 4, mFramesPerPacket: 1, mBytesPerFrame: 4, mChannelsPerFrame: 2,
        mBitsPerChannel: 16, mReserved: 0)
    var description: CMAudioFormatDescription?
    CMAudioFormatDescriptionCreate(allocator: nil, asbd: &format, layoutSize: 0, layout: nil,
                                   magicCookieSize: 0, magicCookie: nil, extensions: nil,
                                   formatDescriptionOut: &description)
    var block: CMBlockBuffer?
    CMBlockBufferCreateWithMemoryBlock(allocator: nil, memoryBlock: nil, blockLength: frames * 4,
                                       blockAllocator: nil, customBlockSource: nil, offsetToData: 0,
                                       dataLength: frames * 4, flags: 0, blockBufferOut: &block)
    CMBlockBufferFillDataBytes(with: 0, blockBuffer: block!, offsetIntoDestination: 0, dataLength: frames * 4)
    var sample: CMSampleBuffer?
    CMAudioSampleBufferCreateReadyWithPacketDescriptions(
        allocator: nil, dataBuffer: block!, formatDescription: description!, sampleCount: frames,
        presentationTimeStamp: .zero, packetDescriptions: nil, sampleBufferOut: &sample)
    audio.append(sample!)
    audio.markAsFinished()
    audioDone = true
    group.leave()
}

group.wait()
let finished = DispatchSemaphore(value: 0)
writer.finishWriting { finished.signal() }
finished.wait()
guard writer.status == .completed else {
    fputs("cannot write \(output.path): \(String(describing: writer.error))\n", stderr)
    exit(1)
}
print("wrote \(output.path)")
