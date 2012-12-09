#!/usr/bin/env python
#-*- coding: big5 -*-

import collections
import math
import os
import time

BBSHOME = '/home/bbs'
INPUT_FILE = '%s/log/angel_perf.txt' % (BBSHOME)
# A regular week = 590 samples.
SAMPLE_MINIMAL = 200

Entry = collections.namedtuple('Entry', 'sample pause1 pause2 max avg std')
DEBUG = None

PREFIX_DOC = '''
== ���g�p�ѨϬ��ʸ�Ʋέp���G (�t�Φ۰ʲ���: %s) ==

����: �ѨϤ��|�b�s�p�D�H��ѨϮɧY�ɲέp�Ҧ����p�ѨϪ��A�A
      �o�� (1) �p�ѨϷ�ɬO�_�b�u�W (2) ���٩I�s���O�_����/����;
      ���W�����G�i���R�X�U�C�W��C

      �p�ѨϦW�٫᭱���Ʀr���Ӥp�ѨϳQ�έp�쪺���� (�H�U�� SAMPLE ��)�A
      �]�� SAMPLE �Ʀr�O���H�I�s�ɤ~�|��s�A�B���קK�c�N�~�Ʀr�y�����G�����A
      �έp�C 600 ��̦h��s�@��(�_�h���H�|�X�ۤv�W�u�}�����g���p�Ѩ�)�A
      �ҥH���Ʀr�j���W����(��������)�p�ѨϹ�ڤW�u�ɶ���ҡC

      ���y�ܻ��A�Y�Ϭ��s�]���N���p�Ѩϳ��S�W�u�L�A�i��u�O�W�u���d�ɶ���
      �L�u�άO�W�u�ɳ��S���s�ϥΪ̭n�I�s�p�ѨϡC

''' % (time.ctime())

def is_lazy(e):
    # 'LAZY'
    '\033[1;33m�H�U�O�o�q�ɶ���SAMPLE�ƹL�C���p�Ѩ�:\033[m'
    # return e.sample < (e.avg - 1.0 * e.std)
    return e.sample < 5

def is_all_reject2(e):
    # 'ALL_REJECT2'
    '\033[1;31m�H�U�O�����I�s���ɶ���ҹL��(�PSAMPLE�ۮt�p��2)���p�Ѩ�:\033[m'
    return (e.pause2 >= e.sample - 1)

def parse_perf_file(filename):
    data = {}
    max_sample = 0
    sum_sample = 0
    sum_sample_square = 0
    with open(filename, 'r') as f:
	for l in f:
	    ls = l.strip()
	    if ls.startswith('#') or ls == '':
		continue
            # format: no. uid sample pause1 pause
	    no, uid, sample, pause1, pause2 = ls.split()
	    data[uid] = map(int, (sample, pause1, pause2))
	    sample = int(sample)
	    sum_sample += sample
	    sum_sample_square += sample * sample
	    if sample > max_sample:
		max_sample = sample
    N = len(data) or 1
    avg_sample = sum_sample / N
    std_sample = math.sqrt((sum_sample_square -
			    (N * avg_sample * avg_sample)) / N)
    return max_sample, avg_sample, std_sample, data

def get_nick(uid):
    fn = '%s/home/%c/%s/angelmsg' % (BBSHOME, uid[0], uid)
    nick = ''
    if os.path.exists(fn):
	nick = open(fn).readline().strip().decode('big5').strip('%%[')
    else:
	nick = uid
    return (nick + '�p�Ѩ�'.decode('big5')).encode('big5')

def build_badges(max_sample, avg_sample, std_sample, data):
    result = {}
    filters = [is_all_reject2]
    for uid, e in data.items():
	nick = '%s (%d)' % (get_nick(uid), e[0])
	if DEBUG:
	    nick += ' {%s/%d/%d/%d}' % (uid, e[0], e[1], e[2])
	entry = Entry(e[0], e[1], e[2], max_sample, avg_sample, std_sample)
	if is_lazy(entry):
	    badges = [is_lazy.__doc__]
	else:
	    badges = [f.__doc__ for f in filters if f(entry)]
	for b in badges:
	    if b not in result:
		result[b] = []
	    result[b] += [nick]
    return result

def main():
    max_sample, avg_sample, std_sample, data = parse_perf_file(INPUT_FILE)
    # print 'max=%f, avg=%f, std=%f' % (max_sample, avg_sample, std_sample)
    if max_sample < SAMPLE_MINIMAL:
    	exit()

    print PREFIX_DOC
    if DEBUG:
	print 'max, avg, std: %d, %d, %d' % (max_sample, avg_sample, std_sample)
    else:
	print ' SAMPLE �Ƴ̤j�� / ���� / �зǮt: %d / %d / %d\n' % (
	       max_sample, avg_sample, std_sample)
    result = build_badges(max_sample, avg_sample, std_sample, data)
    for k, v in result.items():
	print '%s:\n  %s\n' % (k, '\n  '.join(v))

if __name__ == '__main__':
    main()
