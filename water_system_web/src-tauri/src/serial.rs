use std::{io::{Read, Write}, sync::{Mutex, Arc, atomic::{AtomicBool, Ordering}}, thread::JoinHandle, time::Duration};
use tauri::{ipc::Channel, State, WebviewWindow};

pub struct SerialState(pub Mutex<Option<Connection>>);
pub struct LocalOrigin(pub String);
pub struct Connection {
    writer: Box<dyn serialport::SerialPort>,
    stopped: Arc<AtomicBool>,
    thread: Option<JoinHandle<()>>,
}
impl Drop for Connection {
    fn drop(&mut self) {
        self.stopped.store(true, Ordering::Relaxed);
        if let Some(thread) = self.thread.take() { let _ = thread.join(); }
    }
}
fn authorize(window: &WebviewWindow, origin: &LocalOrigin) -> Result<(), String> {
    if window.label() != "main" || window.url().map_err(|error| error.to_string())?.origin().ascii_serialization() != origin.0 {
        return Err("USB access is limited to the local DMBC window".into());
    }
    Ok(())
}

#[tauri::command]
pub fn serial_ports(window: WebviewWindow, origin: State<LocalOrigin>) -> Result<Vec<String>, String> {
    authorize(&window, &origin)?;
    serialport::available_ports().map(|ports| ports.into_iter().map(|port| port.port_name).collect()).map_err(|error| error.to_string())
}

#[tauri::command]
pub fn serial_connect(window: WebviewWindow, origin: State<LocalOrigin>, state: State<SerialState>, name: String, baud_rate: u32, channel: Channel<Vec<u8>>) -> Result<(), String> {
    authorize(&window, &origin)?;
    if baud_rate != 115200 { return Err("DMBC devices require 115200 baud".into()); }
    if !serialport::available_ports().map_err(|error| error.to_string())?.iter().any(|port| port.port_name == name) {
        return Err("That serial port is no longer available. Reconnect the USB cable.".into());
    }
    let mut connection = state.0.lock().map_err(|error| error.to_string())?;
    if connection.is_some() { return Err("Disconnect the current USB device first".into()); }
    let writer = serialport::new(name, baud_rate).timeout(Duration::from_millis(100)).open().map_err(|error| error.to_string())?;
    let mut reader = writer.try_clone().map_err(|error| error.to_string())?;
    let stopped = Arc::new(AtomicBool::new(false));
    let stop_reader = stopped.clone();
    let thread = std::thread::spawn(move || {
        let mut bytes = [0u8; 4096];
        while !stop_reader.load(Ordering::Relaxed) {
            match reader.read(&mut bytes) {
                Ok(0) => {},
                Ok(count) => { if channel.send(bytes[..count].to_vec()).is_err() { break; } },
                Err(error) if matches!(error.kind(), std::io::ErrorKind::TimedOut | std::io::ErrorKind::WouldBlock) => {},
                Err(_) => { let _ = channel.send(Vec::new()); break; },
            }
        }
    });
    *connection = Some(Connection { writer, stopped, thread: Some(thread) });
    Ok(())
}

#[tauri::command]
pub fn serial_write(window: WebviewWindow, origin: State<LocalOrigin>, state: State<SerialState>, line: String) -> Result<(), String> {
    authorize(&window, &origin)?;
    if line.len() > 262144 || line.contains(['\r', '\n']) { return Err("Invalid serial command".into()); }
    let mut connection = state.0.lock().map_err(|error| error.to_string())?;
    let connection = connection.as_mut().ok_or("USB device is disconnected")?;
    connection.writer.write_all(format!("{line}\n").as_bytes()).map_err(|error| error.to_string())
}

#[tauri::command]
pub fn serial_disconnect(window: WebviewWindow, origin: State<LocalOrigin>, state: State<SerialState>) -> Result<(), String> {
    authorize(&window, &origin)?;
    state.0.lock().map_err(|error| error.to_string())?.take();
    Ok(())
}
