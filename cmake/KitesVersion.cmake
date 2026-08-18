# ============================================================================
# KitesVersion.cmake - single source of truth for version + project identity.
#
# Everything that needs to know "what version is this" or "where do updates
# come from" reads it from here: the desktop entry, the AppStream metainfo,
# the TUI installer, the CPack packages, and the in-app update checker.
#
# Bump KITES_VERSION here (and tag the release as v<KITES_VERSION>) when
# cutting a release - nothing else needs editing.
# ============================================================================

# Overridable from the command line: -DKITES_VERSION=1.2.3
# Keep this in step with the latest published release tag, otherwise every user
# is told an update is available the moment they launch.
set(KITES_VERSION "1.1.1" CACHE STRING "Kites release version (major.minor.patch)")

# Reverse-DNS application ID. Must match the .desktop / metainfo filenames.
# GitHub-hosted projects conventionally use io.github.<owner>.<App>, with
# hyphens in the owner replaced by underscores (AppStream IDs disallow '-').
set(KITES_APP_ID "io.github.hghit_harshit.Kites")

# Where the in-app update checker looks for new releases.
set(KITES_GITHUB_OWNER "hghit-harshit")
set(KITES_GITHUB_REPO "Kites")
set(KITES_HOMEPAGE_URL "https://github.com/${KITES_GITHUB_OWNER}/${KITES_GITHUB_REPO}")

set(KITES_SUMMARY "RISC-V simulator and assembly code editor")
set(KITES_VENDOR "Atharva Gajendra Lohare, Harshit Daheriya")
