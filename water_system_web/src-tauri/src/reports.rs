use std::{fs::{self, OpenOptions}, io::{self, Write}, path::{Path, PathBuf}};
use tauri::{Manager, State, WebviewWindow};
use crate::serial::LocalOrigin;

fn save_report(directory: &Path, filename: &str, bytes: &[u8]) -> Result<PathBuf, String> {
    // Accept a report filename, never a caller-supplied destination path.
    if !filename.starts_with("monthly-billing-report-") || !filename.ends_with(".xlsx")
        || filename.len() > 180 || filename.chars().any(|c| c.is_control() || "<>:\"/\\|?*".contains(c)) {
        return Err("Invalid report filename".into());
    }
    if bytes.len() < 4 || bytes.len() > 32 * 1024 * 1024 || !bytes.starts_with(b"PK\x03\x04") {
        return Err("The downloaded report is not a valid Excel workbook".into());
    }
    fs::create_dir_all(directory).map_err(|e| format!("Could not open Downloads: {e}"))?;
    let stem = filename.strip_suffix(".xlsx").unwrap();
    for suffix in 0..10000 {
        let name = if suffix == 0 { filename.to_owned() } else { format!("{stem} ({suffix}).xlsx") };
        let path = directory.join(name);
        match OpenOptions::new().write(true).create_new(true).open(&path) {
            Ok(mut file) => {
                let result = file.write_all(bytes).and_then(|_| file.sync_all());
                drop(file);
                if let Err(error) = result {
                    let _ = fs::remove_file(&path);
                    return Err(format!("Could not save the report: {error}"));
                }
                return Ok(path);
            }
            Err(error) if error.kind() == io::ErrorKind::AlreadyExists => continue,
            Err(error) => return Err(format!("Could not save the report: {error}")),
        }
    }
    Err("Too many reports with this name in Downloads".into())
}

fn reveal(path: &Path) -> bool {
    #[cfg(windows)]
    {
        use std::os::windows::process::CommandExt;
        // Explorer expects /select,"path" as one command-line expression.
        // Windows filenames cannot contain quotes; the path is created above.
        std::process::Command::new("explorer.exe")
            .raw_arg(format!("/select,\"{}\"", dunce::simplified(path).display()))
            .spawn().is_ok()
    }
    #[cfg(not(windows))]
    { let _ = path; false }
}

#[tauri::command]
pub async fn save_billing_report(window: WebviewWindow, origin: State<'_, LocalOrigin>, filename: String, bytes: Vec<u8>) -> Result<(String, bool), String> {
    if window.label() != "main" || window.url().map_err(|e| e.to_string())?.origin().ascii_serialization() != origin.0 {
        return Err("Report exports are limited to the local DMBC window".into());
    }
    let directory = window.app_handle().path().download_dir().map_err(|e| e.to_string())?;
    tauri::async_runtime::spawn_blocking(move || {
        let path = save_report(&directory, &filename, &bytes)?;
        let revealed = reveal(&path);
        Ok((path.to_string_lossy().into_owned(), revealed))
    }).await.map_err(|e| e.to_string())?
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn preserves_existing_exports_and_rejects_invalid_input() {
        let dir = std::env::temp_dir().join(format!("dmbc-report-test-{:x}", rand::random::<u64>()));
        let name = "monthly-billing-report-All-Barangays-2026-10.xlsx";
        let first = save_report(&dir, name, b"PK\x03\x04first").unwrap();
        let second = save_report(&dir, name, b"PK\x03\x04second").unwrap();
        assert_ne!(first, second);
        assert_eq!(fs::read(first).unwrap(), b"PK\x03\x04first");
        assert_eq!(fs::read(second).unwrap(), b"PK\x03\x04second");
        for bad in ["../report.xlsx", "monthly-billing-report-../outside.xlsx", "monthly-billing-report-C:\\outside.xlsx", "monthly-billing-report-test.xlsx:stream", "monthly-billing-report-\"test.xlsx"] {
            assert!(save_report(&dir, bad, b"PK\x03\x04data").is_err());
        }
        assert!(save_report(&dir, name, b"<html>sign in</html>").is_err());
        assert_eq!(fs::read_dir(&dir).unwrap().count(), 2);
        fs::remove_file(dir.join(name)).unwrap();
        fs::remove_file(dir.join("monthly-billing-report-All-Barangays-2026-10 (1).xlsx")).unwrap();
        fs::remove_dir(dir).unwrap();
    }
}
