// The CHD hunk codecs (chdcodec.h): each one's round trip, the "not smaller" rule, and codec list parsing.

#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <vector>

#include "emulator/io/storage/chd/chdcodec.h"
#include "3rdparty/liblzma/include/Alloc.h"
#include "3rdparty/liblzma/include/LzmaDec.h"
#include "3rdparty/liblzma/include/LzmaEnc.h"

using namespace chd;

class ChdLzmaCompatibility_Test : public ::testing::TestWithParam<uint32_t> {};

TEST_P(ChdLzmaCompatibility_Test, InteroperatesWithSdkRawStreams)
{
    const uint32_t hunk = GetParam();
    std::vector<uint8_t> input(hunk);
    for (uint32_t i = 0; i < hunk; ++i)
        input[i] = static_cast<uint8_t>((i % 257) * 37);
    std::vector<uint8_t> packed(hunk);
    std::vector<uint8_t> unpacked(hunk);

    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    props.level = 6;
    props.reduceSize = hunk;
    LzmaEncProps_Normalize(&props);
    Byte properties[LZMA_PROPS_SIZE] = {};
    auto codec = CreateCodec(kCodecLzma, hunk);
    for (int endMarker : {0, 1})
    {
        SCOPED_TRACE(endMarker);
        SizeT propertiesSize = LZMA_PROPS_SIZE;
        SizeT packedSize = hunk;
        ASSERT_EQ(LzmaEncode(packed.data(), &packedSize, input.data(), hunk, &props,
                            properties, &propertiesSize, endMarker, nullptr, &g_Alloc, &g_BigAlloc), SZ_OK);
        ASSERT_LT(packedSize, hunk);
        ASSERT_TRUE(codec->Decompress(packed.data(), static_cast<uint32_t>(packedSize), unpacked.data(), hunk));
        EXPECT_EQ(unpacked, input);
        // Cut into the compressed payload, before an optional end marker.
        EXPECT_FALSE(codec->Decompress(packed.data(), static_cast<uint32_t>(packedSize / 2), unpacked.data(), hunk));
        packed[packedSize] = 0;
        EXPECT_FALSE(codec->Decompress(packed.data(), static_cast<uint32_t>(packedSize + 1), unpacked.data(), hunk));
    }

    // Decode the active backend's output with the independent SDK decoder.
    uint32_t written = 0;
    ASSERT_TRUE(codec->Compress(input.data(), hunk, packed.data(), written));
    SizeT consumed = written;
    SizeT decoded = hunk;
    ELzmaStatus status;
    ASSERT_EQ(LzmaDecode(unpacked.data(), &decoded, packed.data(), &consumed,
                        properties, LZMA_PROPS_SIZE, LZMA_FINISH_END, &status, &g_Alloc), SZ_OK);
    EXPECT_EQ(decoded, hunk);
    EXPECT_EQ(consumed, written);
    EXPECT_EQ(status, LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK);
    EXPECT_EQ(unpacked, input);
}

INSTANTIATE_TEST_SUITE_P(HunkSizes, ChdLzmaCompatibility_Test, ::testing::Values(4096U, 6144U, 16384U));

TEST(ChdCodec_Test, EveryCodecRoundTripsAndRefusesToGrow)
{
    const uint32_t hunk = 4096;
    std::vector<uint8_t> text(hunk);
    const char words[] = "a sector of a hard disk holds five hundred and twelve bytes ";
    for (uint32_t i = 0; i < hunk; i++)
        text[i] = static_cast<uint8_t>(words[i % (sizeof(words) - 1)]);
    std::vector<uint8_t> audio(hunk);
    for (uint32_t i = 0; i < hunk / 2; i++)
    {
        const int16_t s = static_cast<int16_t>(10000 * std::sin(i / 9.0));
        audio[2 * i] = static_cast<uint8_t>(s);
        audio[2 * i + 1] = static_cast<uint8_t>(s >> 8);
    }
    std::mt19937 rng(3);
    std::vector<uint8_t> noise(hunk);
    for (uint8_t& b : noise)
        b = static_cast<uint8_t>(rng());

    for (uint32_t tag : {kCodecZlib, kCodecLzma, kCodecHuffman, kCodecFlac, kCodecZstd})
    {
        auto codec = CreateCodec(tag, hunk);
        ASSERT_NE(codec, nullptr) << CodecName(tag);
        const std::vector<uint8_t>& input = tag == kCodecFlac ? audio : text;
        std::vector<uint8_t> packed(hunk);
        uint32_t written = 0;
        ASSERT_TRUE(codec->Compress(input.data(), hunk, packed.data(), written)) << CodecName(tag);
        EXPECT_LT(written, hunk) << CodecName(tag);
        std::vector<uint8_t> unpacked(hunk);
        ASSERT_TRUE(codec->Decompress(packed.data(), written, unpacked.data(), hunk)) << CodecName(tag);
        EXPECT_EQ(unpacked, input) << CodecName(tag);

        // Noise does not get smaller: the hunk is stored as is
        EXPECT_FALSE(codec->Compress(noise.data(), hunk, packed.data(), written)) << CodecName(tag);
    }
    EXPECT_EQ(CreateCodec(kCodecCdLzma, hunk), nullptr);
    EXPECT_EQ(CreateCodec(kCodecAvHuff, hunk), nullptr);
}

TEST(ChdCodec_Test, CodecListsByName)
{
    CodecList codecs{};
    std::string error;
    ASSERT_TRUE(ParseCodecList("none", codecs, &error));
    EXPECT_EQ(codecs[0], kCodecNone);
    ASSERT_TRUE(ParseCodecList("default", codecs, &error));
    EXPECT_EQ(FormatCodecList(codecs), "lzma,zlib,huff,flac");
    ASSERT_TRUE(ParseCodecList("ZSTD + huffman", codecs, &error));
    EXPECT_EQ(FormatCodecList(codecs), "zstd,huff");
    EXPECT_FALSE(ParseCodecList("lzma,cdlz", codecs, &error));
    EXPECT_NE(error.find("cdlz"), std::string::npos) << error;
    EXPECT_FALSE(ParseCodecList("lzma,lzma", codecs, &error));
    EXPECT_FALSE(ParseCodecList("lzma,zlib,huff,flac,zstd", codecs, &error));
    EXPECT_EQ(CodecName(kCodecCdFlac), "cdfl");
}
