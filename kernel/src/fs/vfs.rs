//! Minimal virtual filesystem layer.
//!
//! Before this module the routing between the persistent DFS root and the
//! volatile USTAR/RAMFS root was open-coded as `if crate::dfs::is_mounted()` at
//! the top of nearly every function in `ramfs.rs`. That coupling made the two
//! backends impossible to reason about independently.
//!
//! [`FileSystem`] is the (deliberately small) contract every backend implements,
//! and [`Vfs`] is the single place that decides which backend is active for a
//! given call. The rest of the kernel keeps using the `ramfs::*` facade, which
//! now delegates here instead of branching on the mounted state itself.
//!
//! This is intentionally *not* a complete VFS: there is no inode/dentry cache,
//! no mount table, no `switch_root`/`pivot_root`, and no per-file `File` object.
//! It removes the duplicated routing and gives a seam to grow those later.

use crate::dfs::{self, DfsError, Metadata};
use crate::ramfs::{self, FsError};

/// Error type returned by every [`FileSystem`] operation.
///
/// It unifies the two backend error enums so callers do not need to know which
/// backend served the request. Codes mirror the syscall ABI negative errnos via
/// [`VfsError::to_errno`].
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VfsError {
    InvalidPath,
    NotFound,
    NotDirectory,
    IsDirectory,
    AlreadyExists,
    DirectoryNotEmpty,
    NoSpace,
    Io,
}

impl VfsError {
    /// Maps to the negative errno used by the syscall ABI.
    pub fn to_errno(self) -> i64 {
        match self {
            VfsError::InvalidPath => -22,        // EINVAL
            VfsError::NotFound => -2,            // ENOENT
            VfsError::NotDirectory => -20,       // ENOTDIR
            VfsError::IsDirectory => -21,        // EISDIR
            VfsError::AlreadyExists => -17,      // EEXIST
            VfsError::DirectoryNotEmpty => -39,  // ENOTEMPTY
            VfsError::NoSpace => -28,            // ENOSPC
            VfsError::Io => -5,                  // EIO
        }
    }
}

impl From<FsError> for VfsError {
    fn from(error: FsError) -> Self {
        match error {
            FsError::InvalidPath => VfsError::InvalidPath,
            FsError::NotFound => VfsError::NotFound,
            FsError::NotDirectory => VfsError::NotDirectory,
            FsError::IsDirectory => VfsError::IsDirectory,
            FsError::AlreadyExists => VfsError::AlreadyExists,
            FsError::DirectoryNotEmpty => VfsError::DirectoryNotEmpty,
            FsError::NoSpace => VfsError::NoSpace,
        }
    }
}

impl From<DfsError> for VfsError {
    fn from(error: DfsError) -> Self {
        match error {
            DfsError::InvalidPath => VfsError::InvalidPath,
            DfsError::NotFound => VfsError::NotFound,
            DfsError::NotDirectory => VfsError::NotDirectory,
            DfsError::IsDirectory => VfsError::IsDirectory,
            DfsError::AlreadyExists => VfsError::AlreadyExists,
            DfsError::DirectoryNotEmpty => VfsError::DirectoryNotEmpty,
            DfsError::NoSpace | DfsError::JournalFull => VfsError::NoSpace,
            DfsError::Block(_) | DfsError::InvalidFilesystem | DfsError::CorruptMetadata => {
                VfsError::Io
            }
        }
    }
}

/// The operations a mounted filesystem must provide.
///
/// Implementations are stateless handles: the concrete backend (USTAR archive,
/// DFS on disk) holds its own state and this trait only describes the verbs.
pub trait FileSystem {
    /// Reads a whole file whose size was previously bounded by the caller.
    fn read_static(&self, path: &str) -> Option<&'static [u8]>;

    /// Reads up to `output.len()` bytes starting at `offset`.
    fn read_at(&self, path: &str, offset: usize, output: &mut [u8]) -> Result<usize, VfsError>;

    fn file_len(&self, path: &str) -> Result<usize, VfsError>;

    fn metadata(&self, path: &str) -> Option<Metadata>;

    fn is_directory(&self, path: &str) -> bool;

    fn is_file(&self, path: &str) -> bool;

    fn list_directory(
        &self,
        path: &str,
        include_hidden: bool,
        output: &mut [u8],
    ) -> Option<usize>;

    fn write_file_as(
        &self,
        path: &str,
        bytes: &[u8],
        uid: u32,
        gid: u32,
    ) -> Result<(), VfsError>;

    fn write_at(&self, path: &str, offset: usize, input: &[u8]) -> Result<usize, VfsError>;

    fn create_file_with_mode_as(
        &self,
        path: &str,
        mode: u16,
        uid: u32,
        gid: u32,
    ) -> Result<(), VfsError>;

    fn create_directory_with_parents_as(
        &self,
        path: &str,
        parents: bool,
        uid: u32,
        gid: u32,
    ) -> Result<(), VfsError>;

    fn remove(&self, path: &str, recursive: bool) -> Result<(), VfsError>;
}

/// Which backend serves filesystem calls.
///
/// The DFS functions are reached through the module-level `dfs::*` entry points,
/// which already hold the mounted instance in their own state; this enum only
/// records which side of the split is active.
#[derive(Clone, Copy, PartialEq, Eq)]
enum Backend {
    /// Persistent DFS mounted from the GPT partition laid over the ATA disk.
    Dfs,
    /// Volatile USTAR archive plus the in-memory overlay.
    RamFs,
}

fn active_backend() -> Backend {
    if dfs::is_mounted() {
        Backend::Dfs
    } else {
        Backend::RamFs
    }
}

/// Reports whether a persistent DFS root is currently mounted.
///
/// This is the one question the rest of the kernel still needs to ask directly
/// (for example to pick the default font source), so it stays a free function
/// rather than being buried in the [`FileSystem`] trait.
pub fn has_persistent_root() -> bool {
    dfs::is_mounted()
}

/// Routes [`FileSystem`] calls to whichever backend is mounted.
///
/// A new `Vfs` is cheap: it is just a snapshot of the current backend choice.
pub struct Vfs {
    backend: Backend,
}

impl Vfs {
    /// Captures the currently mounted backend.
    pub fn current() -> Self {
        Self {
            backend: active_backend(),
        }
    }

}

impl FileSystem for Vfs {
    fn read_static(&self, path: &str) -> Option<&'static [u8]> {
        match self.backend {
            Backend::Dfs => dfs::read_static(path),
            Backend::RamFs => ramfs::read_from_overlay(path),
        }
    }

    fn read_at(&self, path: &str, offset: usize, output: &mut [u8]) -> Result<usize, VfsError> {
        match self.backend {
            Backend::Dfs => dfs::read_at(path, offset, output).map_err(VfsError::from),
            Backend::RamFs => ramfs::read_at_overlay(path, offset, output).map_err(VfsError::from),
        }
    }

    fn file_len(&self, path: &str) -> Result<usize, VfsError> {
        match self.backend {
            Backend::Dfs => dfs::file_len(path).map_err(VfsError::from),
            Backend::RamFs => ramfs::file_len_overlay(path).map_err(VfsError::from),
        }
    }

    fn metadata(&self, path: &str) -> Option<Metadata> {
        match self.backend {
            Backend::Dfs => dfs::stat(path).ok(),
            Backend::RamFs => ramfs::metadata_overlay(path),
        }
    }

    fn is_directory(&self, path: &str) -> bool {
        match self.backend {
            Backend::Dfs => dfs::stat(path).is_ok_and(|metadata| metadata.is_directory),
            Backend::RamFs => ramfs::is_directory_overlay(path),
        }
    }

    fn is_file(&self, path: &str) -> bool {
        match self.backend {
            Backend::Dfs => dfs::stat(path).is_ok_and(|metadata| !metadata.is_directory),
            Backend::RamFs => ramfs::is_file_overlay(path),
        }
    }

    fn list_directory(
        &self,
        path: &str,
        include_hidden: bool,
        output: &mut [u8],
    ) -> Option<usize> {
        match self.backend {
            Backend::Dfs => dfs::list_directory(path, include_hidden, output).ok(),
            Backend::RamFs => ramfs::list_directory_overlay(path, include_hidden, output),
        }
    }

    fn write_file_as(&self, path: &str, bytes: &[u8], uid: u32, gid: u32) -> Result<(), VfsError> {
        match self.backend {
            Backend::Dfs => {
                dfs::write_file_as(path, bytes, 0o644, uid, gid).map_err(VfsError::from)
            }
            Backend::RamFs => ramfs::write_file_overlay(path, bytes, uid, gid).map_err(VfsError::from),
        }
    }

    fn write_at(&self, path: &str, offset: usize, input: &[u8]) -> Result<usize, VfsError> {
        match self.backend {
            Backend::Dfs => {
                dfs::write_at(path, offset, input, offset == 0).map_err(VfsError::from)
            }
            Backend::RamFs => {
                ramfs::write_at_overlay(path, offset, input).map_err(VfsError::from)
            }
        }
    }

    fn create_file_with_mode_as(
        &self,
        path: &str,
        mode: u16,
        uid: u32,
        gid: u32,
    ) -> Result<(), VfsError> {
        match self.backend {
            Backend::Dfs => dfs::write_at_as(path, 0, &[], true, mode & 0o777, uid, gid)
                .map(|_| ())
                .map_err(VfsError::from),
            Backend::RamFs => {
                ramfs::create_file_with_mode_overlay(path, mode, uid, gid).map_err(VfsError::from)
            }
        }
    }

    fn create_directory_with_parents_as(
        &self,
        path: &str,
        parents: bool,
        uid: u32,
        gid: u32,
    ) -> Result<(), VfsError> {
        match self.backend {
            Backend::Dfs => dfs::create_directory_with_parents_as(path, parents, uid, gid)
                .map_err(VfsError::from),
            Backend::RamFs => {
                ramfs::create_directory_with_parents_overlay(path, parents, uid, gid)
                    .map_err(VfsError::from)
            }
        }
    }

    fn remove(&self, path: &str, recursive: bool) -> Result<(), VfsError> {
        match self.backend {
            Backend::Dfs => dfs::remove(path, recursive).map_err(VfsError::from),
            Backend::RamFs => ramfs::remove_overlay(path, recursive).map_err(VfsError::from),
        }
    }
}
