#pragma once

#include "launcher_translations.h"
#include "launcher_translation_urls.h"

namespace launcher_translations {

struct VoicePackage {
    const char* id;
    const char* filename;
    bool rp;
    const char* url;
    const char* hash;
    const char* size;
};
inline constexpr VoicePackage voices[] = {
    { "vanilla-1c", "f2r-voice-vanilla-1c.dat", false, voiceVANILLA_1CURL, voiceVANILLA_1CSHA256, voiceVANILLA_1CSIZE },
    { "rp-1c", "f2r-voice-rp-1c.dat", true, voiceRP_1CURL, voiceRP_1CSHA256, voiceRP_1CSIZE },
    { "rp-levy", "f2r-voice-rp-levy.dat", true, voiceRP_LEVYURL, voiceRP_LEVYSHA256, voiceRP_LEVYSIZE },
    { "vanilla-fargus", "f2r-voice-vanilla-fargus.dat", false, voiceVANILLA_FARGUSURL, voiceVANILLA_FARGUSSHA256, voiceVANILLA_FARGUSSIZE },
    { "vanilla-levy", "f2r-voice-vanilla-levy.dat", false, voiceVANILLA_LEVYURL, voiceVANILLA_LEVYSHA256, voiceVANILLA_LEVYSIZE },
};
inline const VoicePackage* findVoice(const std::string& id)
{
    for (const auto& voice : voices) if (id == voice.id) return &voice;
    return nullptr;
}
inline bool voiceAvailable(const std::string& id, bool rp)
{
    if (id == "original") return true;
    const auto* voice = findVoice(id);
    if (!voice || voice->rp != rp || !validUrl(voice->url) || std::string(voice->hash).size() != 64) return false;
    for (char c : std::string(voice->hash)) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    const std::string size = voice->size;
    if (size.empty() || size.size() > 18 || size.find_first_not_of("0123456789") != std::string::npos) return false;
    return std::stoull(size) > 0;
}

}
