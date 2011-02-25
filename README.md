unit-test for my patch on tag encoding of libmedia:

[libmedia: Fixed bug on mix-encoded tag of Korean mp3s.](http://review.cyanogenmod.com/#change,2956) MERGED!! yei~

## Download ##
This test should placed under the libmedia

    $ cd $ANDROID_ROOT/framework/base/media/libmedia/
    $ git clone $LIBMEDIA_ADDR tests

## MediaScannerClientTest ##
Test MediaScannerClient handle various encoding correctly.

    MediaScannerClient_test.cpp : the gtest.
    testee.h : contains strings with CJK encodings and matched utf-8 strings.
    make_testee.py : make testee.h from data of the cddb.
