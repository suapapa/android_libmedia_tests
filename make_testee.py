#!/usr/bin/python
# -*- coding: utf-8 -*-

# make_testee.py - make testee.h from cddb
#
# Copyright (C) 2011 Homin Lee <ff4500@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

import os
import glob
import chardet
import random

freedbAddr = "http://ftp.freedb.org/pub/freedb/"
dbNameTemplete = "freedb-update-%s-%s.tar.bz2"

yearStart, monthStart = 2010, 6
yearEnd, monthEnd = 2010, 12

itemCntForEachEncoding = 50
outputName = "testee.h"

# hardly find SHIFT-JIS strings. so made it from EUC-JP strings
encodings = ['utf-8', 'windows-1252', 'EUC-KR', 'EUC-JP', 'Big5', 'GB2312']

for y in range(yearStart, yearEnd + 1):
    for m in range(monthStart, monthEnd + 1):
        y_b = y
        m_b = m + 1
        if m_b > 12:
            y_b += 1
            m_b = 1

        dbName = dbNameTemplete %\
            ("%04d%02d01"%(y, m),\
            "%04d%02d01"%(y_b, m_b))

        if os.path.exists(dbName):
            continue

        downAddr = freedbAddr + dbName
        #print downAddr
        os.system("wget " + downAddr)

os.system('mkdir -p cddb')

for z in glob.glob("*.tar.bz2"):
    os.system("tar -C cddb -xvjf " + z)

cds = glob.glob("cddb/*/*")
print "Generating %s from %d cddb items.."%(outputName, len(cds))

testeeDict = {}

def escapeAscii(text):
    retText = ''
    for c in text:
        if 1:#ord(c)&0x80:
            retText += r"\x%02X"%ord(c)
        else:
            retText += c
    return retText

random.shuffle(cds)
for cd in cds:
    if os.path.isdir(cd):
        continue

    enc = chardet.detect(open(cd).read())['encoding']
    if not enc in encodings:
        continue

    for line in open(cd):
        if not line.startswith('TTITLE'):
            continue
        lineEnc = chardet.detect(line)['encoding']
        if not lineEnc == enc:
            continue

        text = line[line.find('=')+1:].strip()
        if not text:
            continue

        #print enc, text
        #if not enc: enc = "None"


        if not enc in testeeDict.keys():
            testeeDict[enc] = [text]
            print 'find new encoding, %s'%enc
            break
        elif len(testeeDict[enc]) < itemCntForEachEncoding:
            testeeDict[enc] += [text]
            break

    isEnough = True
    for enc in encodings:
        if not enc in testeeDict.keys():
            isEnough = False
            break
        if len(testeeDict[enc]) < itemCntForEachEncoding:
            isEnough = False
            break

    if isEnough:
        break




# if no shift-jis found make it from euc-jp
if not "SHIFT-JIS" in testeeDict.keys():
    if "EUC-JP" in testeeDict.keys():
        testeeDict["SHIFT-JIS"] = \
            map(lambda(x):x.decode("EUC-JP").encode("SHIFT-JIS"), testeeDict['EUC-JP'])


print 'write testee to %s'%outputName
wp = open(outputName, 'w')
wp.write('''struct str_pair {
    const char *native;
    const char *utf_8;
};

struct enc_table {
    const char *encoding;
    struct str_pair *table;
};\n\n''')

for enc in testeeDict.keys():
    if enc == "EUC-JP":
        continue
    wp.write("struct str_pair strs_%s[] = {\n"%enc.replace('-', '_'))
    for text in testeeDict[enc]:
        text = text.replace('"', '\\"')
        if enc == 'None':
            wp.write('    {"%s", "%s"},\n'%(escapeAscii(text), escapeAscii(text)))
        if enc == 'ascii':
            wp.write('    {"%s", "%s"},\n'%(escapeAscii(text), escapeAscii(text)))
        else:
            try:
                wp.write('    {"%s", "%s"},\n'
                        %(escapeAscii(text), text.decode(enc).encode('utf-8')))
            except:
                wp.write('    //{"%s", "%s"}, // can\'t decode by python!\n'
                        %(escapeAscii(text), "???"))
    wp.write("};\n\n")

wp.close()

#os.system("rm -rf cddb")
#os.system("rm *.tar.bz2")
