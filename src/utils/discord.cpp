#include <iostream>
#include <algorithm>
#include "../core/globals.h"
#include "crashlog.h"
#include "discord_rpc.h"
#include "helpers.h"

static const char* DEFAULT_STREMIO_ICON = "https://raw.githubusercontent.com/Stremio/stremio-web/refs/heads/development/assets/images/icon.png";

void Discord_Ready(const DiscordUser* user) {
    std::cout << "[DISCORD]: Connected to Discord user: " + std::string(user->username) << "\n";
}

void Discord_Disconnected(int errorCode, const char* message) {
    std::cout << "[DISCORD]: Disconnected (" + std::to_string(errorCode) + "): " + std::string(message) << "\n";
}

void Discord_Error(int errorCode, const char* message) {
    std::cout << "[DISCORD]: Error (" + std::to_string(errorCode) + "): " + std::string(message) << "\n";
    AppendToCrashLog("[DISCORD]: Error (" + std::to_string(errorCode) + "): " + std::string(message));
}

void InitializeDiscord()
{
    DiscordEventHandlers handlers{};
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = Discord_Ready;
    handlers.disconnected = Discord_Disconnected;
    handlers.errored = Discord_Error;

    Discord_Initialize("1361448446862692492", &handlers, 1, nullptr);
}

static std::string TruncateStr(const std::string& str, size_t maxLen) {
    if (str.length() <= maxLen) return str;
    return str.substr(0, maxLen);
}

// Discord has a strict limit (<= 300 chars) on image keys/URLs.
// Addon poster/thumbnail URLs can include long tracking parameters or fallbacks exceeding 300 chars.
static std::string SanitizeImageUrl(const std::string& rawUrl, bool fallbackToDefault = true) {
    if (rawUrl.empty()) {
        return fallbackToDefault ? DEFAULT_STREMIO_ICON : "";
    }

    // Must be HTTP or HTTPS
    if (rawUrl.rfind("http://", 0) != 0 && rawUrl.rfind("https://", 0) != 0) {
        return fallbackToDefault ? DEFAULT_STREMIO_ICON : "";
    }

    // If within safe length (Discord limit is <= 300, keep <= 256), use as-is
    if (rawUrl.length() <= 256) {
        return rawUrl;
    }

    // Attempt 1: Check for fallback= parameter (often used in RPDB / proxy poster URLs)
    std::string lowerUrl = rawUrl;
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(), [](unsigned char c) { return (char)::tolower(c); });
    size_t fallbackPos = lowerUrl.find("fallback=");
    if (fallbackPos != std::string::npos) {
        size_t start = fallbackPos + 9;
        size_t end = rawUrl.find('&', start);
        std::string fallbackEnc = (end == std::string::npos) ? rawUrl.substr(start) : rawUrl.substr(start, end - start);
        std::string decoded = decodeURIComponent(fallbackEnc);
        if ((decoded.rfind("http://", 0) == 0 || decoded.rfind("https://", 0) == 0) && decoded.length() <= 256) {
            return decoded;
        }
    }

    // Attempt 2: Strip query parameters if the base URL ends with an image extension
    size_t queryPos = rawUrl.find('?');
    if (queryPos != std::string::npos) {
        std::string base = rawUrl.substr(0, queryPos);
        if (base.length() <= 256 && (base.rfind("http://", 0) == 0 || base.rfind("https://", 0) == 0)) {
            std::string lowerBase = base;
            std::transform(lowerBase.begin(), lowerBase.end(), lowerBase.begin(), [](unsigned char c) { return (char)::tolower(c); });
            if (lowerBase.ends_with(".jpg") || lowerBase.ends_with(".jpeg") ||
                lowerBase.ends_with(".png") || lowerBase.ends_with(".webp")) {
                return base;
            }
        }
    }

    // Fallback if still too long or invalid
    return fallbackToDefault ? DEFAULT_STREMIO_ICON : "";
}

// Encapsulated presence setters

static void SetDiscordWatchingPresence(
        const std::vector<std::string>& args
) {
    // expects:
    // 0: generic "watching" identifier
    // 1: type (movie, series)
    // 2: title
    // 3: season
    // 4: episode
    // 5: episode name
    // 6: episode thumbnail (small image) (Optional)
    // 7: show/movie image (large image)
    // 8: elapsed seconds
    // 9: duration seconds
    // 10: isPaused ("yes" or "no") (Optional)
    // 11: more detail button link (imdb link) (Optional)
    // 12: watch on stremio button link (stremio link) (Optional)
    if (args.size() < 3) {
        return;
    }

    DiscordRichPresence discordPresence{};
    memset(&discordPresence, 0, sizeof(discordPresence));

    discordPresence.type = DISCORD_ACTIVITY_TYPE_WATCHING;

    // Common fields
    std::string title = (args.size() > 2) ? args[2] : "";
    std::string detailsStr = TruncateStr(title, 128);
    discordPresence.details = detailsStr.empty() ? nullptr : detailsStr.c_str();

    std::string largeImg = SanitizeImageUrl(args.size() > 7 ? args[7] : "", true);
    discordPresence.largeImageKey = largeImg.c_str();

    std::string largeImgText = TruncateStr(title, 128);
    discordPresence.largeImageText = largeImgText.empty() ? "Stremio" : largeImgText.c_str();

    // Handle paused state (optional)
    bool isPaused = (args.size() > 10 && !args[10].empty() && args[10] == "yes");

    std::string stateStr;
    std::string smallImg;
    std::string smallImgText;

    if (isPaused) {
        stateStr = "Paused";
        discordPresence.state = stateStr.c_str();
        discordPresence.startTimestamp = 0;
        discordPresence.endTimestamp = 0;
    } else {
        std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        int elapsedSeconds = 0;
        int durationSeconds = 0;
        try {
            if (args.size() > 8 && !args[8].empty()) elapsedSeconds = (std::max)(0, std::stoi(args[8]));
            if (args.size() > 9 && !args[9].empty()) durationSeconds = (std::max)(0, std::stoi(args[9]));
        } catch (...) {
            elapsedSeconds = 0;
            durationSeconds = 0;
        }

        discordPresence.startTimestamp = currentTime - elapsedSeconds;
        if (durationSeconds > elapsedSeconds) {
            discordPresence.endTimestamp = currentTime + (durationSeconds - elapsedSeconds);
        } else {
            discordPresence.endTimestamp = 0;
        }

        if (args.size() > 1 && args[1] == "series") {
            // Series-specific fields
            std::string epName = (args.size() > 5) ? args[5] : "";
            std::string season = (args.size() > 3) ? args[3] : "";
            std::string episode = (args.size() > 4) ? args[4] : "";

            if (!epName.empty() && !season.empty() && !episode.empty()) {
                stateStr = epName + " (S" + season + "-E" + episode + ")";
            } else if (!season.empty() && !episode.empty()) {
                stateStr = "Season " + season + ", Episode " + episode;
            } else if (!epName.empty()) {
                stateStr = epName;
            } else {
                stateStr = "Watching a Series";
            }
            stateStr = TruncateStr(stateStr, 128);
            discordPresence.state = stateStr.c_str();

            if (args.size() > 6 && !args[6].empty()) {
                smallImg = SanitizeImageUrl(args[6], false);
                if (!smallImg.empty()) {
                    discordPresence.smallImageKey = smallImg.c_str();
                    smallImgText = TruncateStr(epName.empty() ? detailsStr : epName, 128);
                    discordPresence.smallImageText = smallImgText.c_str();
                }
            }
        } else {
            stateStr = "Enjoying a Movie";
            discordPresence.state = stateStr.c_str();
        }
    }

    // Buttons setup (optional)
    std::string button1UrlStr;
    std::string button2UrlStr;

    if (args.size() > 11 && !args[11].empty()) {
        std::string u = args[11];
        if (u.rfind("http://", 0) == 0 || u.rfind("https://", 0) == 0) {
            button1UrlStr = TruncateStr(u, 512);
            discordPresence.button1Label = "More Details";
            discordPresence.button1Url = button1UrlStr.c_str();
        }
    }

    if (args.size() > 12 && !args[12].empty()) {
        std::string u = args[12];
        if (u.rfind("stremio://", 0) == 0) {
            std::string path = u.substr(10);
            while (!path.empty() && path.front() == '/') path.erase(path.begin());
            u = "https://web.stremio.com/#/" + path;
        }
        if (u.rfind("http://", 0) == 0 || u.rfind("https://", 0) == 0) {
            button2UrlStr = TruncateStr(u, 512);
            discordPresence.button2Label = "Watch on Stremio";
            discordPresence.button2Url = button2UrlStr.c_str();
        }
    }

    Discord_UpdatePresence(&discordPresence);
}

static void SetDiscordMetaDetailPresence(const std::vector<std::string>& args) {
    // args structure:
    // 0: embed type ("meta-detail")
    // 1: type ("movie" or "series")
    // 2: title
    // 3: image URL
    if (args.size() < 3) return;

    DiscordRichPresence discordPresence{};
    memset(&discordPresence, 0, sizeof(discordPresence));

    discordPresence.type = DISCORD_ACTIVITY_TYPE_WATCHING;

    std::string title = (args.size() > 2) ? args[2] : "";
    std::string detailsStr = TruncateStr(title, 128);
    discordPresence.details = detailsStr.empty() ? nullptr : detailsStr.c_str();

    std::string largeImg = SanitizeImageUrl(args.size() > 3 ? args[3] : "", true);
    discordPresence.largeImageKey = largeImg.c_str();

    std::string largeImgText = TruncateStr(title, 128);
    discordPresence.largeImageText = largeImgText.empty() ? "Stremio" : largeImgText.c_str();

    // Engaging state
    std::string stateStr;
    if (args.size() > 1 && args[1] == "movie") {
        stateStr = "Exploring a Movie";
    } else if (args.size() > 1 && args[1] == "series") {
        stateStr = "Exploring a Series";
    } else {
        stateStr = "Exploring Details";
    }
    discordPresence.state = stateStr.c_str();

    Discord_UpdatePresence(&discordPresence);
}

static void SetDiscordDiscoverPresence(const char *const details, const char *const state) {
    std::cout << "[DISCORD]: Setting discover Presence\n";

    DiscordRichPresence discordPresence{};
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.type = DISCORD_ACTIVITY_TYPE_WATCHING;
    discordPresence.state = state;
    discordPresence.details = details;
    discordPresence.largeImageKey = DEFAULT_STREMIO_ICON;
    discordPresence.largeImageText = "Stremio";
    Discord_UpdatePresence(&discordPresence);
}

void SetDiscordPresenceFromArgs(const std::vector<std::string>& args) {
    if (!g_isRpcOn || args.empty()) {
        return;
    }

    const std::string& embedType = args[0];
    if (embedType == "watching" && args.size() >= 3) {
        SetDiscordWatchingPresence(args);
    } else if (embedType == "meta-detail" && args.size() >= 3) {
        SetDiscordMetaDetailPresence(args);
    } else if (embedType == "board") {
        SetDiscordDiscoverPresence("Resuming Favorites", "On Board");
    } else if (embedType == "discover") {
        SetDiscordDiscoverPresence("Finding New Gems", "In Discover");
    } else if (embedType == "library") {
        SetDiscordDiscoverPresence("Revisiting Old Favorites", "In Library");
    } else if (embedType == "calendar") {
        SetDiscordDiscoverPresence("Planning My Next Binge", "On Calendar");
    } else if (embedType == "addons") {
        SetDiscordDiscoverPresence("Exploring Add-ons", "In Add-ons");
    } else if (embedType == "settings") {
        SetDiscordDiscoverPresence("Tuning Preferences", "In Settings");
    } else if (embedType == "search") {
        SetDiscordDiscoverPresence("Searching for Shows & Movies", "In Search");
    } else if (embedType == "clear") {
        Discord_ClearPresence();
    }
    // Add more presence types here...
}
