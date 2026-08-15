# Packaging & desktop integration

This directory and `cmake/KitesPackaging.cmake` provide three things:

1. **Desktop integration** — Kites appears in the applications view with a
   proper name, icon and description, instead of being a bare binary.
2. **A TUI installer** — one self-contained artifact that installs Kites on any
   Linux distribution by detecting the system and choosing the right paths.
3. **Native packages** — optional `.deb` / `.rpm` via CPack, plus a `PKGBUILD`
   for Arch.

The in-app update checker that pairs with this lives in [`src/updater/`](../src/updater).

---

## For reviewers: what this touches

The feature is deliberately additive. All the new logic is in new files; the
existing codebase is modified in exactly **two places**, both single hookups:

| File | Change |
| --- | --- |
| `CMakeLists.txt` | One `include(...)` line at the very end, pulling in `cmake/KitesPackaging.cmake`. |
| `src/ui/mainwindow/mainwindow.cpp` | One `#include` plus one `UpdateService::attachTo(helpMenu, this);` line in `setUpMenubar()`. |

Nothing existing was refactored, renamed or reformatted. Removing those two
hookups fully disables everything here.

New files:

```
cmake/KitesVersion.cmake              version + project identity (single source of truth)
cmake/KitesPackaging.cmake            install rules, installer target, CPack config
packaging/linux/kites.desktop.in      desktop entry template
packaging/linux/kites.metainfo.xml.in AppStream metadata (software centers)
packaging/linux/kites-installer.sh.in TUI installer template
packaging/linux/PKGBUILD.in           Arch package build file
src/updater/kites_version.h           version macros, with standalone fallbacks
src/updater/update_checker.{h,cpp}    GitHub Releases query + version compare
src/updater/update_service.{h,cpp}    the "Update available" prompt + menu action
```

`.in` files are templates: CMake substitutes `@KITES_VERSION@` and friends into
`build/packaging/`. Edit the templates, never the generated copies.

### Bumping the version

Edit `KITES_VERSION` in `cmake/KitesVersion.cmake` and tag the release
`v<version>`. The desktop entry, metainfo, installer, packages and the version
the update checker compares against all read from there.

---

## 1. Desktop integration

Built and installed by default (Linux only) — no options needed:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr .
cmake --build build
sudo cmake --install build
```

That installs:

| File | Purpose |
| --- | --- |
| `share/applications/io.github.hghit_harshit.Kites.desktop` | the launcher entry |
| `share/icons/hicolor/512x512/apps/io.github.hghit_harshit.Kites.png` | the icon |
| `share/metainfo/io.github.hghit_harshit.Kites.metainfo.xml` | AppStream data for GNOME Software / KDE Discover |

The app ID is `io.github.hghit_harshit.Kites` — the convention for
GitHub-hosted apps, with the `-` in the owner name replaced by `_` because
AppStream IDs disallow hyphens.

---

## 2. TUI installer

```bash
cmake -B build -DKITES_BUILD_INSTALLER=ON .
cmake --build build --target kites-installer
# -> build/installer/kites-installer-<version>-<arch>.tar.gz
```

Ship that tarball. The user extracts it and runs the script:

```bash
tar xzf kites-installer-1.0.0-x86_64.tar.gz
cd kites-installer-1.0.0-x86_64
./kites-installer.sh              # interactive menu
```

It also runs non-interactively for scripted installs:

```bash
./kites-installer.sh install
./kites-installer.sh uninstall
./kites-installer.sh info                     # show what it detected, change nothing
PREFIX=/opt/kites ./kites-installer.sh install
```

What it does:

- **Detects the distribution** from `/etc/os-release` and classifies it into a
  family (debian / fedora / arch / suse / alpine) to give correct advice.
- **Chooses the prefix by privilege**: `/usr/local` as root, `~/.local`
  otherwise. Both are already scanned for `.desktop` files, so the launcher
  appears either way — no root required for a working install.
- **Rewrites `Exec=` to the absolute installed path**, so launching works
  regardless of the session's `PATH`.
- **Refreshes `update-desktop-database` and `gtk-update-icon-cache`**, so Kites
  shows up in the applications view immediately rather than after a re-login.
- **Checks runtime dependencies** with `ldd` and names the Qt 6 package for the
  detected distro if anything is missing.
- **Writes an install manifest**, so `uninstall` removes exactly what was
  installed rather than guessing.

> **Note:** the payload contains the Kites binary but not the Qt runtime, so the
> target machine needs Qt 6 installed (the installer checks and tells the user
> what to install). For a fully self-contained artifact with Qt bundled, use the
> AppImage produced by `.github/workflows/release.yml`.

---

## 3. Native packages

### Debian / Ubuntu and Fedora / RHEL (CPack)

```bash
cmake -B build -DKITES_ENABLE_CPACK=ON .
cmake --build build
cd build
cpack -G DEB      # needs dpkg-deb
cpack -G RPM      # needs rpmbuild
```

These reuse the same `install()` rules as `cmake --install`, so package contents
can't drift from a normal install.

### Arch

`build/packaging/PKGBUILD` is generated with the version filled in.

```bash
cd build/packaging
makepkg -si
```

Before publishing to the AUR, replace the `SKIP` checksum with a real one
(`makepkg -g`) once the release tag exists — see the comments in the file.

---

## 4. Update checking

`src/updater/` adds **Help → Check for Updates…**, plus a quiet automatic check
a few seconds after launch (at most once every 24 h, tracked in `QSettings`).

When a newer release exists, the user gets a prompt with **Update Now** and
**Later**. **Update Now replaces the running Kites in place** and offers to
restart.

### How the self-update works

1. **Pick an asset** — prefers a bare `Kites*.AppImage` from the release;
   otherwise falls back to the `kites-linux-*.zip` the release workflow
   currently publishes.
2. **Download** with a progress dialog and a working Cancel, streamed to disk
   (the release is ~95 MB, so it is never buffered in memory). Staged in the
   same directory as the target so the final swap is a same-filesystem rename.
3. **Unpack**, if the asset was a zip. Qt ships no public zip reader, so this
   shells out to `unzip`, `bsdtar` or `python3` — whichever exists. It matches
   `Kites*.AppImage` specifically, because the current release zip *also*
   contains the `linuxdeploy` build tools (see "Known issues" below).
4. **Verify** — rejects anything under 1 MiB or lacking an ELF header, so a
   truncated download or an HTML error page can never be installed.
5. **Swap atomically** — the old binary is moved to `Kites.bak`, then the new
   one is renamed into place. Replacing a running executable is safe on Linux:
   `rename()` swaps the directory entry while the live process keeps its
   original inode. If the second step fails, the backup is moved straight back.
6. **Restart** on confirmation.

### Privileged installs

If the executable's directory is not writable (a `.deb`/`.rpm`/pacman install
in `/usr/bin`), the swap is done through `pkexec`, which prompts for
administrator authorisation. The elevated step stages to `Kites.new` and
finishes with `mv`, so it is atomic too.

The prompt warns first, because **this diverges from the package manager**: the
package database still believes the original version is installed, and the next
`apt`/`dnf`/`pacman` upgrade may overwrite the self-updated binary. That is
inherent to self-updating a packaged application, not a bug in this code.

If `pkexec` is unavailable, the update is refused with an explanation rather
than half-applied.

The updater is self-contained: it uses its own `QSettings` keys under
`updates/` and does not modify `AppSettings`.

---

## Known issues in the existing release pipeline

Found while building this; **not fixed here** because it is outside this PR's
files, but both are worth a follow-up:

1. **The Linux release zip contains three AppImages.** `.github/workflows/release.yml`
   runs `zip "$archive_name" *.AppImage` in the directory where it downloaded
   `linuxdeployqt` and `linuxdeploy-plugin-qt`, so both build tools get shipped
   to users. The archive is 95 MB when Kites itself is 51 MB. Zipping only
   `Kites*.AppImage` would fix it, and would nearly halve update downloads.
2. **Publishing the raw `.AppImage` as a release asset** (in addition to, or
   instead of, the zip) would let the updater skip unpacking entirely and drop
   its dependency on an external `unzip`/`bsdtar`/`python3`. The updater already
   prefers a bare AppImage when one is present, so this needs no code change.

Also note `CMakeLists.txt` globs sources with `file(GLOB_RECURSE ...)` without
`CONFIGURE_DEPENDS`, so **adding a new source file requires re-running CMake**
manually or it silently will not be compiled.
