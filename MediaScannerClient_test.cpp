/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "MediaScannerClient_test"

#include <utils/Log.h>
#include <utils/StringArray.h>
#include <media/mediascanner.h>
#include "../autodetect.h"
#include <unicode/ustring.h>
#include <unicode/ucnv.h>
#include "testee.h"

#include <gtest/gtest.h>

namespace android {

static StringArray* results = NULL;

class TestableMediaScannerClient : public MediaScannerClient {
public:
    using MediaScannerClient::mNames;
    using MediaScannerClient::mValues;
    using MediaScannerClient::mLocaleEncoding;

    // not use these members.
    status_t scanFile(const char* path, long long lastModified,
                      long long fileSize, bool isDirectory, bool noMedia)  {
        return OK;
    }
    status_t setMimeType(const char* mimeType) {
        return OK;
    }

    // not use these members.
    bool scanFile(const char* path, long long lastModified, long long fileSize) {
        return true;
    }
    bool setMimeType(const char* mimeType) {
        return true;
    }
    bool addNoMediaFolder(const char* path) {
        return true;
    }

    // the name will used for sorting value order on results.
    bool handleStringTag(const char* name, const char* value) {
        if (results == NULL)
            return false;

        int conLen = strlen(name) + strlen(value) + 1;
        char* conBuff = new char[conLen];
        if (!conBuff)
            return false;

        results->push_back(conBuff);

        delete[] conBuff;

        return OK;
    }

    // instead of using string tag name, we use sorting idx for
    // sort final result.
    bool addStringTag2(int sortingIdx, const char* value) {
        char strSortingIdx[4 + 1];
        sprintf(strSortingIdx, "%04d", sortingIdx);
        return addStringTag((const char*)strSortingIdx, value);
    }

    // ID3.cpp have been convert all native encoding strings to ISO8859-1
    // To simulate this situaltion turn forceConvertToLatin1 to true.
    bool addNativeStringTag(int sortingIdx, const char* value,
                            bool forceConvertToLatin1 = false) {
        UErrorCode status = U_ZERO_ERROR;

        UConverter* conv = ucnv_open("iso_8859_1", &status);
        if (U_FAILURE(status)) {
            LOGE("could not create UConverter for iso_8859_1\n");
            return false;
        }
        UConverter* utf8Conv = ucnv_open("UTF-8", &status);
        if (U_FAILURE(status)) {
            LOGE("could not create UConverter for UTF-8\n");
            ucnv_close(conv);
            return false;
        }
        bool ret;
        if (forceConvertToLatin1) {
            const char* src = value;
            int len = strlen(src);
            int targetLen = len * 3 + 1;
            char* buffer = new char[targetLen];
            char* target = buffer;
            ucnv_convertEx(utf8Conv, conv, &target, target + targetLen,
                           &src, src + len, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
            if (U_FAILURE(status)) {
                LOGE("ucnv_convertEx failed: %d\n", status);
                return false;
            } else {
                // zero terminate
                *target = 0;
                ret = addStringTag2(sortingIdx, buffer);
            }

        } else {
            ret = addStringTag2(sortingIdx, value);
        }
        return ret;
    }
};

class MediaScannerClientTest : public testing::Test {
protected:
    TestableMediaScannerClient* client;

    virtual void SetUp() {
        results = new StringArray;
        client = new TestableMediaScannerClient();
    }

    virtual void TearDown() {
        delete client;
        delete results;
        results = NULL;
    }
};

TEST_F(MediaScannerClientTest, IsResultSorted)
{
    // use name as a index for result sorting
    client->beginFile();
    client->addStringTag2(3, "third");
    client->addStringTag2(1, "first");
    client->addStringTag2(2, "second");
    client->endFile();

    results->sort(StringArray::cmpAscendingAlpha);

    // jump to the value from results
    EXPECT_STREQ(results->getEntry(0) + 4, "first");
    EXPECT_STREQ(results->getEntry(1) + 4, "second");
    EXPECT_STREQ(results->getEntry(2) + 4, "third");
}

// still good on ASCII
TEST_F(MediaScannerClientTest, ASCII)
{
    client->beginFile();
    client->addStringTag("Hello", "World");
    client->endFile();
    EXPECT_STREQ(results->getEntry(0), "HelloWorld");
}

static void __test_str_pairs(TestableMediaScannerClient* client,
                             str_pair* table, unsigned int table_size, bool is_native)
{
    client->beginFile();

    for (unsigned int i = 0; i < table_size; i++) {
        if (is_native)
            client->addNativeStringTag(i, table[i].native);
        else
            client->addStringTag2(i, table[i].native);
    }

    client->endFile();
    results->sort(StringArray::cmpAscendingAlpha);

    for (unsigned int i = 0; i < table_size; i++) {
        // skip idx string from result.
        EXPECT_STREQ(results->getEntry(i) + 4, table[i].utf_8);
    }
}
#define STRS_ARY_SIZE(x) sizeof(x)/sizeof(str_pair)
#define test_str_pairs(c, t, n) __test_str_pairs(c, t, STRS_ARY_SIZE(t), n)

// utf-8 should not be demaged by -whatever- current locale.
TEST_F(MediaScannerClientTest, UTF8)
{
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF8_with_ko)
{
    client->setLocale("ko");
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF8_with_ja)
{
    client->setLocale("ja");
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF8_with_zh)
{
    client->setLocale("zh");
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF8_with_zh_CN)
{
    client->setLocale("zh_CN");
    test_str_pairs(client, strs_utf_8, false);
}

// Latin-1 should not be demaged by -whatever- current locale
TEST_F(MediaScannerClientTest, Latin_1)
{
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_ko)
{
    client->setLocale("ko");
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_ja)
{
    client->setLocale("ja");
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_zh)
{
    client->setLocale("zh");
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_zh_CN)
{
    client->setLocale("zh_CN");
    test_str_pairs(client, strs_windows_1252, true);
}

// euc-kr should be changed to utf-8 when current locale is "ko"
TEST_F(MediaScannerClientTest, EUC_KR_with_ko)
{
    client->setLocale("ko");
    test_str_pairs(client, strs_EUC_KR, true);
}

// SHIFT-JIS should be changed to utf-8 when current locale is "ja"
TEST_F(MediaScannerClientTest, SHIFT_JIS_with_ja)
{
    client->setLocale("ja");
    test_str_pairs(client, strs_SHIFT_JIS, true);
}

// GBK should be changed to utf-8 when current locale is "zh_CN"
TEST_F(MediaScannerClientTest, GBK_with_with_zh_CN)
{
    client->setLocale("zh_CN");
    test_str_pairs(client, strs_GB2312, true);
}

// BIG should be changed to utf-8 when current locale is "zh"
TEST_F(MediaScannerClientTest, BIG5_with_zh)
{
    client->setLocale("zh");
    test_str_pairs(client, strs_Big5, true);
}

// Some Korean id3 has mix-encoded values. :(
TEST_F(MediaScannerClientTest, UTF8_and_native_encoding_in_a_id3_tagset)
{
    client->setLocale("ko");

    client->beginFile();
    client->addStringTag2(0, strs_utf_8[0].native);
    client->addNativeStringTag(1, strs_EUC_KR[0].native);
    client->endFile();

    results->sort(StringArray::cmpAscendingAlpha);

    EXPECT_STREQ(results->getEntry(0) + 4, strs_utf_8[0].utf_8);
    EXPECT_STREQ(results->getEntry(1) + 4, strs_EUC_KR[0].utf_8);
}

// Some Korean id3 is chopped wrongly. :(
TEST_F(MediaScannerClientTest, native_str_is_chopped_wrongly)
{
    client->setLocale("ko");

    client->beginFile();
    client->addNativeStringTag(0, "\xb9\xce\xc1\xd6\xb4\xe7\xb4\xe7\xb1\xc7\xc1\xd6\xc0\xda");
    /* last character should '\xc0\xda', 자. \xda chopped! fxxx! */
    client->addNativeStringTag(1, "\xb9\xce\xc1\xd6\xb4\xe7\xb4\xe7\xb1\xc7\xc1\xd6\xc0");
    client->endFile();

    results->sort(StringArray::cmpAscendingAlpha);

    /* hope android smartly ignore last, unfinished character, \xc0 */
    EXPECT_STREQ(results->getEntry(0) + 4, "민주당당권주자");
    EXPECT_STREQ(results->getEntry(1) + 4, "민주당당권주�");
}

// Sometimes latin-1 strings are decoded as GBK when Locale is "zh_CN"
TEST_F(MediaScannerClientTest, latin1_str_shouldnt_be_decoded_as_gbk)
{
    client->setLocale("zh_CN");

    client->beginFile();
    client->addNativeStringTag(0, "\x5A\x6C\x6F\x74\x6F\x77\x6C\x6F\x73\x61\x20\x6B\x72\xC3\xB3\x6C\x65\x77\x6E\x61"); // It was decoded as "Zlotowlosa kr贸lewna"
    client->addNativeStringTag(1, "\x4A\x4F\x48\x4E\x53\x4F\x4E\x27\x53\x20\x4A\x41\x5A\x5A\x45\x52\x53\x20\x2F\x20\xD0\xA1\xD0\xB0\x6E\x20\x49\x20\x47\x65\x74\x20\x59\x6F\x75\x20\x28\x57\x69\x6C\x6C\x69\x61\x6D\x73\x29"); // It was decoded as "JOHNSON'S JAZZERS / 小邪n I Get You (Williams)"
    client->addNativeStringTag(2,  "\x30\x34\x2E\x20\x57\x20\x67\xC3\xB3\x72\x61\x63\x68\x20\x7A\x6D\x69\x65\x72\x7A\x63\x68"); // It was decoded as "04. W g贸rach zmierzch"
    client->endFile();

    results->sort(StringArray::cmpAscendingAlpha);

    EXPECT_STREQ(results->getEntry(0) + 4, "Zlotowlosa królewna");
    EXPECT_STREQ(results->getEntry(1) + 4, "JOHNSON'S JAZZERS / Саn I Get You (Williams)");
    EXPECT_STREQ(results->getEntry(2) + 4, "04. W górach zmierzch");
}

}
