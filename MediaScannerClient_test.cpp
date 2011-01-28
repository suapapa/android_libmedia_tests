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

#include "testee.h"

#include <gtest/gtest.h>

namespace android {

static StringArray *results = NULL;

class TestableMediaScannerClient : public MediaScannerClient {
    private:
        void latin1_to_utf8(const char *src, char* dest) {
            char *wp = dest;
            char c;

            for (unsigned int i=0; i < strlen(src); i++) {
                c = src[i];
                if (c & 0x80) {
                    *(wp++) = ((src[i] >> 6) + 0xc0) & 0xff;
                    *(wp++) = ((src[i] & 0x3f) + 0x80) & 0xff;
                } else {
                    *(wp++) = src[i];
                }
            }
            *wp = 0;
        }

    public:
        using MediaScannerClient::mNames;
        using MediaScannerClient::mValues;
        using MediaScannerClient::mLocaleEncoding;

        // not use these members.
        bool scanFile(const char* path, long long lastModified, long long fileSize) { return true; }
        bool setMimeType(const char* mimeType) { return true; }
        bool addNoMediaFolder(const char* path) { return true; }

        // the name will used for sorting value order on results.
        bool handleStringTag(const char* name, const char* value) {
            if (results == NULL)
                return false;

            int conLen = strlen(name) + strlen(value) + 1;
            char *conBuff = new char[conLen];
            if (!conBuff)
                return false;

            sprintf(conBuff, "%s%s", name, value);

            results->push_back(conBuff);

            delete[] conBuff;

            return true;
        }

        bool addNativeStringTag(const char* name, const char* value) {
            char *n_value = new char[strlen(value) * 2 + 1];
            latin1_to_utf8(value, n_value);

            bool ret = addStringTag(name, n_value);
            delete[] n_value;

            return ret;
        }
};

class MediaScannerClientTest : public testing::Test {
    protected:
        TestableMediaScannerClient *client;

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

TEST_F(MediaScannerClientTest, IsResultSorted) {
    // use name as a index for result sorting
    client->beginFile();
    client->addStringTag("3", "third");
    client->addStringTag("1", "first");
    client->addStringTag("2", "second");
    client->endFile();

    results->sort(StringArray::cmpAscendingAlpha);

    // jump to the value from results
    EXPECT_STREQ(results->getEntry(0) + 1, "first");
    EXPECT_STREQ(results->getEntry(1) + 1, "second");
    EXPECT_STREQ(results->getEntry(2) + 1, "third");
}

#define STRS_ARY_SIZE(x) sizeof(x)/sizeof(str_pair)
static void __test_str_pairs(TestableMediaScannerClient *client,
        str_pair *table, unsigned int table_size, bool is_native) {
    client->beginFile();

    for (unsigned int i = 0; i < table_size; i++) {
        char idx[4 + 1];
        sprintf(idx, "%04d", i);
        if (is_native) {
            client->addNativeStringTag(idx, table[i].native);
        } else {
            client->addStringTag(idx, table[i].native);
        }

        ASSERT_FALSE(i > 9999);
    }

    client->endFile();
    results->sort(StringArray::cmpAscendingAlpha);

    for (unsigned int i = 0; i < table_size; i++) {
        // skip idx string from result.
        EXPECT_STREQ(results->getEntry(i) + 4, table[i].utf_8);
    }
}
#define test_str_pairs(c, t, n) __test_str_pairs(c, t, STRS_ARY_SIZE(t), n)

// still good on ASCII
TEST_F(MediaScannerClientTest, ASCII) {
    client->beginFile();
    client->addStringTag("Hello", "World");
    client->endFile();
    EXPECT_STREQ(results->getEntry(0), "HelloWorld");
}

// utf-8 should not be demaged by -whatever- current locale.
TEST_F(MediaScannerClientTest, UTF_8) {
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF_8_with_ko) {
    client->setLocale("ko");
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF_8_with_ja) {
    client->setLocale("ja");
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF_8_with_zh) {
    client->setLocale("zh");
    test_str_pairs(client, strs_utf_8, false);
}

TEST_F(MediaScannerClientTest, UTF_8_with_zh_CN) {
    client->setLocale("zh_CN");
    test_str_pairs(client, strs_utf_8, false);
}

// Latin-1 should not be demaged by -whatever- current locale
TEST_F(MediaScannerClientTest, Latin_1) {
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_ko) {
    client->setLocale("ko");
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_ja) {
    client->setLocale("ja");
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_zh) {
    client->setLocale("zh");
    test_str_pairs(client, strs_windows_1252, true);
}

TEST_F(MediaScannerClientTest, Latin_1_with_zh_CN) {
    client->setLocale("zh_CN");
    test_str_pairs(client, strs_windows_1252, true);
}

// euc-kr should be changed to utf-8 when current locale is "ko"
TEST_F(MediaScannerClientTest, EUC_KR_with_ko) {
    client->setLocale("ko");
    test_str_pairs(client, strs_EUC_KR, true);
}

// SHIFT-JIS should be changed to utf-8 when current locale is "ja"
TEST_F(MediaScannerClientTest, SHIFT_JIS_with_ja) {
    client->setLocale("ja");
    test_str_pairs(client, strs_SHIFT_JIS, true);
}

// GBK should be changed to utf-8 when current locale is "zh_CN"
TEST_F(MediaScannerClientTest, GBK_with_with_zh_CN) {
    client->setLocale("zh_CN");
    test_str_pairs(client, strs_GB2312, true);
}

// BIG should be changed to utf-8 when current locale is "zh"
TEST_F(MediaScannerClientTest, BIG5_with_zh) {
    client->setLocale("zh");
    test_str_pairs(client, strs_Big5, true);
}

// Some Korean id3 has mix-encoded values. :(
TEST_F(MediaScannerClientTest, some_stupid_use_EUC_KR_and_utf_8_together) {
    client->setLocale("ko");

    client->beginFile();
    client->addStringTag("00", "Hello ASCII");
    client->addStringTag("01", strs_utf_8[0].native);
    client->addNativeStringTag("02", strs_EUC_KR[0].native);
    client->endFile();

    results->sort(StringArray::cmpAscendingAlpha);

    EXPECT_STREQ(results->getEntry(0) + 2, "Hello ASCII");
    EXPECT_STREQ(results->getEntry(1) + 2, strs_utf_8[0].utf_8);
    EXPECT_STREQ(results->getEntry(2) + 2, strs_EUC_KR[0].utf_8);
}

}
