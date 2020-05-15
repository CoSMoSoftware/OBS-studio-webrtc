/* Copyright Dr. Alex. Gouaillard (2015, 2020) */

#pragma once

// clang-format off
#include <regex>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>

class SDPModif {
public:
    // Enable stereo. Set audio bitrate (if nonzero)
    static void stereoSDP(std::string &sdp, int audioBitrate)
    {
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int aLine = findLines(sdpLines, "m=audio ");
        int vLine = findLines(sdpLines, "m=video ");
        int audio_start = aLine > 0 ? aLine : 0;
        int audio_end = vLine > audio_start ? vLine : sdpLines.size();
        int testLine = findLines(sdpLines, "stereo=1;sprop-stereo=1");
        // audio section contains at least 1 stereo codec
        if (testLine >= audio_start && testLine <= audio_end)
            return;
        std::string opusRe = "a=rtpmap:([0-9]{1,3}) [oO][pP][uU][sS]";
        int rtpmap = findLinesRegEx(sdpLines, opusRe);
        if (rtpmap == -1)
            return;
        std::string aBitrate = std::to_string(audioBitrate);
        std::string maxAvgBitrate = std::to_string(audioBitrate * 1024);
        std::smatch match;
        std::regex re(opusRe);
        if (!std::regex_search(sdpLines[rtpmap], match, re))
            return;
        std::string payloadNumber = match[1].str();
        int fmtp = findLines(sdpLines, "a=fmtp:" + payloadNumber);
        if (fmtp != -1) {
            sdpLines[fmtp] =
                sdpLines[fmtp].append(";stereo=1;sprop-stereo=1")
                              .append(";maxplaybackrate=48000")
                              .append(";sprop-maxcapturerate=48000");
            if (audioBitrate > 0) {
                sdpLines[fmtp] =
                    sdpLines[fmtp].append(";maxaveragebitrate=" + maxAvgBitrate)
                                  .append(";x-google-min-bitrate=" + aBitrate)
                                  .append(";x-google-max-bitrate=" + aBitrate);
            }
        }
        else {
            std::string fmtpLine = "a=fmtp:" + payloadNumber;
            sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
            sdpLines[rtpmap+1] =
                sdpLines[rtpmap+1].append(" minptime=10;useinbandfec=1")
                                  .append(";stereo=1;sprop-stereo=1")
                                  .append(";maxplaybackrate=48000")
                                  .append(";sprop-maxcapturerate=48000");
            if (audioBitrate > 0) {
                sdpLines[rtpmap+1] =
                    sdpLines[rtpmap+1].append(";maxaveragebitrate=" + maxAvgBitrate)
                                      .append(";x-google-min-bitrate=" + aBitrate)
                                      .append(";x-google-max-bitrate=" + aBitrate);
            }
        }
        sdp = join(sdpLines, "\r\n");
    }

    // Set video bitrate constraint (b=AS)
    static void bitrateSDP(std::string &sdp, int newBitrate)
    {
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int aLine = findLines(sdpLines, "m=audio ");
        int vLine = findLines(sdpLines, "m=video ");
        int video_start = vLine > 0 ? vLine : 0;
        int video_end = aLine > video_start ? aLine : sdpLines.size();
        int testLine = findLines(sdpLines, "b=AS:");
        if (testLine >= video_start && testLine <= video_end)
            return;
        std::ostringstream newLine;
        newLine << "b=AS:" << newBitrate;
        if (strncmp(sdpLines[vLine+1].c_str(), "c=", 2) == 0)
            sdpLines.insert(sdpLines.begin() + vLine+2, newLine.str());
        else
            sdpLines.insert(sdpLines.begin() + vLine+1, newLine.str());
        sdp = join(sdpLines, "\r\n");
    }

    // Set video bitrate constraint (b=AS, x-google-min, x-google-max)
    static void bitrateMaxMinSDP(std::string &sdp,
                                 const int newBitrate,
                                 const std::vector<int> &video_payload_numbers)
    {
        std::string kbps = std::to_string(newBitrate);
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int aLine = findLines(sdpLines, "m=audio ");
        int vLine = findLines(sdpLines, "m=video ");
        int video_start = vLine > 0 ? vLine : 0;
        int video_end = aLine > video_start ? aLine : sdpLines.size();
        int testLine = findLines(sdpLines, "b=AS:");

        // video section does not have b=AS. insert line with b=AS constraint
        if (testLine < video_start || testLine > video_end) {
            int vLine = findLines(sdpLines, "m=video ");
            std::ostringstream newLine;
            newLine << "b=AS:" << newBitrate;
            if (strncmp(sdpLines[vLine+1].c_str(), "c=", 2) == 0)
                sdpLines.insert(sdpLines.begin() + vLine+2, newLine.str());
            else
                sdpLines.insert(sdpLines.begin() + vLine+1, newLine.str());
        }
        // video section contains b=AS. replace line with new b=AS constraint
        else {
            std::ostringstream newLine;
            newLine << "b=AS:" << newBitrate;
            sdpLines.insert(sdpLines.begin() + testLine+1, newLine.str());
            sdpLines.erase(sdpLines.begin() + testLine);
        }

        testLine = findLines(sdpLines, "x-google-min-bitrate=" + kbps);

        // video section already has an fmtp line with x-google-min-bitrate constraint
        // replace |testLine| with new x-google-min/max-constraint
        if (testLine >= video_start && testLine <= video_end) {
            std::smatch match;
            std::ostringstream newLine;
            std::string payloadRe = "(a=fmtp:[0-9]+ [^ ]+)(?:x-google-min-bitrate=[0-9]+)(?:;x-google-max-bitrate=[0-9]+)?([^ ]*)";
            std::regex re(payloadRe);
            if (std::regex_search(sdpLines[testLine], match, re)) {
                std::string pre = match[1].str();
                std::string post = match[2].str();
                newLine << pre << "x-google-min-bitrate=" << kbps;
                newLine << ";x-google-max-bitrate=" << kbps << post;
                sdpLines.insert(sdpLines.begin() + testLine+1, newLine.str());
                sdpLines.erase(sdpLines.begin() + testLine);
            }
            sdp = join(sdpLines, "\r\n");
            return;
        }

        for (const auto &num : video_payload_numbers) {
            int fmtp = findLines(sdpLines, "a=fmtp:" + std::to_string(num));
            int rtpmap = findLines(sdpLines, "a=rtpmap:" + std::to_string(num));
            // fmtp line exists, append x-google-*-bitrate constraint
            if (fmtp != -1) {
                std::string x = "x-google-min-bitrate=";
                // fmtp line has x-google-min-bitrate constraint
                // replace line with new x-google-min/max-constraint
                if (sdpLines[fmtp].find(x) != std::string::npos) {
                    std::smatch match;
                    std::ostringstream newLine;
                    std::string payloadRe =
                        "(a=fmtp:[0-9]+ [^ ]+)(?:x-google-min-bitrate=[0-9]+)";
                    payloadRe += "(?:;x-google-max-bitrate=[0-9]+)?([^ ]*)";
                    std::regex re(payloadRe);
                    if (std::regex_search(sdpLines[testLine], match, re)) {
                        std::string pre = match[1].str();
                        std::string post = match[2].str();
                        newLine << pre << "x-google-min-bitrate=" << kbps;
                        newLine << ";x-google-max-bitrate=" << kbps << post;
                        sdpLines.insert(sdpLines.begin() + testLine+1, newLine.str());
                        sdpLines.erase(sdpLines.begin() + testLine);
                    }
                }
                // fmtp line does not have x-google-*-bitrate constraint
                // append x-google-*-bitrate constraints to fmtp line
                else {
                    sdpLines[fmtp] =
                        sdpLines[fmtp].append(";x-google-min-bitrate=" + kbps)
                                      .append(";x-google-max-bitrate=" + kbps);
                }
            }
            // insert fmtp line below rtmap line
            else if (rtpmap != -1) {
                std::string fmtpLine = "a= fmtp:" + std::to_string(num);
                sdpLines.insert(sdpLines.begin() + rtpmap + 1, fmtpLine);
                sdpLines[rtpmap+1] =
                    sdpLines[rtpmap+1].append(" x-google-min-bitrate=" + kbps)
                                      .append(";x-google-max-bitrate=" + kbps);
            }
        }
        sdp = join(sdpLines, "\r\n");
    }

    // Only accept ice candidates matching protocol (UDP, TCP)
    static bool filterIceCandidates(const std::string &candidate, const std::string &protocol)
    {
        std::smatch match;
        std::regex re("candidate:([0-9]+) ([0-9]+) ([tTuU][cCdD][pP])");
        if (std::regex_search(candidate, match, re))
            if (caseInsensitiveStringCompare(match[3].str(), protocol))
                return true;
        return false;
    }

    // Remove all payloads from SDP except Opus and video_codec
    static void forcePayload(std::string &sdp,
                             std::vector<int> &audio_payload_numbers,
                             std::vector<int> &video_payload_numbers,
                             const std::string &video_codec)
    {
        return forcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, 0, "42e01f", 0);
    }

    // Remove all payloads from SDP except Opus and video_codec
    static void forcePayload(std::string &sdp,
                             std::vector<int> &audio_payload_numbers,
                             std::vector<int> &video_payload_numbers,
                             const std::string &video_codec,
                             const int vp9_profile_id)
    {
        return forcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, 0, "42e01f", vp9_profile_id);
    }

    // Remove all payloads from SDP except Opus and video_codec
    static void forcePayload(std::string &sdp,
                             std::vector<int> &audio_payload_numbers,
                             std::vector<int> &video_payload_numbers,
                             const std::string &video_codec,
                             const int h264_packetization_mode,
                             const std::string &h264_profile_level_id)
    {
        return forcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, h264_packetization_mode,
                            h264_profile_level_id, 0);
    }

    // Remove all payloads from SDP except Opus and video_codec
    static void forcePayload(std::string &sdp,
                             std::vector<int> &audio_payload_numbers,
                             std::vector<int> &video_payload_numbers,
                             const std::string &video_codec,
                             const int h264_packetization_mode,
                             const std::string &h264_profile_level_id,
                             const int vp9_profile_id)
    {
        return forcePayload(sdp, audio_payload_numbers, video_payload_numbers,
                            "opus", video_codec, h264_packetization_mode,
                            h264_profile_level_id, vp9_profile_id);
    }

    // Remove all payloads from SDP except |video_codec| & |audio_codec|
    static void forcePayload(std::string &sdp,
                             std::vector<int> &audio_payload_numbers,
                             std::vector<int> &video_payload_numbers,
                             const std::string &audio_codec,
                             const std::string &video_codec,
                             const int h264_packetization_mode,
                             const std::string &h264_profile_level_id,
                             const int vp9_profile_id)
    {
        int line;
        std::ostringstream newLineA;
        std::ostringstream newLineV;
        std::string audio_payloads = "";
        std::string video_payloads = "";
        std::vector<std::string> sdpLines;
        // Retained payloads stored in |audio_payload_numbers|
        filterPayloads(sdp, audio_payloads, audio_payload_numbers, "audio", audio_codec,
                       h264_packetization_mode, h264_profile_level_id, vp9_profile_id);
        // Retained payloads stored in |video_payload_numbers|
        filterPayloads(sdp, video_payloads, video_payload_numbers, "video", video_codec,
                       h264_packetization_mode, h264_profile_level_id, vp9_profile_id);
        split(sdp, (char *)"\r\n", sdpLines);
        // Replace audio m-line
        line = findLines(sdpLines, "m=audio");
        if (line != -1) {
            newLineA << "m=audio 9 UDP/TLS/RTP/SAVPF" << audio_payloads;
            sdpLines.insert(sdpLines.begin() + line+1, newLineA.str());
            sdpLines.erase(sdpLines.begin() + line);
        }
        // Replace video m-line
        line = findLines(sdpLines, "m=video");
        if (line != -1) {
            newLineV << "m=video 9 UDP/TLS/RTP/SAVPF" << video_payloads;
            sdpLines.insert(sdpLines.begin() + line+1, newLineV.str());
            sdpLines.erase(sdpLines.begin() + line);
        }
        sdp = join(sdpLines, "\r\n");
    }

private:
    // Remove all payloads from SDP except Opus and codec
    static void filterPayloads(std::string &sdp,
                               std::string &payloads,
                               std::vector<int> &payload_numbers,
                               const std::string &media_type,
                               const std::string &codec)
    {
        return filterPayloads(sdp, payloads, payload_numbers, media_type, codec,
                              0, "42e01f", 0);
    }

    // Remove all payloads of media_type from SDP except media_codec
    static void filterPayloads(std::string &sdp,
                               std::string &payloads,
                               std::vector<int> &payload_numbers,
                               const std::string &media_type,
                               const std::string &media_codec,
                               const int h264_packetization_mode,
                               const std::string &h264_profile_level_id,
                               const int vp9_profile_id)
    {
        std::vector<int> apt_payload_numbers;
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int aLine = findLines(sdpLines, "m=audio ");
        int vLine = findLines(sdpLines, "m=video ");
        int for_start = 0;
        int for_end = sdpLines.size();
        if (media_type == "audio") {
            for_start = aLine > 0 ? aLine : 0;
            for_end = vLine > for_start ? vLine : sdpLines.size();
        }
        else if (media_type == "video") {
            for_start = vLine > 0 ? vLine : 0;
            for_end = aLine > for_start ? aLine : sdpLines.size();
        }
        for (int i = for_start; i < for_end; i++) {
            std::smatch match;
            std::string payloadRe = "a=rtpmap:([0-9]+) ([a-zA-Z0-9-]+)";
            std::regex re(payloadRe);
            if (std::regex_search(sdpLines[i], match, re)) {
                std::string payloadNumber = match[1].str();
                std::string payloadCodec = match[2].str();
                bool found = caseInsensitiveStringCompare(media_codec, payloadCodec);
                bool all = media_codec.empty();
                getMatchingPayloads(sdp, sdpLines, payloads, payload_numbers,
                                    apt_payload_numbers, payloadCodec,
                                    payloadNumber, all, found,
                                    h264_packetization_mode,
                                    h264_profile_level_id, vp9_profile_id);
            }
        }
    }

    // Store matching payloads in |payloads| & |payload_numbers|
    static void getMatchingPayloads(std::string &sdp,
                                    std::vector<std::string> &sdpLines,
                                    std::string &payloads,
                                    std::vector<int> &payload_numbers,
                                    std::vector<int> &apt_payload_numbers,
                                    const std::string &payloadCodec,
                                    const std::string &payloadNumber,
                                    const bool all,
                                    const bool found,
                                    const int h264_packetization_mode,
                                    const std::string &h264_profile_level_id,
                                    const int vp9_profile_id)
    {
        bool keep = false;
        bool aptKeep = false;
        std::string h264fmtp =
            " level-asymmetry-allowed=[0-1];packetization-mode=([0-9]);profile-level-id=([0-9a-f]{6})";
        std::string vp9fmtp = " profile-id=([0-9])";
        int h264fmtpLine = findLinesRegEx(sdpLines, payloadNumber + h264fmtp);
        int vp9fmtpLine  = findLinesRegEx(sdpLines, payloadNumber + vp9fmtp);
        if (found) {
            if (caseInsensitiveStringCompare("h264", payloadCodec)
                    && h264fmtpLine != -1) {
                std::smatch match;
                std::regex re(payloadNumber + h264fmtp);
                if (std::regex_search(sdpLines[h264fmtpLine], match, re)) {
                    int pkt_mode = std::stoi(match[1].str());
                    std::string p_level_id = match[2].str();
                    if (caseInsensitiveStringCompare(h264_profile_level_id, p_level_id)
                            && h264_packetization_mode == pkt_mode) {
                        keep = true;
                    }
                }
            }
            else if (caseInsensitiveStringCompare("vp9", payloadCodec)
                    && vp9fmtpLine != -1) {
                std::smatch match;
                std::regex re(payloadNumber + vp9fmtp);
                if (std::regex_search(sdpLines[vp9fmtpLine], match, re)) {
                    int profile_id = std::stoi(match[1].str());
                    if (vp9_profile_id == profile_id) {
                        keep = true;
                    }
                }
            }
            else {
                keep = true;
            }
        }
        else if (all) {
            keep = true;
        }
        std::string rtxPayloadNumber = "";
        int aptLine = findLines(sdpLines, "apt=" + payloadNumber);
        if (aptLine != -1) {
            std::smatch matchApt;
            std::regex reApt("a=fmtp:([0-9]+) apt");
            if (std::regex_search(sdpLines[aptLine], matchApt, reApt)) {
                rtxPayloadNumber = matchApt[1].str();
                if (keep) {
                    aptKeep = true;
                }
            }
        }
        if (keep) {
            payloads += " " + payloadNumber;
            payload_numbers.push_back(std::stoi(payloadNumber));
        }
        if (aptKeep) {
            if (!all) {
                payloads += " " + rtxPayloadNumber;
            }
            apt_payload_numbers.push_back(std::stoi(rtxPayloadNumber));
        }
        if (!keep && !aptKeep) {
            const auto begin = payload_numbers.begin();
            const auto end = payload_numbers.end();
            const auto apt_begin = apt_payload_numbers.begin();
            const auto apt_end = apt_payload_numbers.end();
            const auto payload = std::stoi(payloadNumber);
            if (std::find(apt_begin, apt_end, payload) == apt_end
                    && std::find(begin, end, payload) == end) {
                deletePayload(sdp, payload);
            }
        }
    }

    // Delete payload from SDP (string)
    static void deletePayload(std::string &sdp, const int payloadNumber)
    {
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int line;
        do {
            line = findLines(sdpLines, ":" + std::to_string(payloadNumber) + " ");
            if (line != -1)
                sdpLines.erase(sdpLines.begin() + line);
        } while (line != -1);
        sdp = join(sdpLines, "\r\n");
    }

    // Delete payload from SDP (vector<string>)
    static void deletePayload(std::vector<std::string> &sdpLines, const int payloadNumber)
    {
        int line;
        do {
            line = findLines(sdpLines, ":" + std::to_string(payloadNumber) + " ");
            if (line != -1)
                sdpLines.erase(sdpLines.begin() + line);
        } while (line != -1);
    }

    static bool caseInsensitiveStringCompare(const char *str1, const char *str2)
    {
        return caseInsensitiveStringCompare(std::string(str1), std::string(str2));
    }

    static bool caseInsensitiveStringCompare(const std::string &s1, const std::string &s2)
    {
        std::string s1Cpy = s1;
        std::string s2Cpy = s2;
        std::transform(s1Cpy.begin(), s1Cpy.end(), s1Cpy.begin(), ::tolower);
        std::transform(s2Cpy.begin(), s2Cpy.end(), s2Cpy.begin(), ::tolower);
        return (s1Cpy == s2Cpy);
    }

    static int findLines(const std::vector<std::string> &sdpLines, std::string prefix)
    {
        for (unsigned long i = 0; i < sdpLines.size(); i++) {
            if (sdpLines[i].find(prefix) != std::string::npos)
                return i;
        }
        return -1;
    }

    static int findLinesRegEx(const std::vector<std::string> &sdpLines, std::string prefix)
    {
        std::regex re(prefix);
        for (unsigned long i = 0; i < sdpLines.size(); i++) {
            std::smatch match;
            if (std::regex_search(sdpLines[i], match, re))
                return i;
        }
        return -1;
    }

    static std::string join(std::vector<std::string> &v, std::string delim)
    {
        std::ostringstream s;
        for (const auto &i : v) {
            if (&i != &v[0])
                s << delim;
            s << i;
        }
        s << delim;
        return s.str();
    }

    static void split(const std::string &s, char *delim, std::vector<std::string> &v)
    {
        char *dup = strdup(s.c_str());
        char *token = strtok(dup, delim);
        while (token != NULL) {
            v.push_back(std::string(token));
            token = strtok(NULL, delim);
        }
        free(dup);
    }
};

