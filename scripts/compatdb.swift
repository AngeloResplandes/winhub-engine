// Signs and checks the compatibility database that WinHub downloads (data/compatdb.json).
//
//   swift scripts/compatdb.swift sign   [data/compatdb.json] [chave privada]
//   swift scripts/compatdb.swift verify [data/compatdb.json]
//
// The signature (Ed25519 over the file's exact bytes, base64) goes in <arquivo>.sig. The private key
// is base64 in ~/.winhub-signing/compatdb-ed25519.key (or $WINHUB_COMPATDB_KEY) and never enters the
// repository; data/compatdb-public-key.txt is the key built into WinHub. Raise "timestamp" whenever the
// rules change: WinHub refuses a database older than the one it already has.
import CryptoKit
import Foundation

func fail(_ message: String) -> Never {
    FileHandle.standardError.write(Data((message + "\n").utf8))
    exit(1)
}

func readBase64(_ path: String) -> Data {
    guard let text = try? String(contentsOfFile: path, encoding: .utf8),
          let data = Data(base64Encoded: text.trimmingCharacters(in: .whitespacesAndNewlines)) else {
        fail("não foi possível ler \(path)")
    }
    return data
}

/// The same checks winhubcompat makes before using a file.
func validate(_ data: Data, _ path: String) {
    guard let root = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
          root["version"] as? Int == 1, root["timestamp"] is NSNumber, root["rules"] is [Any] else {
        fail("\(path): precisa de \"version\": 1, \"timestamp\" (número) e \"rules\" (lista)")
    }
}

let arguments = Array(CommandLine.arguments.dropFirst())
guard let command = arguments.first, ["sign", "verify"].contains(command) else {
    fail("uso: swift scripts/compatdb.swift sign|verify [data/compatdb.json] [chave privada]")
}
let root = URL(fileURLWithPath: #filePath).deletingLastPathComponent().deletingLastPathComponent()
let file = arguments.count > 1 ? arguments[1] : root.appending(path: "data/compatdb.json").path
guard let data = FileManager.default.contents(atPath: file) else { fail("não foi possível ler \(file)") }
validate(data, file)

switch command {
case "sign":
    let keyPath = arguments.count > 2 ? arguments[2]
        : ProcessInfo.processInfo.environment["WINHUB_COMPATDB_KEY"]
        ?? NSString(string: "~/.winhub-signing/compatdb-ed25519.key").expandingTildeInPath
    guard let key = try? Curve25519.Signing.PrivateKey(rawRepresentation: readBase64(keyPath)) else {
        fail("chave privada inválida em \(keyPath)")
    }
    let published = readBase64(root.appending(path: "data/compatdb-public-key.txt").path)
    guard key.publicKey.rawRepresentation == published else {
        fail("a chave privada não corresponde a data/compatdb-public-key.txt")
    }
    let signature = try! key.signature(for: data)
    try! Data((signature.base64EncodedString() + "\n").utf8).write(to: URL(fileURLWithPath: file + ".sig"))
    print("assinado: \(file).sig")
default:
    guard let key = try? Curve25519.Signing.PublicKey(
        rawRepresentation: readBase64(root.appending(path: "data/compatdb-public-key.txt").path)) else {
        fail("chave pública inválida")
    }
    guard key.isValidSignature(readBase64(file + ".sig"), for: data) else {
        fail("\(file): assinatura inválida ou ausente (rode: swift scripts/compatdb.swift sign)")
    }
    print("assinatura válida: \(file)")
}
