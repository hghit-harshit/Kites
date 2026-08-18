# =============================================================================
# KitesPackaging.cmake - Linux desktop integration, packaging and versioning.
#
# This is the ONLY file the root CMakeLists.txt needs to include for all of the
# packaging work (it pulls in KitesVersion.cmake itself). It is included at the
# very end of the root CMakeLists so the RISC_V_Simulator target already exists.
#
# What it does:
#   1. Feeds the version / repo identity to the app as compile definitions,
#      so the in-app update checker (src/updater/) knows what it is and where
#      to look for new releases.
#   2. Generates and installs the .desktop entry, icon and AppStream metainfo,
#      which is what makes Kites appear in the applications view.
#   3. -DKITES_BUILD_INSTALLER=ON  -> adds a `kites-installer` target that
#      produces a self-contained TUI installer tarball.
#   4. -DKITES_ENABLE_CPACK=ON     -> configures CPack DEB/RPM generators.
#
# Nothing here runs by default beyond the compile definitions and the install()
# rules; both extra features are opt-in behind options.
# =============================================================================

include("${CMAKE_CURRENT_LIST_DIR}/KitesVersion.cmake")

option(KITES_BUILD_INSTALLER "Build the self-contained Linux TUI installer" OFF)
option(KITES_ENABLE_CPACK "Configure CPack DEB/RPM package generators" OFF)

# -----------------------------------------------------------------------------
# 1. Version / identity available to the C++ code
#
# Done as compile definitions rather than a generated header so no include
# paths change. src/updater/kites_version.h provides fallbacks, so the updater
# still compiles if this module is ever not included.
# -----------------------------------------------------------------------------
target_compile_definitions(RISC_V_Simulator PRIVATE
    KITES_VERSION="${KITES_VERSION}"
    KITES_APP_ID="${KITES_APP_ID}"
    KITES_GITHUB_OWNER="${KITES_GITHUB_OWNER}"
    KITES_GITHUB_REPO="${KITES_GITHUB_REPO}"
    KITES_HOMEPAGE_URL="${KITES_HOMEPAGE_URL}"
)

if(ENABLE_TESTS AND TARGET tests)
    target_compile_definitions(tests PRIVATE
        KITES_VERSION="${KITES_VERSION}"
        KITES_APP_ID="${KITES_APP_ID}"
        KITES_GITHUB_OWNER="${KITES_GITHUB_OWNER}"
        KITES_GITHUB_REPO="${KITES_GITHUB_REPO}"
        KITES_HOMEPAGE_URL="${KITES_HOMEPAGE_URL}"
    )
endif()

# Everything below is Linux desktop integration and has no meaning on
# Windows/macOS, which have their own packaging in .github/workflows/release.yml.
if(NOT UNIX OR APPLE)
    return()
endif()

# -----------------------------------------------------------------------------
# 2. Desktop entry, icon and AppStream metainfo
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

set(KITES_PACKAGING_SRC "${CMAKE_CURRENT_LIST_DIR}/../packaging/linux")
set(KITES_PACKAGING_BIN "${CMAKE_BINARY_DIR}/packaging")
string(TIMESTAMP KITES_RELEASE_DATE "%Y-%m-%d" UTC)

# For distro packages the binary lands on PATH, so a bare name is correct here.
# The TUI installer rewrites this to an absolute path at install time, because
# it only knows the chosen prefix then.
set(KITES_EXEC_PATH "Kites")

configure_file(
    "${KITES_PACKAGING_SRC}/kites.desktop.in"
    "${KITES_PACKAGING_BIN}/${KITES_APP_ID}.desktop"
    @ONLY
)
configure_file(
    "${KITES_PACKAGING_SRC}/kites.metainfo.xml.in"
    "${KITES_PACKAGING_BIN}/${KITES_APP_ID}.metainfo.xml"
    @ONLY
)
configure_file(
    "${KITES_PACKAGING_SRC}/PKGBUILD.in"
    "${KITES_PACKAGING_BIN}/PKGBUILD"
    @ONLY
)

install(FILES "${KITES_PACKAGING_BIN}/${KITES_APP_ID}.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

install(FILES "${KITES_PACKAGING_BIN}/${KITES_APP_ID}.metainfo.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")

# The source icon is a 512x512 PNG. Icon themes downscale as needed, so a
# single hicolor size is enough for the launcher to render correctly.
install(FILES "${CMAKE_CURRENT_LIST_DIR}/../resources/icons/kite.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps"
        RENAME "${KITES_APP_ID}.png")

# -----------------------------------------------------------------------------
# 3. TUI installer  (-DKITES_BUILD_INSTALLER=ON, then: cmake --build build --target kites-installer)
#
# Stages a payload mirroring the final install layout next to the installer
# script, then tars the pair up. The result is one file you can hand to a user
# on any distribution: they extract it and run ./kites-installer.sh.
# -----------------------------------------------------------------------------
if(KITES_BUILD_INSTALLER)
    set(KITES_INSTALLER_NAME "kites-installer-${KITES_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
    set(KITES_INSTALLER_ROOT "${CMAKE_BINARY_DIR}/installer")
    set(KITES_INSTALLER_DIR "${KITES_INSTALLER_ROOT}/${KITES_INSTALLER_NAME}")
    set(KITES_INSTALLER_PAYLOAD "${KITES_INSTALLER_DIR}/payload")

    configure_file(
        "${KITES_PACKAGING_SRC}/kites-installer.sh.in"
        "${KITES_INSTALLER_DIR}/kites-installer.sh"
        @ONLY
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )

    add_custom_target(kites-installer
        COMMENT "Staging self-contained TUI installer: ${KITES_INSTALLER_NAME}.tar.gz"

        # Start from a clean payload so removed files never linger.
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${KITES_INSTALLER_PAYLOAD}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${KITES_INSTALLER_PAYLOAD}/bin"
                "${KITES_INSTALLER_PAYLOAD}/share/applications"
                "${KITES_INSTALLER_PAYLOAD}/share/icons/hicolor/512x512/apps"
                "${KITES_INSTALLER_PAYLOAD}/share/metainfo"

        COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:RISC_V_Simulator>"
                "${KITES_INSTALLER_PAYLOAD}/bin/Kites"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${KITES_PACKAGING_BIN}/${KITES_APP_ID}.desktop"
                "${KITES_INSTALLER_PAYLOAD}/share/applications/"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${KITES_PACKAGING_BIN}/${KITES_APP_ID}.metainfo.xml"
                "${KITES_INSTALLER_PAYLOAD}/share/metainfo/"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_CURRENT_LIST_DIR}/../resources/icons/kite.png"
                "${KITES_INSTALLER_PAYLOAD}/share/icons/hicolor/512x512/apps/${KITES_APP_ID}.png"

        COMMAND ${CMAKE_COMMAND} -E chdir "${KITES_INSTALLER_ROOT}"
                ${CMAKE_COMMAND} -E tar czf "${KITES_INSTALLER_NAME}.tar.gz" "${KITES_INSTALLER_NAME}"

        COMMAND ${CMAKE_COMMAND} -E echo
                "Installer ready: ${KITES_INSTALLER_ROOT}/${KITES_INSTALLER_NAME}.tar.gz"

        DEPENDS RISC_V_Simulator
        VERBATIM
    )

    # NOTE: the payload contains only the Kites binary, not the Qt runtime, so
    # the target machine needs Qt 6 installed. The installer detects missing
    # libraries via ldd and prints the right package name per distro. For a
    # fully self-contained artifact, use the AppImage produced by
    # .github/workflows/release.yml instead.
endif()

# -----------------------------------------------------------------------------
# 4. Native .deb / .rpm  (-DKITES_ENABLE_CPACK=ON, then: cpack -G DEB  /  cpack -G RPM)
#
# These reuse the install() rules above and in the root CMakeLists, so package
# contents never drift from a plain `cmake --install`.
# -----------------------------------------------------------------------------
if(KITES_ENABLE_CPACK)
    set(CPACK_PACKAGE_NAME "kites")
    set(CPACK_PACKAGE_VENDOR "${KITES_VENDOR}")
    set(CPACK_PACKAGE_VERSION "${KITES_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${KITES_SUMMARY}")
    # Each generator reads its own description variable; without these CPack
    # substitutes its "This is an installer created using CPack" boilerplate.
    set(KITES_PACKAGE_DESCRIPTION
"Kites is a RISC-V simulator and assembly code editor aimed at learning and
teaching computer architecture. It supports the I, M, D and F extensions in
single-cycle mode and the I and M extensions in 5-stage pipelined mode, with
configurable hazard detection and forwarding, a cache hierarchy with pluggable
replacement policies, and register, memory, cache and profiling inspectors.")
    set(CPACK_PACKAGE_DESCRIPTION "${KITES_PACKAGE_DESCRIPTION}")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "${KITES_PACKAGE_DESCRIPTION}")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${KITES_PACKAGE_DESCRIPTION}")
    set(CPACK_PACKAGE_HOMEPAGE_URL "${KITES_HOMEPAGE_URL}")
    set(CPACK_PACKAGE_CONTACT "${KITES_HOMEPAGE_URL}/issues")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/../LICENSE.md")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
    set(CPACK_PACKAGE_FILE_NAME
        "${CPACK_PACKAGE_NAME}-${KITES_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

    # Runtime dependencies are deliberately NOT hard-coded: Qt 6 package names
    # differ per distro (qt6-qtbase on Fedora, libqt6core6 on Debian,
    # libQt6Core6 on openSUSE). Both generators below derive them from the
    # binary's actual shared-library links instead, which is portable and
    # cannot drift as the link line changes.

    # Debian / Ubuntu
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)  # runs dpkg-shlibdeps
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # Fedora / RHEL
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
    set(CPACK_RPM_PACKAGE_AUTOREQ ON)       # soname-based Requires
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
    # Directories owned by the filesystem/hicolor packages must not be claimed.
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        "/usr/share/applications"
        "/usr/share/metainfo"
        "/usr/share/icons"
        "/usr/share/icons/hicolor"
        "/usr/share/icons/hicolor/512x512"
        "/usr/share/icons/hicolor/512x512/apps"
    )

    include(CPack)
endif()
