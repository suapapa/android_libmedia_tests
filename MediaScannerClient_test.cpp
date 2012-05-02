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
#include <unicode/ustring.h>
#include <unicode/ucnv.h>
#include <media/mediascanner.h>

#include <gtest/gtest.h>
#include "testee.h"

namespace android {

class TestableMediaScannerClient : public MediaScannerClient {
private:
    StringArray* _results;
    bool _isResultSorted;

public:
    // not use these members.
    virtual status_t scanFile(const char* path, long long lastModified,
                              long long fileSize, bool isDirectory, bool noMedia)  {
        return OK;
    }
    virtual status_t setMimeType(const char* mimeType) {
        return OK;
    }

    // push a tag's name/value pair to client. It called from addStringTag() and endFile()
    // for this TestableMediaScannerClient, It push name+value to _results.
    virtual status_t handleStringTag(const char* name, const char* value) {
        if (_results == NULL)
            return UNKNOWN_ERROR;

        int conLen = strlen(name) + strlen(value) + 1;
        char* conBuff = new char[conLen];
        if (!conBuff)
            return UNKNOWN_ERROR;

        sprintf(conBuff, "%s%s", name, value);
        _results->push_back(conBuff);

        delete[] conBuff;

        return OK;
    }

    // instead of using string tag name, we use sorting idx for
    // sort final result.
    bool addStringTagWithIdx(int sortingIdx, const char* value) {
        char strSortingIdx[4 + 1];
        sprintf(strSortingIdx, "%04d", sortingIdx);
        return addStringTag((const char*)strSortingIdx, value);
    }

    // ID3.cpp have been convert all native encoding strings to ISO8859-1
    // To simulate this situaltion turn forceConvertToLatin1 to true.
    bool addNativeStringTagWithIdx(int sortingIdx, const char* value,
                                   bool forceConvertToLatin1 = true) {
        char* buffer = NULL;
        if (forceConvertToLatin1) {
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

            const char* src = value;
            int len = strlen(src);
            int targetLen = len * 3 + 1;
            buffer = new char[targetLen];
            char* target = buffer;
            ucnv_convertEx(utf8Conv, conv, &target, target + targetLen,
                           &src, src + len, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
            if (U_FAILURE(status)) {
                LOGE("ucnv_convertEx failed: %d\n", status);
                return false;
            }
            // zero terminate
            *target = 0;
        } else {
            buffer = (char*)value;
        }

        return addStringTagWithIdx(sortingIdx, buffer);
    }

    void initResults() {
        _results = new StringArray;
        _isResultSorted = false;
    }

    void releaseResults() {
        delete _results;
    }

    const char* getResult(int idx) {
        if (!_isResultSorted) {
            _results->sort(StringArray::cmpAscendingAlpha);
            _isResultSorted = true;
        }
        return _results->getEntry(idx) + 4;
    }
};

class MediaScannerClientTest : public testing::Test {
protected:
    TestableMediaScannerClient* client;

    virtual void SetUp() {
        client = new TestableMediaScannerClient();
        client->initResults();
    }

    virtual void TearDown() {
        client->releaseResults();
        delete client;
    }
};

TEST_F(MediaScannerClientTest, IsResultSorted)
{
    // idx is used to sort final results
    client->beginFile();
    client->addStringTagWithIdx(3, "third");
    client->addStringTagWithIdx(1, "first");
    client->addStringTagWithIdx(2, "second");
    client->endFile();

    // get result by index.
    EXPECT_STREQ(client->getResult(0), "first");
    EXPECT_STREQ(client->getResult(1), "second");
    EXPECT_STREQ(client->getResult(2), "third");
}

static void __test_str_pairs(TestableMediaScannerClient* client,
                             str_pair* table, unsigned int table_size, bool is_native)
{
    client->beginFile();
    for (unsigned int i = 0; i < table_size; i++) {
        if (is_native)
            client->addNativeStringTagWithIdx(i, table[i].native);
        else
            client->addStringTagWithIdx(i, table[i].native);
    }
    client->endFile();

    for (unsigned int i = 0; i < table_size; i++) {
        EXPECT_STREQ(client->getResult(i), table[i].utf_8);
    }
}

// utf-8 should not be demaged by -whatever- current locale.
#define test_utf8_str_pairs(c, t) __test_str_pairs(c, t, (sizeof(t)/sizeof(str_pair)), false)
TEST_F(MediaScannerClientTest, UTF8)
{
    test_utf8_str_pairs(client, strs_utf_8);
}

TEST_F(MediaScannerClientTest, UTF8_with_ko)
{
    client->setLocale("ko");
    test_utf8_str_pairs(client, strs_utf_8);
}

TEST_F(MediaScannerClientTest, UTF8_with_ja)
{
    client->setLocale("ja");
    test_utf8_str_pairs(client, strs_utf_8);
}

TEST_F(MediaScannerClientTest, UTF8_with_zh)
{
    client->setLocale("zh");
    test_utf8_str_pairs(client, strs_utf_8);
}

TEST_F(MediaScannerClientTest, UTF8_with_zh_CN)
{
    client->setLocale("zh_CN");
    test_utf8_str_pairs(client, strs_utf_8);
}

// Latin-1 should not be demaged by -whatever- current locale
#define test_native_str_pairs(c, t) __test_str_pairs(c, t, (sizeof(t)/sizeof(str_pair)), true)
TEST_F(MediaScannerClientTest, Latin_1)
{
    test_native_str_pairs(client, strs_windows_1252);
}

TEST_F(MediaScannerClientTest, Latin_1_with_ko)
{
    client->setLocale("ko");
    test_native_str_pairs(client, strs_windows_1252);
}

TEST_F(MediaScannerClientTest, Latin_1_with_ja)
{
    client->setLocale("ja");
    test_native_str_pairs(client, strs_windows_1252);
}

TEST_F(MediaScannerClientTest, Latin_1_with_zh)
{
    client->setLocale("zh");
    test_native_str_pairs(client, strs_windows_1252);
}

TEST_F(MediaScannerClientTest, Latin_1_with_zh_CN)
{
    client->setLocale("zh_CN");
    test_native_str_pairs(client, strs_windows_1252);
}

// euc-kr should be changed to utf-8 when current locale is "ko"
TEST_F(MediaScannerClientTest, EUC_KR_with_ko)
{
    client->setLocale("ko");
    test_native_str_pairs(client, strs_EUC_KR);
}

// SHIFT-JIS should be changed to utf-8 when current locale is "ja"
TEST_F(MediaScannerClientTest, SHIFT_JIS_with_ja)
{
    client->setLocale("ja");
    test_native_str_pairs(client, strs_SHIFT_JIS);
}

// GBK should be changed to utf-8 when current locale is "zh_CN"
TEST_F(MediaScannerClientTest, GBK_with_with_zh_CN)
{
    client->setLocale("zh_CN");
    test_native_str_pairs(client, strs_GB2312);
}

// BIG should be changed to utf-8 when current locale is "zh"
TEST_F(MediaScannerClientTest, BIG5_with_zh)
{
    client->setLocale("zh");
    test_native_str_pairs(client, strs_Big5);
}

// Some Korean id3 has mix-encoded values. :(
TEST_F(MediaScannerClientTest, UTF8_and_native_encoding_in_a_id3_tagset)
{
    client->setLocale("ko");

    client->beginFile();
    client->addStringTagWithIdx(0, strs_utf_8[0].native);
    client->addNativeStringTagWithIdx(1, strs_EUC_KR[0].native);
    client->endFile();

    EXPECT_STREQ(client->getResult(0), strs_utf_8[0].utf_8);
    EXPECT_STREQ(client->getResult(1), strs_EUC_KR[0].utf_8);
}

// Some Korean id3 is chopped wrongly. :(
TEST_F(MediaScannerClientTest, native_str_is_chopped_wrongly)
{
    client->setLocale("ko");

    client->beginFile();
    client->addNativeStringTagWithIdx(0, "\xb9\xce\xc1\xd6\xb4\xe7\xb4\xe7\xb1\xc7\xc1\xd6\xc0\xda");
    /* last character should '\xc0\xda', 자. \xda chopped! fxxx! */
    client->addNativeStringTagWithIdx(1, "\xb9\xce\xc1\xd6\xb4\xe7\xb4\xe7\xb1\xc7\xc1\xd6\xc0");
    client->endFile();

    /* hope android smartly ignore last, unfinished character, \xc0 */
    EXPECT_STREQ(client->getResult(0), "민주당당권주자");
    EXPECT_STREQ(client->getResult(1), "민주당당권주�");
}

// Sometimes latin-1 strings are decoded as GBK when Locale is "zh_CN"
TEST_F(MediaScannerClientTest, latin1_str_shouldnt_be_decoded_as_gbk)
{
    client->setLocale("zh_CN");

    client->beginFile();
    client->addNativeStringTagWithIdx(0, "\x5A\x6C\x6F\x74\x6F\x77\x6C\x6F\x73\x61\x20\x6B\x72\xC3\xB3\x6C\x65\x77\x6E\x61"); // It was decoded as "Zlotowlosa kr贸lewna"
    client->addNativeStringTagWithIdx(1, "\x4A\x4F\x48\x4E\x53\x4F\x4E\x27\x53\x20\x4A\x41\x5A\x5A\x45\x52\x53\x20\x2F\x20\xD0\xA1\xD0\xB0\x6E\x20\x49\x20\x47\x65\x74\x20\x59\x6F\x75\x20\x28\x57\x69\x6C\x6C\x69\x61\x6D\x73\x29"); // It was decoded as "JOHNSON'S JAZZERS / 小邪n I Get You (Williams)"
    client->addNativeStringTagWithIdx(2,  "\x30\x34\x2E\x20\x57\x20\x67\xC3\xB3\x72\x61\x63\x68\x20\x7A\x6D\x69\x65\x72\x7A\x63\x68"); // It was decoded as "04. W g贸rach zmierzch"
    client->endFile();

    EXPECT_STREQ(client->getResult(0), "Zlotowlosa królewna");
    EXPECT_STREQ(client->getResult(1), "JOHNSON'S JAZZERS / Саn I Get You (Williams)");
    EXPECT_STREQ(client->getResult(2), "04. W górach zmierzch");
}

}
