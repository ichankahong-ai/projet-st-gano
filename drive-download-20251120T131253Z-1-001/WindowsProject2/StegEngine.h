#pragma once
#include <windows.h>
#include <string>

namespace StegEngine {
    const uint32_t MAGIC = 0xDEADBEEF;
    void EmbedLSB(HBITMAP hBmp, const std::string& msg);
    std::string ExtractLSB(HBITMAP hBmp);

    uint32_t CRC32(const std::string& data);
}
#include "StegEngine.h"
#include <vector>
#include <cstdint>

namespace StegEngine {

    // ---------- Utilitaires ----------
    static inline int PixelOffset(int bitPos)
    {
        // Chaque pixel = 4 octets (BGRA)
        int pixelIndex = (bitPos / 3);  // chaque 3 bits = 1 pixel (R,G,B)
        int channel = bitPos % 3;       // 0=B, 1=G, 2=R
        return pixelIndex * 4 + channel;
    }

    // ---------- CRC32 ----------
    static std::vector<uint32_t> generate_crc32_table() {
        std::vector<uint32_t> table(256);
        const uint32_t polynomial = 0xEDB88320;
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (size_t j = 0; j < 8; j++) {
                if (c & 1)
                    c = polynomial ^ (c >> 1);
                else
                    c >>= 1;
            }
            table[i] = c;
        }
        return table;
    }

    static uint32_t CRC32(const std::string& data) {
        static std::vector<uint32_t> table = generate_crc32_table();
        uint32_t crc = 0xFFFFFFFF;
        for (unsigned char byte : data)
            crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFF;
    }

    // ---------- EMBED ----------
    void EmbedLSB(HBITMAP hBmp, const std::string& msg)
    {
        BITMAP bm;
        GetObject(hBmp, sizeof(bm), &bm);

        int width = bm.bmWidth;
        int height = bm.bmHeight;
        int rowSize = width * 4;  // 4 bytes per pixel (BGRA)

        std::vector<unsigned char> px(rowSize * height);
        GetBitmapBits(hBmp, px.size(), px.data());

        // Payload = MAGIC + len + CRC32 + msg
        std::vector<unsigned char> data;

        // MAGIC
        for (int i = 0; i < 4; i++)
            data.push_back((MAGIC >> (i * 8)) & 0xFF);

        // LEN
        uint32_t len = msg.size();
        for (int i = 0; i < 4; i++)
            data.push_back((len >> (i * 8)) & 0xFF);

        // CRC32
        uint32_t crc = CRC32(msg);
        for (int i = 0; i < 4; i++)
            data.push_back((crc >> (i * 8)) & 0xFF);

        // MSG
        data.insert(data.end(), msg.begin(), msg.end());

        // Encodage LSB
        int bit = 0;
        const int capacityBits = px.size() * 3; // 3 bits par pixel (RGB)
        for (unsigned char c : data)
        {
            for (int b = 0; b < 8; b++)
            {
                if (bit >= capacityBits)
                    return;

                int off = PixelOffset(bit);
                px[off] &= 0xFE;
                px[off] |= (c >> b) & 1;

                bit++;
            }
        }

        SetBitmapBits(hBmp, px.size(), px.data());
    }

    // ---------- EXTRACT ----------
    std::string ExtractLSB(HBITMAP hBmp)
    {
        BITMAP bm;
        GetObject(hBmp, sizeof(bm), &bm);

        int width = bm.bmWidth;
        int height = bm.bmHeight;
        int rowSize = width * 4;

        std::vector<unsigned char> px(rowSize * height);
        GetBitmapBits(hBmp, px.size(), px.data());

        auto readByte = [&](int& bitPos) -> unsigned char {
            unsigned char c = 0;
            for (int b = 0; b < 8; b++)
            {
                int off = PixelOffset(bitPos);
                c |= (px[off] & 1) << b;
                bitPos++;
            }
            return c;
            };

        int bit = 0;

        // MAGIC
        uint32_t magic = 0;
        for (int i = 0; i < 4; i++)
            magic |= readByte(bit) << (i * 8);
        if (magic != MAGIC) return "";

        // LEN
        uint32_t len = 0;
        for (int i = 0; i < 4; i++)
            len |= readByte(bit) << (i * 8);

        // CRC32 stocké
        uint32_t storedCRC = 0;
        for (int i = 0; i < 4; i++)
            storedCRC |= readByte(bit) << (i * 8);

        // MSG
        std::string msg(len, '\0');
        for (uint32_t i = 0; i < len; i++)
            msg[i] = readByte(bit);

        // Vérification CRC
        if (CRC32(msg) != storedCRC)
            return ""; // message corrompu

        return msg;
    }

}

