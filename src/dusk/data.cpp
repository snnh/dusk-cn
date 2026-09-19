#include "data.hpp"

#include "dusk/app_info.hpp"
#include "dusk/ui/i18n.hpp"

#include <borealis/io.hpp>
#include <borealis/log.hpp>

#include <string>
#include <system_error>

namespace dusk::data {
namespace {

constexpr borealis::Log Log{"dusk::data"};

std::string status_message(const borealis::data::Status& status) {
    using enum borealis::data::ErrorCode;

    std::string message;
    switch (status.code) {
    case None:
        return {};
    case NotInitialized:
        message = "[DATA_FOLDERS_HAVE_NOT_BEEN_INITIALIZED]";
        break;
    case PreferencePathUnavailable:
        message = "[THE_SYSTEM_DATA_FOLDER_IS_UNAVAILABLE]";
        break;
    case EmptyPath:
        message = "[CHOOSE_A_FOLDER]";
        break;
    case CreateDirectoryFailed:
        message = "[THE_SELECTED_FOLDER_COULD_NOT_BE_CREATED]";
        break;
    case NotDirectory:
        message = "[THE_SELECTED_PATH_IS_NOT_A_FOLDER]";
        break;
    case WriteProbeFailed:
        message = "[THE_SELECTED_FOLDER_IS_NOT_WRITABLE]";
        break;
    case WriteProbeCleanupFailed:
        message = "[THE_SELECTED_FOLDER_COULD_NOT_BE_VALIDATED]";
        break;
    case DescriptorWriteFailed:
        message = "[DUSKLIGHT_COULD_NOT_SAVE_THE_DATA_FOLDER_SETTING]";
        break;
    case MigrationIncomplete:
        message = "[DATA_MIGRATION_COULD_NOT_BE_COMPLETED]";
        break;
    case OverrideActive:
        message = "[THE_DATA_FOLDER_IS_FIXED_BY_USER_DIR_FOR_THIS_SESSION]";
        break;
    case Unsupported:
        message = "[CHANGING_THE_DATA_FOLDER_IS_NOT_SUPPORTED_ON_THIS_PLATFORM]";
        break;
    case OpenFolderFailed:
        message = "[THE_DATA_FOLDER_COULD_NOT_BE_OPENED]";
        break;
    }

    if (!status.path.empty()) {
        message += " (" + borealis::io::fs_path_to_string(status.path) + ")";
    }
    if (status.systemError) {
        message += ": " + status.systemError.message();
    }
    return message;
}

bool operation_succeeded(const borealis::data::Status& status, std::string* errorOut = nullptr) {
    if (status) {
        return true;
    }
    const auto message = status_message(status);
    if (errorOut != nullptr) {
        *errorOut = message;
    }
    Log.warn("{}", ui::i18n::tr(message));
    return false;
}

}  // namespace

borealis::data::Manager& manager() {
    static borealis::data::Manager instance{
        AppInfo,
        {
            .defaultPath = {.useDocumentsOnIOS = true},
            .portableRelativePath = "data",
            .legacyApps =
                {
                    {.orgName = "TwilitRealm", .appName = "Dusk"},
                },
            .migration =
                {
                    .directories =
                        {
                            "texture_replacements",
                            "USA",
                            "EUR",
                            "JAP",
                            "MemoryCardA.USA.raw.mods",
                            "MemoryCardA.EUR.raw.mods",
                            "MemoryCardA.JAP.raw.mods",
                            "MemoryCardB.USA.raw.mods",
                            "MemoryCardB.EUR.raw.mods",
                            "MemoryCardB.JAP.raw.mods",
                        },
                    .files =
                        {
                            "achievements.json",
                            "config.json",
                            "controller_ports.dat",
                            "gamecontrollerdb.txt",
                            "imgui.ini",
                            "keyboard_bindings.dat",
                            "states.json",
                        },
                    .extensions = {".controller", ".gci"},
                    .filenamePatterns =
                        {
                            {.prefix = "MemoryCard", .suffix = ".raw"},
                        },
                },
        },
    };
    return instance;
}

Paths initialize_data(const std::filesystem::path& userDirectoryOverride) {
    const auto status = manager().initialize(userDirectoryOverride);
    if (!status && status.code != borealis::data::ErrorCode::MigrationIncomplete) {
        Log.fatal("Failed to initialize data folders: {}", ui::i18n::tr(status_message(status)));
    }
    if (!status) {
        Log.warn("{} Migration will be retried on the next launch.",
            ui::i18n::tr(status_message(status)));
    }
    return manager().paths();
}

std::filesystem::path base_path_relative(const std::filesystem::path& path) {
    return manager().base_path_relative(path);
}

std::filesystem::path portable_data_path() {
    return base_path_relative("data");
}

bool portable_marker_exists() {
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IOS) ||                               \
    (defined(__APPLE__) && TARGET_OS_TV)
    return false;
#else
    std::error_code ec;
    return std::filesystem::is_regular_file(base_path_relative("portable.txt"), ec);
#endif
}

std::filesystem::path configured_data_path() {
    return manager().configured_data_path();
}

std::filesystem::path cache_path() {
    return manager().paths().cachePath;
}

bool open_data_path() {
    return operation_succeeded(manager().open_active_data_path());
}

bool set_custom_data_path(const std::filesystem::path& path, std::string* errorOut) {
    return operation_succeeded(manager().set_custom_data_path(path), errorOut);
}

bool set_custom_data_path(const char* path, std::string* errorOut) {
    if (path == nullptr) {
        if (errorOut != nullptr) {
            *errorOut = "[CHOOSE_A_FOLDER]";
        }
        return false;
    }
    return set_custom_data_path(borealis::io::fs_path_from_utf8(path), errorOut);
}

bool set_portable_data_path() {
    return operation_succeeded(manager().set_portable_data_path());
}

bool reset_data_path() {
    return operation_succeeded(manager().reset_data_path());
}

bool is_default_data_path() {
    return manager().is_default_data_path();
}

bool is_data_path_restart_pending() {
    return manager().is_data_path_restart_pending();
}

std::filesystem::path user_home_path() {
    return manager().user_home_path();
}

std::filesystem::path normalized_display_path(const std::filesystem::path& path) {
    return manager().normalized_display_path(path);
}

std::string abbreviated_path_string(const std::filesystem::path& path) {
    return manager().abbreviated_path_string(path);
}

}  // namespace dusk::data
