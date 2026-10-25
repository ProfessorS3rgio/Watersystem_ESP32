fn main() {
    tauri_build::try_build(tauri_build::Attributes::new().app_manifest(
        tauri_build::AppManifest::new().commands(&["serial_ports", "serial_connect", "serial_write", "serial_disconnect"])
    )).expect("Failed to configure desktop permissions");
}
