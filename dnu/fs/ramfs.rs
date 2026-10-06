use core::cell::UnsafeCell;

const BLOCK_SIZE: usize = 512;
pub const MAX_WRITE_FILE_SIZE: usize = 4096;

pub struct Archive<'a> {
    bytes: &'a [u8],
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FsError {
    InvalidPath,
    NotFound,
    NotDirectory,
    IsDirectory,
    AlreadyExists,
    DirectoryNotEmpty,
    NoSpace,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum NodeKind {
    Empty,
    File,
    Directory,
    OpaqueDirectory,
    Whiteout,
}

#[derive(Clone, Copy)]
struct OverlayNode {
    path: [u8; 256],
    path_length: usize,
    kind: NodeKind,
    data: [u8; MAX_WRITE_FILE_SIZE],
    data_length: usize,
}

impl OverlayNode {
    const EMPTY: Self = Self {
        path: [0; 256],
        path_length: 0,
        kind: NodeKind::Empty,
        data: [0; MAX_WRITE_FILE_SIZE],
        data_length: 0,
    };

    fn path(&self) -> &str {
        core::str::from_utf8(&self.path[..self.path_length]).unwrap_or("")
    }
}

const MAX_OVERLAY_NODES: usize = 128;

struct RamFs {
    archive: Option<Archive<'static>>,
    overlay: [OverlayNode; MAX_OVERLAY_NODES],
}

impl RamFs {
    const fn new() -> Self {
        Self {
            archive: None,
            overlay: [OverlayNode::EMPTY; MAX_OVERLAY_NODES],
        }
    }
}

impl<'a> Archive<'a> {
    pub const fn new(bytes: &'a [u8]) -> Self {
        Self { bytes }
    }

    pub fn find(&self, wanted: &str) -> Option<&'a [u8]> {
        let wanted = wanted.trim_start_matches('/');
        let mut offset = 0usize;

        while offset.checked_add(BLOCK_SIZE)? <= self.bytes.len() {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                return None;
            }

            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let path_matches = if prefix.is_empty() {
                name == wanted.as_bytes()
            } else {
                wanted
                    .strip_prefix(core::str::from_utf8(prefix).ok()?)
                    .is_some_and(|suffix| {
                        suffix
                            .strip_prefix('/')
                            .is_some_and(|rest| rest.as_bytes() == name)
                    })
            };
            let size = parse_octal(&header[124..136])?;
            let data_start = offset.checked_add(BLOCK_SIZE)?;
            let data_end = data_start.checked_add(size)?;
            if data_end > self.bytes.len() {
                return None;
            }
            if path_matches && (header[156] == 0 || header[156] == b'0') {
                return Some(&self.bytes[data_start..data_end]);
            }

            let padded_size = size.checked_add(BLOCK_SIZE - 1)? & !(BLOCK_SIZE - 1);
            offset = data_start.checked_add(padded_size)?;
        }
        None
    }

    pub fn find_font(&self) -> Option<&'a [u8]> {
        let mut offset = 0usize;
        while offset.checked_add(BLOCK_SIZE)? <= self.bytes.len() {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                return None;
            }
            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let mut path = [0u8; 256];
            let path_len = if prefix.is_empty() {
                if name.len() > path.len() {
                    return None;
                }
                path[..name.len()].copy_from_slice(name);
                name.len()
            } else {
                let required = prefix.len().checked_add(1)?.checked_add(name.len())?;
                if required > path.len() {
                    return None;
                }
                path[..prefix.len()].copy_from_slice(prefix);
                path[prefix.len()] = b'/';
                path[prefix.len() + 1..required].copy_from_slice(name);
                required
            };
            let path = &path[..path_len];
            let in_font_directory = path.starts_with(b"system/fonts/");
            let supported_font = path.ends_with(b".psf") || path.ends_with(b".psf2");
            let size = parse_octal(&header[124..136])?;
            let data_start = offset.checked_add(BLOCK_SIZE)?;
            let data_end = data_start.checked_add(size)?;
            if data_end > self.bytes.len() {
                return None;
            }
            if in_font_directory && supported_font && (header[156] == 0 || header[156] == b'0') {
                return Some(&self.bytes[data_start..data_end]);
            }
            let padded_size = size.checked_add(BLOCK_SIZE - 1)? & !(BLOCK_SIZE - 1);
            offset = data_start.checked_add(padded_size)?;
        }
        None
    }

    pub fn is_directory(&self, wanted: &str) -> bool {
        let wanted = wanted.trim_start_matches('/');
        if wanted.is_empty() {
            return true;
        }
        let mut offset = 0usize;
        while offset
            .checked_add(BLOCK_SIZE)
            .is_some_and(|end| end <= self.bytes.len())
        {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                return false;
            }
            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let mut path = [0u8; 256];
            let path_len = if prefix.is_empty() {
                if name.len() > path.len() {
                    return false;
                }
                path[..name.len()].copy_from_slice(name);
                name.len()
            } else {
                let Some(required) = prefix
                    .len()
                    .checked_add(1)
                    .and_then(|n| n.checked_add(name.len()))
                else {
                    return false;
                };
                if required > path.len() {
                    return false;
                }
                path[..prefix.len()].copy_from_slice(prefix);
                path[prefix.len()] = b'/';
                path[prefix.len() + 1..required].copy_from_slice(name);
                required
            };
            let current = &path[..path_len];
            if (current == wanted.as_bytes() && header[156] == b'5')
                || current
                    .strip_prefix(wanted.as_bytes())
                    .is_some_and(|rest| rest.starts_with(b"/"))
            {
                return true;
            }
            let Some(size) = parse_octal(&header[124..136]) else {
                return false;
            };
            let Some(data_start) = offset.checked_add(BLOCK_SIZE) else {
                return false;
            };
            let Some(padded_size) = size
                .checked_add(BLOCK_SIZE - 1)
                .map(|size| size & !(BLOCK_SIZE - 1))
            else {
                return false;
            };
            let Some(next) = data_start.checked_add(padded_size) else {
                return false;
            };
            if next > self.bytes.len() {
                return false;
            }
            offset = next;
        }
        false
    }

    pub fn is_file(&self, wanted: &str) -> bool {
        self.find(wanted).is_some()
    }

    pub fn list_directory(&self, directory: &str, output: &mut [u8]) -> Option<usize> {
        let wanted = directory.trim_matches('/');
        let mut offset = 0usize;
        let mut written = 0usize;
        while offset.checked_add(BLOCK_SIZE)? <= self.bytes.len() {
            let header = &self.bytes[offset..offset + BLOCK_SIZE];
            if header.iter().all(|byte| *byte == 0) {
                break;
            }
            let name = field(&header[..100]);
            let prefix = field(&header[345..500]);
            let mut full_path = [0u8; 256];
            let path_length = if prefix.is_empty() {
                if name.len() > full_path.len() {
                    return None;
                }
                full_path[..name.len()].copy_from_slice(name);
                name.len()
            } else {
                let required = prefix.len().checked_add(1)?.checked_add(name.len())?;
                if required > full_path.len() {
                    return None;
                }
                full_path[..prefix.len()].copy_from_slice(prefix);
                full_path[prefix.len()] = b'/';
                full_path[prefix.len() + 1..required].copy_from_slice(name);
                required
            };
            let full_path = &full_path[..path_length];
            let relative = if wanted.is_empty() {
                Some(full_path)
            } else {
                full_path
                    .strip_prefix(wanted.as_bytes())
                    .and_then(|suffix| suffix.strip_prefix(b"/"))
            };
            if let Some(relative) = relative {
                if let Some(component) = relative.split(|byte| *byte == b'/').next() {
                    if !component.is_empty() {
                        let mut already_listed = false;
                        for existing in output[..written].split(|byte| *byte == b'\n') {
                            if existing == component {
                                already_listed = true;
                                break;
                            }
                        }
                        if !already_listed {
                            let required = written.checked_add(component.len())?.checked_add(1)?;
                            if required > output.len() {
                                return None;
                            }
                            output[written..written + component.len()].copy_from_slice(component);
                            output[required - 1] = b'\n';
                            written = required;
                        }
                    }
                }
            }
            let size = parse_octal(&header[124..136])?;
            let data_start = offset.checked_add(BLOCK_SIZE)?;
            let padded_size = size.checked_add(BLOCK_SIZE - 1)? & !(BLOCK_SIZE - 1);
            offset = data_start.checked_add(padded_size)?;
        }
        Some(written)
    }
}

struct SharedRamFs(UnsafeCell<RamFs>);
unsafe impl Sync for SharedRamFs {}
static MOUNTED_RAMFS: SharedRamFs = SharedRamFs(UnsafeCell::new(RamFs::new()));

pub fn mount(bytes: &'static [u8]) {
    let fs = unsafe { &mut *MOUNTED_RAMFS.0.get() };
    fs.archive = Some(Archive::new(bytes));
    fs.overlay = [OverlayNode::EMPTY; MAX_OVERLAY_NODES];
}

pub fn read(path: &str) -> Option<&'static [u8]> {
    let fs = unsafe { &*MOUNTED_RAMFS.0.get() };
    let path = canonical_path(path)?;
    if is_hidden(fs, path) {
        return None;
    }
    match find_overlay_node(fs, path).map(|node| node.kind) {
        Some(NodeKind::File) => {
            let node = find_overlay_node(fs, path)?;
            Some(&node.data[..node.data_length])
        }
        Some(NodeKind::Directory | NodeKind::OpaqueDirectory | NodeKind::Whiteout) => None,
        Some(NodeKind::Empty) | None => fs.archive.as_ref()?.find(path),
    }
}

pub fn find_font() -> Option<&'static [u8]> {
    let fs = unsafe { &*MOUNTED_RAMFS.0.get() };
    fs.archive.as_ref()?.find_font()
}

pub fn is_directory(path: &str) -> bool {
    let fs = unsafe { &*MOUNTED_RAMFS.0.get() };
    let Some(path) = canonical_path(path) else {
        return false;
    };
    match find_overlay_node(fs, path).map(|node| node.kind) {
        Some(NodeKind::Directory | NodeKind::OpaqueDirectory) => return true,
        Some(NodeKind::File | NodeKind::Whiteout) => return false,
        Some(NodeKind::Empty) | None => {}
    }
    if is_hidden(fs, path) {
        return false;
    }
    fs.archive
        .as_ref()
        .is_some_and(|archive| archive.is_directory(path))
        || fs.overlay.iter().any(|node| {
            matches!(
                node.kind,
                NodeKind::File | NodeKind::Directory | NodeKind::OpaqueDirectory
            ) && is_child_path(node.path(), path)
        })
}

pub fn is_file(path: &str) -> bool {
    let fs = unsafe { &*MOUNTED_RAMFS.0.get() };
    let Some(path) = canonical_path(path) else {
        return false;
    };
    if is_hidden(fs, path) {
        return false;
    }
    match find_overlay_node(fs, path).map(|node| node.kind) {
        Some(NodeKind::File) => true,
        Some(NodeKind::Directory | NodeKind::OpaqueDirectory | NodeKind::Whiteout) => false,
        Some(NodeKind::Empty) | None => fs
            .archive
            .as_ref()
            .is_some_and(|archive| archive.is_file(path)),
    }
}

pub fn list_directory(path: &str, output: &mut [u8]) -> Option<usize> {
    let fs = unsafe { &*MOUNTED_RAMFS.0.get() };
    let path = canonical_path(path)?;
    if !is_directory(path) {
        return None;
    }
    let opaque =
        find_overlay_node(fs, path).is_some_and(|node| node.kind == NodeKind::OpaqueDirectory);
    let mut entries = [0u8; 4096];
    let mut written = 0usize;
    if !opaque {
        if let Some(archive) = fs.archive.as_ref() {
            if archive.is_directory(path) {
                if let Some(base_length) = archive.list_directory(path, &mut entries) {
                    for name in entries[..base_length].split(|byte| *byte == b'\n') {
                        if name.is_empty() {
                            continue;
                        }
                        let mut child = [0u8; 256];
                        let child_length = join_child(path, name, &mut child)?;
                        if !is_hidden(fs, core::str::from_utf8(&child[..child_length]).ok()?) {
                            append_name(output, &mut written, name)?;
                        }
                    }
                }
            }
        }
    }
    for node in fs.overlay.iter().filter(|node| {
        matches!(
            node.kind,
            NodeKind::File | NodeKind::Directory | NodeKind::OpaqueDirectory
        )
    }) {
        if let Some(Some(name)) = immediate_child(node.path(), path) {
            append_name(output, &mut written, name)?;
        }
    }
    Some(written)
}

pub fn create_file(path: &str) -> Result<(), FsError> {
    let path = canonical_path(path).ok_or(FsError::InvalidPath)?;
    if path.is_empty() {
        return Err(FsError::IsDirectory);
    }
    if is_file(path) {
        return Ok(());
    }
    if is_directory(path) {
        return Err(FsError::IsDirectory);
    }
    ensure_parent_directory(path)?;
    insert_overlay(path, NodeKind::File)
}

pub fn write_file(path: &str, bytes: &[u8]) -> Result<(), FsError> {
    if bytes.len() > MAX_WRITE_FILE_SIZE {
        return Err(FsError::NoSpace);
    }
    let path = canonical_path(path).ok_or(FsError::InvalidPath)?;
    if path.is_empty() {
        return Err(FsError::IsDirectory);
    }
    if is_directory(path) {
        return Err(FsError::IsDirectory);
    }
    if !is_file(path) {
        ensure_parent_directory(path)?;
    }
    insert_overlay(path, NodeKind::File)?;
    let fs = unsafe { &mut *MOUNTED_RAMFS.0.get() };
    let node = fs
        .overlay
        .iter_mut()
        .find(|node| node.kind == NodeKind::File && node.path() == path)
        .ok_or(FsError::NoSpace)?;
    node.data[..bytes.len()].copy_from_slice(bytes);
    node.data[bytes.len()..].fill(0);
    node.data_length = bytes.len();
    Ok(())
}

pub fn create_directory(path: &str) -> Result<(), FsError> {
    create_directory_with_parents(path, false)
}

pub fn create_directory_with_parents(path: &str, parents: bool) -> Result<(), FsError> {
    let path = canonical_path(path).ok_or(FsError::InvalidPath)?;
    if path.is_empty() {
        return Err(FsError::AlreadyExists);
    }
    if parents {
        let mut current = [0u8; 256];
        let mut current_length = 0usize;
        for component in path.split('/') {
            if current_length != 0 {
                current[current_length] = b'/';
                current_length += 1;
            }
            if current_length + component.len() > current.len() {
                return Err(FsError::InvalidPath);
            }
            current[current_length..current_length + component.len()]
                .copy_from_slice(component.as_bytes());
            current_length += component.len();
            let current_path = core::str::from_utf8(&current[..current_length])
                .map_err(|_| FsError::InvalidPath)?;
            if is_directory(current_path) {
                continue;
            }
            if is_file(current_path) {
                return Err(FsError::AlreadyExists);
            }
            create_directory_one(current_path)?;
        }
        return Ok(());
    }
    create_directory_one(path)
}

fn create_directory_one(path: &str) -> Result<(), FsError> {
    if is_file(path) || is_directory(path) {
        return Err(FsError::AlreadyExists);
    }
    ensure_parent_directory(path)?;
    let fs = unsafe { &*MOUNTED_RAMFS.0.get() };
    let hides_archive = fs
        .archive
        .as_ref()
        .is_some_and(|archive| archive.is_directory(path));
    insert_overlay(
        path,
        if hides_archive {
            NodeKind::OpaqueDirectory
        } else {
            NodeKind::Directory
        },
    )
}

pub fn remove(path: &str, recursive: bool) -> Result<(), FsError> {
    let path = canonical_path(path).ok_or(FsError::InvalidPath)?;
    if path.is_empty() {
        return Err(FsError::InvalidPath);
    }
    if !is_file(path) && !is_directory(path) {
        return Err(FsError::NotFound);
    }
    if is_directory(path) {
        let mut children = [0u8; 4096];
        if list_directory(path, &mut children).unwrap_or(0) != 0 && !recursive {
            return Err(FsError::DirectoryNotEmpty);
        }
    }
    let fs = unsafe { &mut *MOUNTED_RAMFS.0.get() };
    let exists_in_archive = fs
        .archive
        .as_ref()
        .is_some_and(|archive| archive.is_file(path) || archive.is_directory(path));
    if exists_in_archive {
        insert_overlay(path, NodeKind::Whiteout)?;
    } else {
        for node in fs.overlay.iter_mut() {
            if node.kind != NodeKind::Empty
                && (node.path() == path || is_child_path(node.path(), path))
            {
                *node = OverlayNode::EMPTY;
            }
        }
    }
    Ok(())
}

fn find_overlay_node<'a>(fs: &'a RamFs, path: &str) -> Option<&'a OverlayNode> {
    fs.overlay
        .iter()
        .find(|node| node.kind != NodeKind::Empty && node.path() == path)
}

fn is_hidden(fs: &RamFs, path: &str) -> bool {
    if let Some(node) = find_overlay_node(fs, path) {
        return node.kind == NodeKind::Whiteout;
    }
    fs.overlay.iter().any(|node| {
        (node.kind == NodeKind::Whiteout
            && (node.path() == path || is_child_path(path, node.path())))
            || (node.kind == NodeKind::OpaqueDirectory && is_child_path(path, node.path()))
    })
}

fn is_child_path(path: &str, parent: &str) -> bool {
    if parent.is_empty() {
        return !path.is_empty();
    }
    path.strip_prefix(parent)
        .is_some_and(|suffix| suffix.starts_with('/'))
}

fn canonical_path(path: &str) -> Option<&str> {
    if path.is_empty() {
        return Some(path);
    }
    let path = path.strip_prefix('/').unwrap_or(path);
    if path.is_empty() {
        return Some(path);
    }
    if path
        .split('/')
        .any(|component| component.is_empty() || component == "." || component == "..")
    {
        return None;
    }
    Some(path)
}

fn ensure_parent_directory(path: &str) -> Result<(), FsError> {
    let parent = path.rsplit_once('/').map_or("", |(parent, _)| parent);
    if is_directory(parent) {
        Ok(())
    } else {
        Err(FsError::NotDirectory)
    }
}

fn insert_overlay(path: &str, kind: NodeKind) -> Result<(), FsError> {
    if path.len() > 256 {
        return Err(FsError::InvalidPath);
    }
    let fs = unsafe { &mut *MOUNTED_RAMFS.0.get() };
    let slot = fs
        .overlay
        .iter()
        .position(|node| node.kind != NodeKind::Empty && node.path() == path)
        .or_else(|| {
            fs.overlay
                .iter()
                .position(|node| node.kind == NodeKind::Empty)
        })
        .ok_or(FsError::NoSpace)?;
    let node = &mut fs.overlay[slot];
    node.path.fill(0);
    node.path[..path.len()].copy_from_slice(path.as_bytes());
    node.path_length = path.len();
    node.kind = kind;
    node.data.fill(0);
    node.data_length = 0;
    Ok(())
}

fn join_child(parent: &str, name: &[u8], output: &mut [u8]) -> Option<usize> {
    let prefix = parent.len() + usize::from(!parent.is_empty());
    if prefix + name.len() > output.len() {
        return None;
    }
    output[..parent.len()].copy_from_slice(parent.as_bytes());
    if !parent.is_empty() {
        output[parent.len()] = b'/';
    }
    output[prefix..prefix + name.len()].copy_from_slice(name);
    Some(prefix + name.len())
}

fn immediate_child<'a>(path: &'a str, parent: &str) -> Option<Option<&'a [u8]>> {
    let relative = if parent.is_empty() {
        path
    } else if path == parent {
        return Some(None);
    } else {
        path.strip_prefix(parent)?.strip_prefix('/')?
    };
    if relative.is_empty() {
        return Some(None);
    }
    Some(Some(relative.split('/').next()?.as_bytes()))
}

fn append_name(output: &mut [u8], written: &mut usize, name: &[u8]) -> Option<()> {
    if output[..*written]
        .split(|byte| *byte == b'\n')
        .any(|existing| existing == name)
    {
        return Some(());
    }
    let end = written.checked_add(name.len())?.checked_add(1)?;
    if end > output.len() {
        return None;
    }
    output[*written..*written + name.len()].copy_from_slice(name);
    output[end - 1] = b'\n';
    *written = end;
    Some(())
}

fn field(bytes: &[u8]) -> &[u8] {
    let end = bytes
        .iter()
        .position(|byte| *byte == 0)
        .unwrap_or(bytes.len());
    &bytes[..end]
}

fn parse_octal(bytes: &[u8]) -> Option<usize> {
    let mut value = 0usize;
    let mut found_digit = false;
    for byte in bytes
        .iter()
        .copied()
        .skip_while(|byte| *byte == b' ' || *byte == 0)
    {
        if byte == 0 || byte == b' ' {
            break;
        }
        if !(b'0'..=b'7').contains(&byte) {
            return None;
        }
        found_digit = true;
        value = value.checked_mul(8)?.checked_add((byte - b'0') as usize)?;
    }
    found_digit.then_some(value)
}

#[cfg(test)]
mod tests {
    use super::Archive;

    fn archive_with_file(prefix: &[u8], name: &[u8], contents: &[u8]) -> Vec<u8> {
        let size = (contents.len() + 511) & !511;
        let mut bytes = vec![0u8; 512 + size + 1024];
        bytes[..name.len()].copy_from_slice(name);
        bytes[124..135].copy_from_slice(format!("{:011o}", contents.len()).as_bytes());
        bytes[135] = 0;
        bytes[156] = b'0';
        bytes[257..262].copy_from_slice(b"ustar");
        bytes[345..345 + prefix.len()].copy_from_slice(prefix);
        bytes[512..512 + contents.len()].copy_from_slice(contents);
        bytes
    }

    #[test]
    fn finds_root_file_and_accepts_leading_slash() {
        let bytes = archive_with_file(b"", b"init", b"abc");
        let archive = Archive::new(&bytes);
        assert_eq!(archive.find("init"), Some(&b"abc"[..]));
        assert_eq!(archive.find("/init"), Some(&b"abc"[..]));
    }

    #[test]
    fn rejects_missing_and_truncated_entries() {
        let mut bytes = archive_with_file(b"", b"init", b"abc");
        let archive = Archive::new(&bytes);
        assert_eq!(archive.find("missing"), None);
        bytes[124..136].copy_from_slice(b"77777777777\0");
        assert_eq!(Archive::new(&bytes).find("init"), None);
    }

    #[test]
    fn finds_sbin_init_and_nested_psf_font() {
        let program = archive_with_file(b"sbin", b"init", b"ELF");
        assert_eq!(Archive::new(&program).find("/sbin/init"), Some(&b"ELF"[..]));
        assert!(Archive::new(&program).is_directory("sbin"));

        let font = archive_with_file(b"system/fonts", b"default8x9.psf", b"PSF");
        assert_eq!(Archive::new(&font).find_font(), Some(&b"PSF"[..]));
        assert!(Archive::new(&font).is_directory("system/fonts"));
        assert!(!Archive::new(&font).is_directory("sbin"));

        let program = archive_with_file(b"sbin", b"init", b"ELF");
        let mut entries = [0u8; 32];
        let length = Archive::new(&program)
            .list_directory("/sbin", &mut entries)
            .unwrap();
        assert_eq!(&entries[..length], b"init\n");
    }

    #[test]
    fn writable_overlay_creates_lists_and_removes_nodes() {
        let empty_archive: &'static [u8] = Box::leak(vec![0u8; 1024].into_boxed_slice());
        super::mount(empty_archive);
        assert_eq!(
            super::create_directory_with_parents("/var/log/app", true),
            Ok(())
        );
        assert_eq!(super::create_file("/var/log/app/empty.txt"), Ok(()));
        assert_eq!(super::create_file("/var/log/app/empty.txt"), Ok(()));
        assert_eq!(super::read("/var/log/app/empty.txt"), Some(&[][..]));
        assert_eq!(
            super::write_file("/var/log/app/empty.txt", "ação\n".as_bytes()),
            Ok(())
        );
        assert_eq!(
            super::read("/var/log/app/empty.txt"),
            Some("ação\n".as_bytes())
        );
        assert_eq!(
            super::write_file("/var/log/app/empty.txt", b"replacement"),
            Ok(())
        );
        assert_eq!(
            super::read("/var/log/app/empty.txt"),
            Some(&b"replacement"[..])
        );
        assert_eq!(
            super::write_file(
                "/var/log/app/empty.txt",
                &[0; super::MAX_WRITE_FILE_SIZE + 1]
            ),
            Err(super::FsError::NoSpace)
        );
        assert_eq!(
            super::create_file("/missing/file"),
            Err(super::FsError::NotDirectory)
        );

        let mut entries = [0u8; 128];
        let length = super::list_directory("/var/log/app", &mut entries).unwrap();
        assert_eq!(&entries[..length], b"empty.txt\n");
        assert_eq!(
            super::remove("/var/log/app", false),
            Err(super::FsError::DirectoryNotEmpty)
        );
        assert_eq!(super::remove("/var/log/app/empty.txt", false), Ok(()));
        assert_eq!(super::is_file("/var/log/app/empty.txt"), false);
        assert_eq!(super::remove("/var/log/app", true), Ok(()));
        assert_eq!(super::is_directory("/var/log/app"), false);
        assert_eq!(
            super::create_directory_with_parents("/tmp/a/b", true),
            Ok(())
        );
        assert!(super::is_directory("/tmp/a/b"));

        let lower_file: &'static [u8] =
            Box::leak(archive_with_file(b"bin", b"legacy", b"read-only").into_boxed_slice());
        super::mount(lower_file);
        assert_eq!(super::read("/bin/legacy"), Some(&b"read-only"[..]));
        assert_eq!(super::write_file("/bin/legacy", b"updated"), Ok(()));
        assert_eq!(super::read("/bin/legacy"), Some(&b"updated"[..]));
        assert_eq!(super::remove("/bin/legacy", false), Ok(()));
        assert_eq!(super::read("/bin/legacy"), None);
    }
}
