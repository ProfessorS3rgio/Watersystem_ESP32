#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use rand::RngCore;
use std::{fs, net::{TcpListener, TcpStream}, path::{Path, PathBuf}, process::{Child, Command, Stdio}, sync::Mutex, time::{Duration, Instant}};
use tauri::Manager;
mod serial;
#[cfg(windows)]
use std::os::windows::process::CommandExt;

struct Backend(Mutex<Option<Child>>);
impl Drop for Backend {
    fn drop(&mut self) {
        if let Ok(child) = self.0.get_mut() {
            if let Some(child) = child.as_mut() { let _ = child.kill(); let _ = child.wait(); }
        }
    }
}

fn php_command(php: &Path, backend: &Path, data: &Path, origin: &str, token: &str) -> Command {
    // Tauri resolves installed resources with a Windows verbatim (\\?\) prefix.
    // PHP can launch from that path but silently ignores php.ini passed with it.
    let php = dunce::simplified(php);
    let backend = dunce::simplified(backend);
    let data = dunce::simplified(data);
    let mut command = Command::new(php.join("php.exe"));
    command.arg("-c").arg(php.join("php.ini"))
        .arg("-d").arg(format!("extension_dir={}", php.join("ext").display())).current_dir(backend)
        .env("DMBC_DESKTOP_DATA", data).env("DMBC_DESKTOP_TOKEN", token)
        .env("APP_ENV", "production").env("APP_DEBUG", "false").env("APP_NAME", "DMBC Water")
        .env("APP_URL", origin).env("DB_CONNECTION", "sqlite").env("DB_DATABASE", format!("{}/dmbc.sqlite", data.display()))
        .env("DB_FOREIGN_KEYS", "true").env("SESSION_DRIVER", "file").env("SESSION_COOKIE", "dmbc_desktop_session")
        .env("CACHE_STORE", "file").env("QUEUE_CONNECTION", "sync").env("LOG_CHANNEL", "single");
    for (key, file) in [("APP_CONFIG_CACHE", "config.php"), ("APP_SERVICES_CACHE", "services.php"), ("APP_PACKAGES_CACHE", "packages.php"), ("APP_ROUTES_CACHE", "routes.php"), ("APP_EVENTS_CACHE", "events.php")] {
        command.env(key, data.join("cache").join(file));
    }
    #[cfg(windows)]
    command.creation_flags(0x08000000);
    command
}

fn start(app: &tauri::App) -> Result<(), Box<dyn std::error::Error>> {
    let resources = app.path().resource_dir()?;
    // tauri dev keeps resources in the source directory; bundles use resource_dir.
    let staged = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("resources");
    let resources = if resources.join("backend/desktop/initialize.php").is_file() { resources }
        else if cfg!(debug_assertions) { staged }
        else { return Err("The bundled DMBC backend is missing. Reinstall DMBC Water.".into()); };
    let backend = resources.join("backend");
    let php = resources.join("php");
    let data = dunce::simplified(&app.path().app_local_data_dir()?).to_path_buf();
    fs::create_dir_all(&data)?;
    let listener = TcpListener::bind("127.0.0.1:0")?;
    let port = listener.local_addr()?.port();
    let origin = format!("http://127.0.0.1:{port}");
    app.manage(serial::LocalOrigin(origin.clone()));
    let mut secret = [0u8; 32];
    rand::thread_rng().fill_bytes(&mut secret);
    let token: String = secret.iter().map(|byte| format!("{byte:02x}")).collect();
    let runtime = php_command(&php, &backend, &data, &origin, &token).arg("desktop/check-runtime.php").output()?;
    if !runtime.status.success() || !String::from_utf8_lossy(&runtime.stdout).contains("Bundled PHP extensions verified.") {
        fs::write(data.join("startup-error.log"), [&runtime.stdout[..], &runtime.stderr[..]].concat())?;
        return Err(format!("The bundled PHP runtime could not load its required extensions. See {}", data.join("startup-error.log").display()).into());
    }
    let output = php_command(&php, &backend, &data, &origin, &token).arg("desktop/initialize.php").output()?;
    if !output.status.success() || !String::from_utf8_lossy(&output.stdout).contains("DMBC SQLite profile ready.") {
        fs::write(data.join("startup-error.log"), [&output.stdout[..], &output.stderr[..]].concat())?;
        return Err(format!("DMBC could not prepare SQLite. See {}", data.join("startup-error.log").display()).into());
    }
    drop(listener);
    let log = fs::OpenOptions::new().create(true).append(true).open(data.join("backend.log"))?;
    let child = php_command(&php, &backend, &data, &origin, &token)
        .args(["-S", &format!("127.0.0.1:{port}"), "-t", "public", "desktop/router.php"])
        .stdin(Stdio::null()).stdout(log.try_clone()?).stderr(log).spawn()?;
    app.manage(Backend(Mutex::new(Some(child))));
    let deadline = Instant::now() + Duration::from_secs(15);
    loop {
        if let Some(status) = app.state::<Backend>().0.lock().unwrap().as_mut().unwrap().try_wait()? {
            return Err(format!("Local backend stopped ({status}). See backend.log in {}", data.display()).into());
        }
        if TcpStream::connect(("127.0.0.1", port)).is_ok() { break; }
        if Instant::now() > deadline { return Err("Local backend startup timed out".into()); }
        std::thread::sleep(Duration::from_millis(100));
    }
    let allowed_origin = origin.clone();
    let downloads = app.path().download_dir()?;
    let receipt_app = app.handle().clone();
    tauri::WebviewWindowBuilder::new(app, "main", tauri::WebviewUrl::External(format!("{origin}/desktop-launch/{token}").parse()?))
        .title("DMBC Water — Development").inner_size(1360.0, 900.0).min_inner_size(980.0, 680.0)
        .on_navigation(move |url| url.origin().ascii_serialization() == allowed_origin)
        .on_new_window(move |url, features| {
            // Receipts use window.open('') and write their escaped contents from Vue.
            // Receipt windows receive no native command capabilities.
            if url.as_str() != "about:blank" { return tauri::webview::NewWindowResponse::Deny; }
            let label = format!("receipt-{:x}", rand::random::<u64>());
            match tauri::WebviewWindowBuilder::new(&receipt_app, label, tauri::WebviewUrl::External(url))
                .window_features(features).title("DMBC payment receipt")
                .inner_size(460.0, 780.0)
                .on_navigation(|url| url.as_str() == "about:blank")
                .build() {
                    Ok(window) => tauri::webview::NewWindowResponse::Create { window },
                    Err(_) => tauri::webview::NewWindowResponse::Deny,
                }
        })
        .on_download(move |_webview, event| {
            if let tauri::webview::DownloadEvent::Requested { destination, .. } = event {
                let filename = destination.file_name().unwrap_or_else(|| std::ffi::OsStr::new("dmbc-download"));
                let mut path = downloads.join(filename);
                if path.exists() {
                    let stamp = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap_or_default().as_millis();
                    path = downloads.join(format!("{stamp}-{}", filename.to_string_lossy()));
                }
                *destination = path;
            }
            true
        })
        .build()?;
    Ok(())
}

fn report_startup_error(message: &str) {
    #[cfg(windows)]
    {
        #[link(name = "user32")]
        extern "system" {
            fn MessageBoxW(window: *mut std::ffi::c_void, text: *const u16, caption: *const u16, kind: u32) -> i32;
        }
        let text: Vec<u16> = format!("DMBC Water could not start.\n\n{message}\n\nYour saved records have not been removed.").encode_utf16().chain(Some(0)).collect();
        let title: Vec<u16> = "DMBC Water — Startup problem".encode_utf16().chain(Some(0)).collect();
        unsafe { MessageBoxW(std::ptr::null_mut(), text.as_ptr(), title.as_ptr(), 0x10); }
    }
    #[cfg(not(windows))]
    eprintln!("DMBC Water could not start: {message}");
}

fn main() {
    let application = tauri::Builder::default()
        .manage(serial::SerialState(Mutex::new(None)))
        .invoke_handler(tauri::generate_handler![serial::serial_ports, serial::serial_connect, serial::serial_write, serial::serial_disconnect])
        .on_window_event(|window, event| {
            if window.label() == "main" && matches!(event, tauri::WindowEvent::Destroyed) { window.app_handle().exit(0); }
        })
        .plugin(tauri_plugin_single_instance::init(|app, _, _| {
            if let Some(window) = app.get_webview_window("main") { let _ = window.unminimize(); let _ = window.set_focus(); }
        }))
        .setup(|app| { start(app)?; Ok(()) })
        .build(tauri::generate_context!());
    let application = match application {
        Ok(application) => application,
        Err(error) => { report_startup_error(&error.to_string()); std::process::exit(1); }
    };
    application.run(|app, event| {
            if let tauri::RunEvent::Exit = event {
                if let Some(backend) = app.try_state::<Backend>() {
                    if let Some(mut child) = backend.0.lock().unwrap().take() { let _ = child.kill(); let _ = child.wait(); }
                }
            }
        });
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[cfg(windows)]
    fn php_receives_normal_windows_paths_and_an_absolute_extension_directory() {
        let command = php_command(
            Path::new(r"\\?\C:\Users\Tester\DMBC Water\php"),
            Path::new(r"\\?\C:\Users\Tester\DMBC Water\backend"),
            Path::new(r"\\?\C:\Users\Tester\DMBC Data"),
            "http://127.0.0.1:12345", "test-token",
        );
        let arguments: Vec<_> = command.get_args().map(|value| value.to_string_lossy().into_owned()).collect();
        assert_eq!(arguments[1], r"C:\Users\Tester\DMBC Water\php\php.ini");
        assert_eq!(arguments[3], r"extension_dir=C:\Users\Tester\DMBC Water\php\ext");
        assert_eq!(command.get_current_dir().unwrap(), Path::new(r"C:\Users\Tester\DMBC Water\backend"));
    }
}
