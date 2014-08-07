#!/usr/bin/env python

import collections
import json
import logging
import os
import struct
import sys
import time

from gevent import monkey; monkey.patch_all()
import gevent
import gevent.server
import leveldb

from pyutil import pttstruct
from pyutil import pttpost

# Ref: ../../include/daemons.h
RequestFormatString = 'HH'
Request = collections.namedtuple('Request', 'cb operation')
PostKeyFormatString = '%ds%ds' % (pttstruct.IDLEN + 1, pttstruct.FNLEN + 1)
PostKey = collections.namedtuple('PostKey', 'board file')
# TODO in future we don't need userref, and ctime should be automatically set.
AddRecordFormatString = 'III%ds' % (pttstruct.IDLEN + 1)
AddRecord = collections.namedtuple('AddRecord', 'userref ctime ipv4 userid')
CONTENT_LEN_FORMAT = 'I'
REQ_ADD = 1
REQ_IMPORT = 2
REQ_GET_CONTENT = 3
_SERVER_ADDR = '127.0.0.1'
_SERVER_PORT = 5135
_DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),
			'db_posts.db')
BBSHOME = '/home/bbs'

def serialize(data):
    return json.dumps(data)

def deserialize(data):
    return json.loads(data)

def DecodeFileHeader(blob):
    header = pttstruct.unpack_data(blob, pttstruct.FILEHEADER_FMT)
    return header

def EncodeFileHeader(header):
    blob = pttstruct.pack_data(header, pttstruct.FILEHEADER_FMT)
    return blob

def UnpackAddRecord(blob):
    def strip_if_string(v):
	return v.strip(chr(0)) if type(v) == str else v
    data = struct.unpack(AddRecordFormatString, blob)
    logging.debug("UnpackAddRecord: %r" % (data,))
    return AddRecord._make(map(strip_if_string, data))

def UnpackPostKey(blob):
    def strip_if_string(v):
	return v.strip(chr(0)) if type(v) == str else v
    data = struct.unpack(PostKeyFormatString, blob)
    logging.debug("UnpackPostKey: %r" % (data,))
    return PostKey._make(map(strip_if_string, data))

def PackPost(comment):
    return struct.pack(PostFormatString, *comment)

def LoadPost(query):
    logging.debug("LoadPost: %r", query)
    key = '%s/%s' % (query.board, query.file)
    num = int(g_db.get(key) or '0')
    if query.start >= num:
	return None
    key += '#%08d' % (query.start + 1)
    data = g_db.get(key)
    logging.debug(" => %r", UnpackPost(data))
    return UnpackPost(data)

def SavePost(legacy, keypak, data, extra=None):
    if extra:
	data.update(extra._asdict())
    logging.debug("SavePost: %r => %r", keypak, data)
    key = '%s/%s' % (keypak.board, keypak.file)
    g_db.set(key, serialize(data))
    logging.debug(' Saved: %s', key)
    content_file = os.path.join(BBSHOME, 'boards', keypak.board[0],
				keypak.board, keypak.file)
    if legacy:
	(content, comments) = pttpost.ParsePost(content_file)
	content_length = len(content)
	# TODO update comments
    else:
	content_length = os.path.getsize(content_file)
	content = open(content_file).read()
    start = time.time()
    g_db.set(key + ':content', content)
    exec_time = time.time() - start
    logging.debug(' Content (%d) save time: %.3fs.', content_length, exec_time)
    if exec_time > 0.1:
	logging.error('%s/%s: save time (%d bytes): %.3fs.',
		      keypak.board, keypak.file, content_length, exec_time)

def GetPostContent(keypak):
    logging.debug("GetPostContent: %r", keypak)
    key = '%s/%s:content' % (keypak.board, keypak.file)
    return g_db.get(key) or ''

def open_database(db_path):
    global g_db

    class LevelDBWrapper(object):
	def __init__(self, db):
	    self.db = db

	def get(self, key):
	    try:
		return self.db.Get(key)
	    except KeyError:
		return None

	def set(self, key, value):
	    self.db.Put(key, value)

	def RangeIter(self, **args):
	    return self.db.RangeIter(*args)

    g_db = LevelDBWrapper(leveldb.LevelDB(db_path))
    return g_db

def handle_request(socket, _):
    fd = socket.makefile('rw')
    try:
        req = fd.read(struct.calcsize(RequestFormatString))
	req = Request._make(struct.unpack(RequestFormatString, req))
	logging.debug('Found request: %d' % req.operation)
	if req.operation == REQ_ADD:
	    header_blob = fd.read(pttstruct.FILEHEADER_SIZE)
	    addblob = fd.read(struct.calcsize(AddRecordFormatString))
	    keyblob = fd.read(struct.calcsize(PostKeyFormatString))
	    SavePost(False, UnpackPostKey(keyblob),
		     DecodeFileHeader(header_blob), UnpackAddRecord(addblob))
	elif req.operation == REQ_IMPORT:
	    header_blob = fd.read(pttstruct.FILEHEADER_SIZE)
	    addblob = fd.read(struct.calcsize(AddRecordFormatString))
	    keyblob = fd.read(struct.calcsize(PostKeyFormatString))
	    SavePost(True, UnpackPostKey(keyblob),
		     DecodeFileHeader(header_blob), UnpackAddRecord(addblob))
	elif req.operation == REQ_GET_CONTENT:
	    keyblob = fd.read(struct.calcsize(PostKeyFormatString))
	    content = GetPostContent(UnpackPostKey(keyblob))
	    fd.write(struct.pack(CONTENT_LEN_FORMAT, len(content)))
	    fd.write(content)
	else:
	    raise ValueError('Unkown operation %d' % req.operation)
    except:
        logging.exception("handle_request")
    finally:
        try:
            fd.close()
            socket.close()
        except:
            pass

def main(myname, argv):
    level = logging.INFO
    level = logging.WARNING
    level = logging.DEBUG
    logging.basicConfig(level=level, format='%(asctime)-15s %(message)s')
    if len(argv) not in [0, 1]:
        print "Usage: %s [db_path]" % myname
        exit(1)
    db_path = argv[0] if len(argv) > 0 else _DB_PATH
    logging.warn("Serving at %s:%s [db:%s][pid:%d]...",
                 _SERVER_ADDR, _SERVER_PORT, db_path, os.getpid())
    open_database(db_path)
    server = gevent.server.StreamServer(
	    (_SERVER_ADDR, _SERVER_PORT), handle_request)
    server.serve_forever()

if __name__ == '__main__':
    main(sys.argv[0], sys.argv[1:])
