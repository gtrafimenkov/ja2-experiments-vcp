#[cfg(not(target_os = "windows"))]
use std::os::unix::prelude::PermissionsExt;
use std::path::PathBuf;

/// Remove all files in a directory, but not the directory itself or any subdirectories.
pub fn remove_files_in_directory(path: &PathBuf) -> std::io::Result<()> {
    let iter = std::fs::read_dir(path)?;
    for entry in iter {
        let entry = entry?;
        if entry.path().is_file() || entry.path().is_symlink() {
            std::fs::remove_file(entry.path())?
        }
    }
    Ok(())
}

// Remove read-only attribute from a file.
#[cfg(target_os = "windows")]
#[allow(clippy::permissions_set_readonly_false)]
pub fn remove_readonly_attribute(path: &PathBuf) -> std::io::Result<()> {
    let mut perms = std::fs::metadata(path)?.permissions();
    perms.set_readonly(false);
    std::fs::set_permissions(path, perms)?;
    Ok(())
}

// Remove read-only attribute from a file.
#[cfg(not(target_os = "windows"))]
pub fn remove_readonly_attribute(path: &PathBuf) -> std::io::Result<()> {
    let mut perms = std::fs::metadata(path)?.permissions();
    perms.set_mode(0o600);
    Ok(())
}
