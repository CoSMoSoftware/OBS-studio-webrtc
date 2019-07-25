#ifndef _SPDMODIF_H_
#define _SPDMODIF_H_

#include <initializer_list>
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
        int audioLine = findLines(sdpLines, "m=audio ");
        int videoLine = findLines(sdpLines, "m=video ");
        int audio_start = audioLine > 0 ? audioLine : 0;
        int audio_end = videoLine > audioLine ? videoLine : sdpLines.size();
        int testLine = findLines(sdpLines, "stereo=1;sprop-stereo=1");
        if (testLine < audio_start || testLine > audio_end) {
            std::string opusPayloadRe = "a=rtpmap:([0-9]{1,3}) [oO][pP][uU][sS]";
            int opusRtpmapLine = findLinesRegEx(sdpLines, opusPayloadRe);
            if (opusRtpmapLine != -1) {
                std::string aBitrate = std::to_string(audioBitrate);
                std::string maxAvgBitrate = std::to_string(audioBitrate * 1024);
                std::string payloadNumber = "111";
                std::smatch match;
                std::regex re(opusPayloadRe);
                if (std::regex_search(sdpLines[opusRtpmapLine], match, re))
                    payloadNumber = match[1].str();
                int fmtpLine = findLines(sdpLines, "a=fmtp:" + payloadNumber);
                if (fmtpLine != -1) {
                    sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";stereo=1;sprop-stereo=1");
                    sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";maxplaybackrate=48000;sprop-maxcapturerate=48000");
                    if (audioBitrate > 0) {
                        sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";maxaveragebitrate=" + maxAvgBitrate);
                        sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-min-bitrate=" + aBitrate);
                        sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-max-bitrate=" + aBitrate);
                    }
                } else {
                    std::string newLineFmtp = "a=fmtp:" + payloadNumber;
                    sdpLines.insert(sdpLines.begin() + opusRtpmapLine+1, newLineFmtp);
                    sdpLines[opusRtpmapLine+1] = sdpLines[opusRtpmapLine+1].append(" minptime=10;useinbandfec=1");
                    sdpLines[opusRtpmapLine+1] = sdpLines[opusRtpmapLine+1].append(";stereo=1;sprop-stereo=1");
                    sdpLines[opusRtpmapLine+1] = sdpLines[opusRtpmapLine+1].append(";maxplaybackrate=48000;sprop-maxcapturerate=48000");
                    if (audioBitrate > 0) {
                        sdpLines[opusRtpmapLine+1] = sdpLines[opusRtpmapLine+1].append(";maxaveragebitrate=" + maxAvgBitrate);
                        sdpLines[opusRtpmapLine+1] = sdpLines[opusRtpmapLine+1].append(";x-google-min-bitrate=" + aBitrate);
                        sdpLines[opusRtpmapLine+1] = sdpLines[opusRtpmapLine+1].append(";x-google-max-bitrate=" + aBitrate);
                    }
                }
            }
            sdp = join(sdpLines, "\r\n");
        }
    }

    // Set video bitrate constraint (b=AS)
    static void bitrateSDP(std::string &sdp, int newBitrate)
    {
        std::string vBitrate = std::to_string(newBitrate);
        std::ostringstream newLineBitrate;
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int audioLine = findLines(sdpLines, "m=audio ");
        int videoLine = findLines(sdpLines, "m=video ");
        int video_start = videoLine > 0 ? videoLine : 0;
        int video_end = audioLine > videoLine ? audioLine : sdpLines.size();
        int testLine = findLines(sdpLines, "b=AS:");
        if (testLine < video_start || testLine > video_end) {
            int videoLine = findLines(sdpLines, "m=video ");
            newLineBitrate << "b=AS:" << newBitrate;
            if (strncmp(sdpLines[videoLine+1].c_str(), "c=", 2) == 0)
                sdpLines.insert(sdpLines.begin() + videoLine+2, newLineBitrate.str());
            else
                sdpLines.insert(sdpLines.begin() + videoLine+1, newLineBitrate.str());
        }
        sdp = join(sdpLines, "\r\n");
    }

    // Set video bitrate constraint (b=AS, x-google-min, x-google-max)
    static void bitrateMaxMinSDP(std::string &sdp,
                                 const int newBitrate,
                                 const std::vector<int> &video_payload_numbers)
    {
        std::string vBitrate = std::to_string(newBitrate);
        std::ostringstream newLineBitrate;
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int audioLine = findLines(sdpLines, "m=audio ");
        int videoLine = findLines(sdpLines, "m=video ");
        int video_start = videoLine > 0 ? videoLine : 0;
        int video_end = audioLine > videoLine ? audioLine : sdpLines.size();
        int testLine = findLines(sdpLines, "b=AS:");
        if (testLine < video_start || testLine > video_end) {
            int videoLine = findLines(sdpLines, "m=video ");
            newLineBitrate << "b=AS:" << newBitrate;
            if (strncmp(sdpLines[videoLine+1].c_str(), "c=", 2) == 0)
                sdpLines.insert(sdpLines.begin() + videoLine+2, newLineBitrate.str());
            else
                sdpLines.insert(sdpLines.begin() + videoLine+1, newLineBitrate.str());
        }
        testLine = findLines(sdpLines, "x-google-min-bitrate=" + vBitrate);
        if (testLine < video_start || testLine > video_end) {
            for (const auto &payload : video_payload_numbers) {
                int fmtpLine = findLines(sdpLines, "a=fmtp:" + std::to_string(payload));
                if (fmtpLine != -1) {
                    sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-min-bitrate=" + vBitrate);
                    sdpLines[fmtpLine] = sdpLines[fmtpLine].append(";x-google-max-bitrate=" + vBitrate);
                } else {
                    int rtpmapLine = findLines(sdpLines, "a=rtpmap:" + std::to_string(payload));
                    if (rtpmapLine != -1) {
                        std::string newLineFmtp = "a=fmtp:" + std::to_string(payload);
                        sdpLines.insert(sdpLines.begin() + rtpmapLine+1, newLineFmtp);
                        sdpLines[rtpmapLine+1] = sdpLines[rtpmapLine+1].append(" x-google-min-bitrate=" + vBitrate);
                        sdpLines[rtpmapLine+1] = sdpLines[rtpmapLine+1].append(";x-google-max-bitrate=" + vBitrate);
                    }
                }
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

    // Remove all payloads from SDP except video_codec & audio_codec
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
        // Retained payloads stored in audio_payload_numbers, video_payload_numbers
        filterPayloads(sdp, audio_payloads, audio_payload_numbers, "audio", audio_codec,
                       h264_packetization_mode, h264_profile_level_id, vp9_profile_id);
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

    // Change priority of UDP to 50, TCP to 49
    static void preferUdpCandidate(std::string &candidate)
    {
        std::smatch match;
        std::regex re("candidate:([0-9]+) ([0-9]+) (TCP|UDP) ([0-9]+) ([0-9.]+) ([0-9]+) ([^0-9]+) ([0-9]+)");
        if (std::regex_search(candidate, match, re)) {
            if (caseInsensitiveStringCompare(match[3].str(), "TCP"))
                candidate = std::regex_replace(candidate, re, "candidate:$1 $2 $3 49 $5 $6 $7 $8");
            if (caseInsensitiveStringCompare(match[3].str(), "UDP"))
                candidate = std::regex_replace(candidate, re, "candidate:$1 $2 $3 50 $5 $6 $7 $8");
        }
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

    // Remove all payloads from SDP except specified codecs
    static void filterPayloads(std::string &sdp,
                               std::string &payloads,
                               std::vector<int> &payload_numbers,
                               const std::string &media_type,
                               const std::initializer_list<std::string> &codecs)
    {
        return filterPayloads(sdp, payloads, payload_numbers, media_type, codecs,
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
        int audioLine = findLines(sdpLines, "m=audio ");
        int videoLine = findLines(sdpLines, "m=video ");
        int for_start = 0;
        int for_end = sdpLines.size();
        if (media_type == "audio") {
            for_start = audioLine;
            for_end = audioLine < videoLine ? videoLine : sdpLines.size();
        } else if (media_type == "video") {
            for_start = videoLine;
            for_end = videoLine < audioLine ? audioLine : sdpLines.size();
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
                                    apt_payload_numbers,
                                    payloadCodec, payloadNumber, all, found,
                                    h264_packetization_mode,
                                    h264_profile_level_id, vp9_profile_id);
            }
        }
    }

    // After verifying that the initializer list varient is working for all cases,
    // uncomment the following block and remove the preceding method

    /*
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
        std::initializer_list<std::string> codecs = { media_codec };
        return filterPayloads(sdp, payloads, payload_numbers, media_type, codecs,
                              h264_packetization_mode, h264_profile_level_id, vp9_profile_id);
    }
    */

    // Remove all payloads of media_type from SDP except specified list of codecs
    static void filterPayloads(std::string &sdp,
                               std::string &payloads,
                               std::vector<int> &payload_numbers,
                               const std::string &media_type,
                               const std::initializer_list<std::string> &codecs,
                               const int h264_packetization_mode,
                               const std::string &h264_profile_level_id,
                               const int vp9_profile_id)
    {
        std::vector<int> apt_payload_numbers;
        std::vector<std::string> sdpLines;
        split(sdp, (char *)"\r\n", sdpLines);
        int audioLine = findLines(sdpLines, "m=audio ");
        int videoLine = findLines(sdpLines, "m=video ");
        int for_start = 0;
        int for_end = sdpLines.size();
        if (media_type == "audio") {
            for_start = audioLine;
            for_end = videoLine;
        } else if (media_type == "video") {
            for_start = videoLine;
        }
        for (int i = for_start; i < for_end; i++) {
            std::smatch match;
            std::string payloadRe = "a=rtpmap:([0-9]+) ([a-zA-Z0-9-]+)";
            std::regex re(payloadRe);
            if (std::regex_search(sdpLines[i], match, re)) {
                std::string payloadNumber = match[1].str();
                std::string payloadCodec = match[2].str();
                bool found = false;
                for (const auto &codec : codecs) {
                    if (caseInsensitiveStringCompare(codec, payloadCodec)) {
                        found = true;
                        break;
                    }
                }
                bool all = codecs.size() == 0;
                getMatchingPayloads(sdp, sdpLines, payloads, payload_numbers,
                                    apt_payload_numbers,
                                    payloadCodec, payloadNumber, all, found,
                                    h264_packetization_mode,
                                    h264_profile_level_id, vp9_profile_id);
            }
        }
    }

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
        if (found) {
            if (caseInsensitiveStringCompare("h264", payloadCodec)) {
                std::string h264fmtp = " level-asymmetry-allowed=[0-1]";
                h264fmtp += ";packetization-mode=([0-9])";
                h264fmtp += ";profile-level-id=([0-9a-f]{6})";
                int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + h264fmtp);
                if (fmtpLine != -1) {
                    std::smatch matchFmtp;
                    std::regex reFmtp(payloadNumber + h264fmtp);
                    if (std::regex_search(sdpLines[fmtpLine], matchFmtp, reFmtp)) {
                        int pkt_mode = std::stoi(matchFmtp[1].str());
                        std::string p_level_id = matchFmtp[2].str();
                        if (caseInsensitiveStringCompare(h264_profile_level_id, p_level_id)
                                && h264_packetization_mode == pkt_mode) {
                            keep = true;
                        }
                    }
                }
            } else if (caseInsensitiveStringCompare("vp9", payloadCodec)) {
                std::string vp9fmtp = " profile-id=([0-9])";
                int fmtpLine = findLinesRegEx(sdpLines, payloadNumber + vp9fmtp);
                if (fmtpLine != -1) {
                    std::smatch matchFmtp;
                    std::regex reFmtp(payloadNumber + vp9fmtp);
                    if (std::regex_search(sdpLines[fmtpLine], matchFmtp, reFmtp)) {
                        int profile_id = std::stoi(matchFmtp[1].str());
                        if (vp9_profile_id == profile_id) {
                            keep = true;
                        }
                    }
                }
            } else {
                keep = true;
            }
        } else if (all) {
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
        while(token != NULL) {
            v.push_back(std::string(token));
            token = strtok(NULL, delim);
        }
        free(dup);
    }
};

#endif
